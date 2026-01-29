#ifndef POST_MANAGEMENT_H
#define POST_MANAGEMENT_H

#include <stdbool.h>
#include "site-builder.h"

int add_post_to_builder(SiteBuilder *builder, const char *raw_date, const char *date, const char *title, const char *tags, const char *description, const char *filename);
int compare_posts(const void *a, const void *b);
void sort_posts(SiteBuilder *builder);
void append_post_link(String *content, PostInfo *post, const char *blog_base_url, bool show_description);
int generate_page_with_posts(SiteBuilder *builder, String *content, const char *title, const char *description, const char *filename, PostInfo *posts, int post_count);
int generate_index_page(SiteBuilder *builder, bool show_description);
int generate_archive_page(SiteBuilder *builder);

#endif
