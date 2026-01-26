#ifndef SITE_BUILDER_H
#define SITE_BUILDER_H

#include "org-string.h"

/* Constants */
#define MAX_PATH_LEN 512
#define INITIAL_POST_CAPACITY 64
#define INITIAL_TAG_CAPACITY 16
#define TAG_INITIAL_POST_CAPACITY 8
#define TEMPLATE_VAR_INITIAL_CAPACITY 8
#define DEFAULT_LINE_BUFFER_SIZE 1024
#define DEFAULT_STRING_BUFFER_SIZE 8192
#define OUTPUT_BUFFER_SIZE 16384
#define DATE_BUFFER_SIZE 32
#define PAGE_TITLE_BUFFER_SIZE 128

/* Blog URL constant */
#define BLOG_BASE_URL "https://www.vandee.art/blog/"

typedef struct {
    char *date;
    char *title;
    char *tags;
    char *filename;
} PostInfo;

typedef struct {
    char *input_dir;
    char *output_dir;
    char *template_dir;
    char *site_title;
    String *content;
    PostInfo *posts;
    int post_count;
    int post_capacity;
} SiteBuilder;

int mkdir_p(const char *path);
char *get_filename_without_ext(const char *filename);
char *join_path(const char *dir, const char *file);
int process_org_file(SiteBuilder *builder, const char *input_path, const char *output_path);
int process_directory(SiteBuilder *builder, const char *input_dir, const char *output_dir);
int generate_index_page(SiteBuilder *builder);
int generate_tags_page(SiteBuilder *builder);
int generate_individual_tag_pages(SiteBuilder *builder);
int generate_archive_page(SiteBuilder *builder);

#endif
