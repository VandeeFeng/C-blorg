#ifndef TAG_PAGES_H
#define TAG_PAGES_H

#include "site-builder.h"

typedef struct {
    char *name;
    PostInfo *posts;
    int count;
    int capacity;
} TagGroup;

void ensure_tag_capacity(TagGroup *tag, int initial_cap);
TagGroup *find_or_create_tag(TagGroup **tags, int *tag_count, int *tag_capacity, const char *tag_name, int initial_cap);
void add_post_to_tag(TagGroup *tag, PostInfo *post);
void process_post_tags(PostInfo *post, TagGroup **tags, int *tag_count, int *tag_capacity);
TagGroup *group_posts_by_tags(SiteBuilder *builder, int *tag_count_out);
void free_tag_groups(TagGroup *tags, int tag_count);
void append_tag_group_content(String *content, TagGroup *tag, const char *blog_base_url);
String *generate_all_tags_content(SiteBuilder *builder, int *tag_count_out);
void generate_single_tag_page(SiteBuilder *builder, TagGroup *tag, const char *tag_dir);
int generate_tags_page(SiteBuilder *builder);
int generate_individual_tag_pages(SiteBuilder *builder);

#endif
