#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "site-builder/filesystem.h"
#include "site-builder.h"
#include "site-builder/org-parser.h"

int mkdir_p(const char *path) {
    char tmp[MAX_PATH_LEN];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    return mkdir(tmp, 0755);
}

char* get_filename_without_ext(const char *filename) {
    char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        return strdup(filename);
    }
    size_t len = dot - filename;
    char *result = malloc(len + 1);
    if (result) {
        strncpy(result, filename, len);
        result[len] = '\0';
    }
    return result;
}

char* join_path(const char *dir, const char *file) {
    size_t dir_len = strlen(dir);
    size_t file_len = strlen(file);
    size_t total_len = dir_len + file_len + 2;

    char *result = malloc(total_len);
    if (!result) return NULL;

    strcpy(result, dir);
    if (dir_len > 0 && result[dir_len - 1] != '/') {
        strcat(result, "/");
    }
    strcat(result, file);

    return result;
}

int process_regular_file(SiteBuilder *builder, const char *input_path, const char *output_dir, const char *filename) {
    size_t name_len = strlen(filename);
    int is_org = name_len >= 4 && strcmp(filename + name_len - 4, ".org") == 0;

    if (!is_org) return 0;

    char *output_filename = get_filename_without_ext(filename);
    char *final_path = join_path(output_dir, output_filename);
    free(output_filename);

    char *html_filename = malloc(strlen(final_path) + 6);
    sprintf(html_filename, "%s.html", final_path);
    free(final_path);

    int result = process_org_file(builder, input_path, html_filename);
    free(html_filename);

    return result;
}

int process_directory(SiteBuilder *builder, const char *input_dir, const char *output_dir) {
    DIR *dir = opendir(input_dir);
    if (!dir) {
        fprintf(stderr, "ERROR: Failed to open directory %s\n", input_dir);
        return 1;
    }

    struct dirent *entry;
    int error_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char *input_path = join_path(input_dir, entry->d_name);
        char *output_path = join_path(output_dir, entry->d_name);

        struct stat st;
        if (stat(input_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                mkdir_p(output_path);
                error_count += process_directory(builder, input_path, output_path);
            } else if (S_ISREG(st.st_mode)) {
                error_count += process_regular_file(builder, input_path, output_dir, entry->d_name);
            }
        }

        free(input_path);
        free(output_path);
    }

    closedir(dir);
    return error_count;
}

int copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) {
        fprintf(stderr, "ERROR: Failed to open source file %s\n", src);
        return 1;
    }

    FILE *out = fopen(dst, "wb");
    if (!out) {
        fprintf(stderr, "ERROR: Failed to open destination file %s\n", dst);
        fclose(in);
        return 1;
    }

    char buffer[8192];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        if (fwrite(buffer, 1, bytes, out) != bytes) {
            fprintf(stderr, "ERROR: Failed to write to %s\n", dst);
            fclose(in);
            fclose(out);
            return 1;
        }
    }

    fclose(in);
    fclose(out);
    printf("Copied: %s -> %s\n", src, dst);
    return 0;
}

int copy_directory_recursive(const char *src_dir, const char *dst_dir) {
    DIR *dir = opendir(src_dir);
    if (!dir) {
        fprintf(stderr, "ERROR: Failed to open source directory %s\n", src_dir);
        return 1;
    }

    mkdir_p(dst_dir);

    struct dirent *entry;
    int error_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char *src_path = join_path(src_dir, entry->d_name);
        char *dst_path = join_path(dst_dir, entry->d_name);

        struct stat st;
        if (stat(src_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                error_count += copy_directory_recursive(src_path, dst_path);
            } else if (S_ISREG(st.st_mode)) {
                error_count += copy_file(src_path, dst_path);
            }
        }

        free(src_path);
        free(dst_path);
    }

    closedir(dir);
    return error_count;
}

int copy_template_assets(SiteBuilder *builder) {
    printf("\nCopying template assets...\n");

    char *custom_dir = join_path(builder->template_dir, "custom");
    DIR *dir = opendir(custom_dir);
    if (!dir) {
        printf("Warning: custom template directory not found: %s\n", custom_dir);
        free(custom_dir);
        return 0;
    }

    int error_count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char *src_path = join_path(custom_dir, entry->d_name);
        char *dst_path = join_path(builder->output_dir, entry->d_name);

        struct stat st;
        if (stat(src_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                error_count += copy_directory_recursive(src_path, dst_path);
                if (error_count == 0) {
                    printf("  Copied directory: %s\n", entry->d_name);
                }
            } else {
                error_count += copy_file(src_path, dst_path);
                if (error_count == 0) {
                    printf("  Copied file: %s\n", entry->d_name);
                }
            }
        }

        free(src_path);
        free(dst_path);
    }

    closedir(dir);
    free(custom_dir);

    return error_count;
}
