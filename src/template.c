#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "template.h"

static char *extract_include_filename(const char *cstr, size_t *i, size_t len) {
    size_t start = *i + 2;

    if (strncmp(cstr + start, "include ", 8) != 0) return NULL;

    size_t pos = start + 8;
    while (pos < len && !(cstr[pos] == '}' && pos + 1 < len && cstr[pos + 1] == '}')) {
        pos++;
    }

    if (pos >= len) return NULL;

    size_t filename_len = pos - (start + 8);
    char *filename = malloc(filename_len + 1);
    if (filename) {
        strncpy(filename, cstr + start + 8, filename_len);
        filename[filename_len] = '\0';
        *i = pos + 2;
    }

    return filename;
}

static int read_file_content(const char *path, String *output) {
    FILE *f = fopen(path, "r");
    if (!f) return 1;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc(size + 1);
    if (content) {
        size_t read_size = fread(content, 1, size, f);
        content[read_size] = '\0';
        string_append_cstr(output, content);
        free(content);
    }
    fclose(f);
    return 0;
}

static void process_single_include(const char *template_dir, String *result, char *filename) {
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", template_dir, filename);

    if (read_file_content(full_path, result) != 0) {
        fprintf(stderr, "Warning: Could not include template file: %s\n", full_path);
    }

    free(filename);
}

static void process_includes(String *content, const char *template_dir) {
    if (!content || !template_dir) return;

    char *cstr = string_to_cstr(content);
    size_t len = strlen(cstr);
    String *result = string_create(len);

    for (size_t i = 0; i < len; i++) {
        if (cstr[i] == '{' && i + 1 < len && cstr[i + 1] == '{') {
            char *filename = extract_include_filename(cstr, &i, len);
            if (filename) {
                process_single_include(template_dir, result, filename);
                continue;
            }
        }
        string_append(result, &cstr[i], 1);
    }

    char *result_str = string_to_cstr(result);
    free(content->data);
    content->data = strdup(result_str);
    content->len = strlen(result_str);
    content->cap = content->len + 1;
    string_free(result);
    free(result_str);
    free(cstr);
}

Template *template_create(const char *filename, const char *template_dir) {
    Template *t = malloc(sizeof(Template));
    if (!t) return NULL;

    t->content = NULL;
    t->vars = NULL;
    t->var_count = 0;
    t->var_capacity = 0;

    FILE *f = fopen(filename, "r");
    if (!f) {
        free(t);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *file_content = malloc(size + 1);
    if (!file_content) {
        fclose(f);
        free(t);
        return NULL;
    }

    size_t read_size = fread(file_content, 1, size, f);
    file_content[read_size] = '\0';
    fclose(f);

    t->content = string_create(strlen(file_content));
    string_append_cstr(t->content, file_content);
    free(file_content);

    if (template_dir) {
        process_includes(t->content, template_dir);
    }

    return t;
}

void template_free(Template *t) {
    if (!t) return;

    if (t->content) {
        string_free(t->content);
    }

    for (int i = 0; i < t->var_count; i++) {
        free(t->vars[i].key);
        free(t->vars[i].value);
    }

    if (t->vars) {
        free(t->vars);
    }

    free(t);
}

void template_set_var(Template *t, const char *key, const char *value) {
    if (!t || !key || !value) return;

    for (int i = 0; i < t->var_count; i++) {
        if (strcmp(t->vars[i].key, key) == 0) {
            free(t->vars[i].value);
            t->vars[i].value = strdup(value);
            return;
        }
    }

    if (t->var_count >= t->var_capacity) {
        int new_cap = t->var_capacity == 0 ? 8 : t->var_capacity * 2;
        TemplateVar *new_vars = realloc(t->vars, new_cap * sizeof(TemplateVar));
        if (!new_vars) return;

        t->vars = new_vars;
        t->var_capacity = new_cap;
    }

    t->vars[t->var_count].key = strdup(key);
    t->vars[t->var_count].value = strdup(value);
    t->var_count++;
}

static const char *find_template_var(Template *t, const char *key) {
    for (int i = 0; i < t->var_count; i++) {
        if (strcmp(t->vars[i].key, key) == 0) {
            return t->vars[i].value;
        }
    }
    return "";
}

static int extract_var_key(const char *content, size_t *i, size_t len, char **key_out) {
    size_t start = *i + 2;
    size_t pos = start;

    while (pos < len && !(content[pos] == '}' && pos + 1 < len && content[pos + 1] == '}')) {
        pos++;
    }

    if (pos >= len) return 0;

    size_t key_len = pos - start;
    *key_out = malloc(key_len + 1);
    if (*key_out) {
        strncpy(*key_out, content + start, key_len);
        (*key_out)[key_len] = '\0';
    }

    *i = pos + 1;
    return *key_out != NULL;
}

static int process_var_substitution(Template *t, String *output, const char *content, size_t *i, size_t len) {
    char *key = NULL;
    if (!extract_var_key(content, i, len, &key)) return 0;

    const char *value = find_template_var(t, key);
    string_append_cstr(output, value);
    free(key);

    return 1;
}

void template_render(Template *t, String *output) {
    if (!t || !output) return;

    char *content = string_to_cstr(t->content);
    size_t len = strlen(content);

    for (size_t i = 0; i < len; i++) {
        if (content[i] == '{' && i + 1 < len && content[i + 1] == '{') {
            if (process_var_substitution(t, output, content, &i, len)) {
                continue;
            }
        }
        string_append(output, &content[i], 1);
    }

    free(content);
}
