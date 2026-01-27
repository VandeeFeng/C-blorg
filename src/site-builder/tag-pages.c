#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "site-builder/tag-pages.h"
#include "site-builder.h"
#include "site-builder/page-renderer.h"
#include "site-builder/filesystem.h"
#include "site-builder/post-management.h"
#include "org-string.h"

void ensure_tag_capacity(TagGroup *tag, int initial_cap) {
    if (tag->count >= tag->capacity) {
        int new_cap = tag->capacity == 0 ? initial_cap : tag->capacity * 2;
        PostInfo *new_posts = realloc(tag->posts, new_cap * sizeof(PostInfo));
        if (new_posts) {
            tag->posts = new_posts;
            tag->capacity = new_cap;
        }
    }
}

TagGroup *find_or_create_tag(TagGroup **tags, int *tag_count, int *tag_capacity, const char *tag_name, int initial_cap) {
    for (int i = 0; i < *tag_count; i++) {
        if (strcmp((*tags)[i].name, tag_name) == 0) {
            return &(*tags)[i];
        }
    }

    if (*tag_count >= *tag_capacity) {
        int new_cap = *tag_capacity == 0 ? INITIAL_TAG_CAPACITY : *tag_capacity * 2;
        TagGroup *new_tags = realloc(*tags, new_cap * sizeof(TagGroup));
        if (!new_tags) return NULL;

        *tags = new_tags;
        *tag_capacity = new_cap;
    }

    (*tags)[*tag_count].name = strdup(tag_name);
    (*tags)[*tag_count].posts = malloc(initial_cap * sizeof(PostInfo));
    (*tags)[*tag_count].count = 0;
    (*tags)[*tag_count].capacity = initial_cap;
    (*tag_count)++;

    return &(*tags)[*tag_count - 1];
}

void add_post_to_tag(TagGroup *tag, PostInfo *post) {
    if (!tag || !post) return;
    ensure_tag_capacity(tag, TAG_INITIAL_POST_CAPACITY);
    tag->posts[tag->count++] = *post;
}

void process_post_tags(PostInfo *post, TagGroup **tags, int *tag_count, int *tag_capacity) {
    if (strlen(post->tags) == 0) return;

    char *tags_str = strdup(post->tags);
    char *token = strtok(tags_str, " ");

    while (token != NULL) {
        TagGroup *tag = find_or_create_tag(tags, tag_count, tag_capacity, token, TAG_INITIAL_POST_CAPACITY);
        if (tag) {
            add_post_to_tag(tag, post);
        }
        token = strtok(NULL, " ");
    }

    free(tags_str);
}

TagGroup *group_posts_by_tags(SiteBuilder *builder, int *tag_count_out) {
    TagGroup *tags = NULL;
    int tag_count = 0;
    int tag_capacity = 0;

    for (int i = 0; i < builder->post_count; i++) {
        process_post_tags(&builder->posts[i], &tags, &tag_count, &tag_capacity);
    }

    for (int i = 0; i < tag_count; i++) {
        qsort(tags[i].posts, tags[i].count, sizeof(PostInfo), compare_posts);
    }

    *tag_count_out = tag_count;
    return tags;
}

void free_tag_groups(TagGroup *tags, int tag_count) {
    for (int i = 0; i < tag_count; i++) {
        free(tags[i].name);
        free(tags[i].posts);
    }
    free(tags);
}

void append_tag_group_content(String *content, TagGroup *tag, const char *blog_base_url) {
    string_append_cstr(content, "<h1 class=\"tags-title\">Posts tagged \"");
    string_append_cstr(content, tag->name);
    string_append_cstr(content, "\":</h1>\n");
    for (int j = 0; j < tag->count; j++) {
        append_post_link(content, &tag->posts[j], blog_base_url);
    }
}

String *generate_all_tags_content(SiteBuilder *builder, int *tag_count_out) {
    String *content = string_create(DEFAULT_STRING_BUFFER_SIZE);
    int tag_count;
    TagGroup *tags = group_posts_by_tags(builder, &tag_count);

    for (int i = 0; i < tag_count; i++) {
        append_tag_group_content(content, &tags[i], builder->blog_base_url);
    }

    *tag_count_out = tag_count;
    free_tag_groups(tags, tag_count);
    return content;
}

void generate_single_tag_page(SiteBuilder *builder, TagGroup *tag, const char *tag_dir) {
    String *content = string_create(DEFAULT_STRING_BUFFER_SIZE);
    append_tag_group_content(content, tag, builder->blog_base_url);

    Template *tpl = load_base_template(builder);
    if (!tpl) {
        fprintf(stderr, "ERROR: Failed to load template for tag %s\n", tag->name);
        string_free(content);
        return;
    }

    String *page_title = string_create(PAGE_TITLE_BUFFER_SIZE);
    string_append_cstr(page_title, "Tag: ");
    string_append_cstr(page_title, tag->name);

    set_template_common_vars(tpl, builder, string_to_cstr(page_title), "Posts tagged with this tag", "", "", tag->name);

    char *output_filename = malloc(strlen(tag->name) + 6);
    sprintf(output_filename, "%s.html", tag->name);
    char *output_path = join_path(tag_dir, output_filename);

    render_and_write_page(tpl, content, output_path, output_filename);

    free(output_filename);
    free(output_path);
    string_free(page_title);
    string_free(content);
    template_free(tpl);
}

int generate_tags_page(SiteBuilder *builder) {
    if (builder->post_count == 0) {
        printf("No posts to generate tags page\n");
        return 0;
    }

    sort_posts(builder);

    int tag_count;
    String *content = generate_all_tags_content(builder, &tag_count);

    Template *tpl = load_base_template(builder);
    if (!tpl) {
        fprintf(stderr, "ERROR: Failed to load tags template\n");
        string_free(content);
        return 1;
    }

    set_template_common_vars(tpl, builder, "Tags", "All blog tags", "", "", "tags");

    char *output_path = join_path(builder->output_dir, "tags.html");
    int result = render_and_write_page(tpl, content, output_path, "tags.html");

    free(output_path);
    template_free(tpl);
    string_free(content);

    return result;
}

int generate_individual_tag_pages(SiteBuilder *builder) {
    if (builder->post_count == 0) {
        printf("No posts to generate individual tag pages\n");
        return 0;
    }

    sort_posts(builder);

    int tag_count;
    TagGroup *tags = group_posts_by_tags(builder, &tag_count);

    char *tag_dir = join_path(builder->output_dir, "tags");
    mkdir_p(tag_dir);

    for (int i = 0; i < tag_count; i++) {
        generate_single_tag_page(builder, &tags[i], tag_dir);
    }

    free_tag_groups(tags, tag_count);
    free(tag_dir);

    return 0;
}
