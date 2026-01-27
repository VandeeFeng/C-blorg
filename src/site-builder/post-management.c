#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "site-builder/post-management.h"
#include "site-builder.h"
#include "site-builder/page-renderer.h"
#include "site-builder/filesystem.h"
#include "org-string.h"

int add_post_to_builder(SiteBuilder *builder, const char *raw_date, const char *date, const char *title, const char *tags, const char *filename) {
    if (builder->post_count >= builder->post_capacity) {
        int new_cap = builder->post_capacity == 0 ? INITIAL_POST_CAPACITY : builder->post_capacity * 2;
        PostInfo *new_posts = realloc(builder->posts, new_cap * sizeof(PostInfo));
        if (!new_posts) return 1;

        builder->posts = new_posts;
        builder->post_capacity = new_cap;
    }

    builder->posts[builder->post_count].raw_date = strdup(raw_date);
    builder->posts[builder->post_count].date = strdup(date);
    builder->posts[builder->post_count].title = strdup(title);
    builder->posts[builder->post_count].tags = strdup(tags);
    builder->posts[builder->post_count].filename = strdup(filename);
    builder->post_count++;

    return 0;
}

void append_post_link(String *content, PostInfo *post, const char *blog_base_url) {
    string_append_cstr(content, "<h2 class=\"post-title\"><a href=\"");
    string_append_cstr(content, blog_base_url);
    string_append_cstr(content, post->filename);
    string_append_cstr(content, ".html\">");
    string_append_cstr(content, post->title);
    string_append_cstr(content, "</a></h2><div class=\"post-date\">");
    string_append_cstr(content, post->date);
    string_append_cstr(content, "</div><div class=\"taglist\"><a href=\"");
    string_append_cstr(content, blog_base_url);
    string_append_cstr(content, "tags.html\">Tags</a>: ");

    if (strlen(post->tags) > 0) {
        char *tags_str = strdup(post->tags);
        char *token = strtok(tags_str, " ");
        while (token != NULL) {
            string_append_cstr(content, "<a href=\"");
            string_append_cstr(content, blog_base_url);
            string_append_cstr(content, "tags/");
            string_append_cstr(content, token);
            string_append_cstr(content, ".html\">");
            string_append_cstr(content, token);
            string_append_cstr(content, "</a> ");
            token = strtok(NULL, " ");
        }
        free(tags_str);
    }

    string_append_cstr(content, "</div>");
}

int compare_posts(const void *a, const void *b) {
    const PostInfo *post_a = (const PostInfo *)a;
    const PostInfo *post_b = (const PostInfo *)b;
    return strcmp(post_b->raw_date, post_a->raw_date);
}

void sort_posts(SiteBuilder *builder) {
    if (builder->post_count < 2) return;
    qsort(builder->posts, builder->post_count, sizeof(PostInfo), compare_posts);
}

int generate_page_with_posts(SiteBuilder *builder, String *content, const char *title, const char *description, const char *filename, PostInfo *posts, int post_count) {
    for (int i = 0; i < post_count; i++) {
        append_post_link(content, &posts[i], builder->blog_base_url);
    }

    Template *tpl = load_base_template(builder);
    if (!tpl) {
        fprintf(stderr, "ERROR: Failed to load template for %s\n", filename);
        string_free(content);
        return 1;
    }

    set_template_common_vars(tpl, builder, title, description, "", "", filename);

    char *output_path = join_path(builder->output_dir, filename);
    int result = render_and_write_page(tpl, content, output_path, filename);

    free(output_path);
    template_free(tpl);
    string_free(content);

    return result;
}

int generate_index_page(SiteBuilder *builder) {
    if (builder->post_count == 0) {
        printf("No posts to generate index page\n");
        return 0;
    }

    sort_posts(builder);

    String *content = string_create(DEFAULT_STRING_BUFFER_SIZE);
    int recent_count = builder->post_count > 5 ? 5 : builder->post_count;
    for (int i = 0; i < recent_count; i++) {
        append_post_link(content, &builder->posts[i], builder->blog_base_url);
    }

    char *template_path = join_path(builder->template_dir, "index.html");
    Template *tpl = template_create(template_path, builder->template_dir);
    free(template_path);

    if (!tpl) {
        fprintf(stderr, "ERROR: Failed to load index template\n");
        string_free(content);
        return 1;
    }

    set_template_common_vars(tpl, builder, builder->site_title, "Vandee's Blog", "", "", "index.html");
    set_page_content(tpl, content);

    char *output_path = join_path(builder->output_dir, "index.html");
    String *output = string_create(OUTPUT_BUFFER_SIZE);
    template_render(tpl, output);
    int result = write_html_file(output_path, output, "index.html");

    free(output_path);
    template_free(tpl);
    string_free(output);
    string_free(content);

    return result;
}

int generate_archive_page(SiteBuilder *builder) {
    if (builder->post_count == 0) {
        printf("No posts to generate archive page\n");
        return 0;
    }

    sort_posts(builder);

    String *content = string_create(builder->post_count * 200);
    return generate_page_with_posts(builder, content, "Archive", "Archive of all blog posts", "archive.html", builder->posts, builder->post_count);
}
