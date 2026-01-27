#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "site-builder.h"
#include "org-ffi.h"
#include "template.h"

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

typedef struct {
    char *name;
    PostInfo *posts;
    int count;
    int capacity;
} TagGroup;

void set_template_common_vars(Template *tpl, SiteBuilder *builder, const char *title, const char *description, const char *date, const char *tags, const char *filename) {
    template_set_var(tpl, "title", title);
    template_set_var(tpl, "description", description);
    template_set_var(tpl, "site_title", builder->site_title);
    template_set_var(tpl, "date", date);
    template_set_var(tpl, "tags", tags);
    template_set_var(tpl, "filename", filename);
}

static char *format_date(const char *raw_date) {
    if (!raw_date || strlen(raw_date) < 11) {
        return strdup("");
    }

    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    int year = 0, month = 0, day = 0;

    if (sscanf(raw_date, "<%d-%d-%d", &year, &month, &day) == 3 &&
        year >= 2000 && month >= 1 && month <= 12 && day >= 1 && day <= 31) {
        char *date = malloc(DATE_BUFFER_SIZE);
        if (date) {
            snprintf(date, DATE_BUFFER_SIZE, "%02d %s %d", day, months[month - 1], year);
            return date;
        }
    }

    return strdup(raw_date);
}

static int add_post_to_builder(SiteBuilder *builder, const char *raw_date, const char *date, const char *title, const char *tags, const char *filename) {
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

static Template *load_base_template(SiteBuilder *builder) {
    char *template_path = join_path(builder->template_dir, "base.html");
    Template *tpl = template_create(template_path, builder->template_dir);
    free(template_path);
    return tpl;
}

static int write_html_file(const char *path, String *content, const char *name) {
    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "ERROR: Failed to open %s for writing\n", name);
        return 1;
    }

    char *output_str = string_to_cstr(content);
    fputs(output_str, f);
    free(output_str);
    fclose(f);

    printf("Generated: %s\n", name);
    return 0;
}

static void set_page_content(Template *tpl, String *content) {
    if (!tpl || !content) return;
    char *html_content = string_to_cstr(content);
    template_set_var(tpl, "content", html_content);
    free(html_content);
}

static int render_and_write_page(Template *tpl, String *content, const char *output_path, const char *output_name) {
    set_page_content(tpl, content);

    String *output = string_create(OUTPUT_BUFFER_SIZE);
    template_render(tpl, output);

    int result = write_html_file(output_path, output, output_name);
    string_free(output);
    return result;
}

static String *generate_tags_html(const char *tags) {
    String *tags_html = string_create(DEFAULT_STRING_BUFFER_SIZE);
    if (strlen(tags) == 0) return tags_html;

    char *tags_str = strdup(tags);
    char *token = strtok(tags_str, " ");
    while (token != NULL) {
        string_append_cstr(tags_html, "<a href=\"tags/");
        string_append_cstr(tags_html, token);
        string_append_cstr(tags_html, ".html\">");
        string_append_cstr(tags_html, token);
        string_append_cstr(tags_html, "</a>");
        token = strtok(NULL, " ");
        if (token) string_append_cstr(tags_html, ", ");
    }
    free(tags_str);
    return tags_html;
}

typedef struct {
    char *filename;
    char *formatted_date;
    char *content;
    OrgMetadata *meta;
    char *html;
    Template *base_tpl;
    Template *post_tpl;
} OrgFileResources;

static void free_org_file_resources(OrgFileResources *r, int free_post_tpl) {
    free(r->filename);
    free(r->formatted_date);
    free(r->content);
    if (r->meta) org_free_metadata(r->meta);
    if (r->html) org_free_string(r->html);
    if (r->base_tpl) template_free(r->base_tpl);
    if (free_post_tpl && r->post_tpl) template_free(r->post_tpl);
}

static int read_org_file(const char *path, char **out_content, size_t *out_size) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "ERROR: Failed to open %s\n", path);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    *out_content = malloc(file_size + 1);
    if (!*out_content) {
        fprintf(stderr, "ERROR: Failed to allocate memory for %s\n", path);
        fclose(f);
        return 1;
    }

    *out_size = fread(*out_content, 1, file_size, f);
    (*out_content)[*out_size] = '\0';
    fclose(f);
    return 0;
}

static int render_post_page(SiteBuilder *builder, OrgFileResources *r, const char *title, const char *description, const char *tags, const char *filename_only, const char *output_path) {
    r->post_tpl = template_create(join_path(builder->template_dir, "post.html"), builder->template_dir);
    if (!r->post_tpl) {
        fprintf(stderr, "ERROR: Failed to load post template\n");
        return 1;
    }

    String *tags_html = generate_tags_html(tags);

    char *toc = org_extract_toc(r->content, strlen(r->content));

    template_set_var(r->post_tpl, "date", r->formatted_date);
    template_set_var(r->post_tpl, "title", title);
    template_set_var(r->post_tpl, "filename", filename_only);
    template_set_var(r->post_tpl, "content", r->html);
    template_set_var(r->post_tpl, "tags", string_to_cstr(tags_html));
    template_set_var(r->post_tpl, "toc", toc ? toc : "");

    String *post_content = string_create(DEFAULT_STRING_BUFFER_SIZE);
    template_render(r->post_tpl, post_content);

    set_template_common_vars(r->base_tpl, builder, title, description, "", "", filename_only);
    template_set_var(r->base_tpl, "content", string_to_cstr(post_content));

    String *output = string_create(OUTPUT_BUFFER_SIZE);
    template_render(r->base_tpl, output);
    write_html_file(output_path, output, output_path);

    string_free(output);
    string_free(post_content);
    string_free(tags_html);
    if (toc) org_free_string(toc);
    return 0;
}

int process_org_file(SiteBuilder *builder, const char *input_path, const char *output_path) {
    OrgFileResources r = {0};
    size_t content_size;

    if (read_org_file(input_path, &r.content, &content_size) != 0) {
        return 1;
    }

    r.html = org_parse_to_html(r.content, content_size);
    if (!r.html) {
        fprintf(stderr, "ERROR: Failed to parse %s\n", input_path);
        free_org_file_resources(&r, 0);
        return 1;
    }

    r.meta = org_extract_metadata(r.content, content_size);
    if (!r.meta) {
        fprintf(stderr, "ERROR: Failed to extract metadata from %s\n", input_path);
        free_org_file_resources(&r, 0);
        return 1;
    }

    const char *title = org_meta_get_title(r.meta);
    title = title ? title : "Untitled";
    const char *description = org_meta_get_description(r.meta);
    description = description ? description : "";
    const char *raw_date = org_meta_get_date(r.meta);
    const char *tags = org_meta_get_tags(r.meta);
    tags = tags ? tags : "";

    r.formatted_date = raw_date ? format_date(raw_date) : strdup("");

    r.filename = get_filename_without_ext(output_path);
    char *filename_only = strrchr(r.filename, '/');
    filename_only = filename_only ? filename_only + 1 : r.filename;

    r.base_tpl = load_base_template(builder);
    if (!r.base_tpl) {
        fprintf(stderr, "ERROR: Failed to load template for %s\n", input_path);
        free_org_file_resources(&r, 0);
        return 1;
    }

    if (add_post_to_builder(builder, raw_date ? raw_date : "", r.formatted_date, title, tags, filename_only) != 0) {
        fprintf(stderr, "ERROR: Failed to add post to builder\n");
        free_org_file_resources(&r, 0);
        return 1;
    }

    int result = render_post_page(builder, &r, title, description, tags, filename_only, output_path);
    free_org_file_resources(&r, 1);
    return result;
}

static int process_regular_file(SiteBuilder *builder, const char *input_path, const char *output_dir, const char *filename) {
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

static void append_post_link(String *content, PostInfo *post) {
    string_append_cstr(content, "<h2 class=\"post-title\"><a href=\"");
    string_append_cstr(content, BLOG_BASE_URL);
    string_append_cstr(content, post->filename);
    string_append_cstr(content, ".html\">");
    string_append_cstr(content, post->title);
    string_append_cstr(content, "</a></h2><div class=\"post-date\">");
    string_append_cstr(content, post->date);
    string_append_cstr(content, "</div><div class=\"taglist\"><a href=\"");
    string_append_cstr(content, BLOG_BASE_URL);
    string_append_cstr(content, "tags.html\">Tags</a>: ");

    if (strlen(post->tags) > 0) {
        char *tags_str = strdup(post->tags);
        char *token = strtok(tags_str, " ");
        while (token != NULL) {
            string_append_cstr(content, "<a href=\"");
            string_append_cstr(content, BLOG_BASE_URL);
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

static int compare_posts(const void *a, const void *b) {
    const PostInfo *post_a = (const PostInfo *)a;
    const PostInfo *post_b = (const PostInfo *)b;
    return strcmp(post_b->raw_date, post_a->raw_date);
}

static void sort_posts(SiteBuilder *builder) {
    if (builder->post_count < 2) return;
    qsort(builder->posts, builder->post_count, sizeof(PostInfo), compare_posts);
}

static int generate_page_with_posts(SiteBuilder *builder, String *content, const char *title, const char *description, const char *filename, PostInfo *posts, int post_count) {
    for (int i = 0; i < post_count; i++) {
        append_post_link(content, &posts[i]);
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
        append_post_link(content, &builder->posts[i]);
    }

    char *template_path = join_path(builder->template_dir, "index.html");
    Template *tpl = template_create(template_path, builder->template_dir);
    free(template_path);

    if (!tpl) {
        fprintf(stderr, "ERROR: Failed to load index template\n");
        string_free(content);
        return 1;
    }

    set_template_common_vars(tpl, builder, builder->site_title, "Vandee's personal blog", "", "", "index.html");
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

static void ensure_tag_capacity(TagGroup *tag, int initial_cap) {
    if (tag->count >= tag->capacity) {
        int new_cap = tag->capacity == 0 ? initial_cap : tag->capacity * 2;
        PostInfo *new_posts = realloc(tag->posts, new_cap * sizeof(PostInfo));
        if (new_posts) {
            tag->posts = new_posts;
            tag->capacity = new_cap;
        }
    }
}

static TagGroup *find_or_create_tag(TagGroup **tags, int *tag_count, int *tag_capacity, const char *tag_name, int initial_cap) {
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

static void add_post_to_tag(TagGroup *tag, PostInfo *post) {
    if (!tag || !post) return;
    ensure_tag_capacity(tag, TAG_INITIAL_POST_CAPACITY);
    tag->posts[tag->count++] = *post;
}

static void process_post_tags(PostInfo *post, TagGroup **tags, int *tag_count, int *tag_capacity) {
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

static TagGroup *group_posts_by_tags(SiteBuilder *builder, int *tag_count_out) {
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

static void free_tag_groups(TagGroup *tags, int tag_count) {
    for (int i = 0; i < tag_count; i++) {
        free(tags[i].name);
        free(tags[i].posts);
    }
    free(tags);
}

static void append_tag_group_content(String *content, TagGroup *tag) {
    string_append_cstr(content, "<h1 class=\"tags-title\">Posts tagged \"");
    string_append_cstr(content, tag->name);
    string_append_cstr(content, "\":</h1>\n");
    for (int j = 0; j < tag->count; j++) {
        append_post_link(content, &tag->posts[j]);
    }
}

static String *generate_all_tags_content(SiteBuilder *builder, int *tag_count_out) {
    String *content = string_create(DEFAULT_STRING_BUFFER_SIZE);
    int tag_count;
    TagGroup *tags = group_posts_by_tags(builder, &tag_count);

    for (int i = 0; i < tag_count; i++) {
        append_tag_group_content(content, &tags[i]);
    }

    *tag_count_out = tag_count;
    free_tag_groups(tags, tag_count);
    return content;
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

static void generate_single_tag_page(SiteBuilder *builder, TagGroup *tag, const char *tag_dir) {
    String *content = string_create(DEFAULT_STRING_BUFFER_SIZE);
    append_tag_group_content(content, tag);

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

static int copy_file(const char *src, const char *dst) {
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

static int copy_directory_recursive(const char *src_dir, const char *dst_dir);

static int process_entry_copy(const char *src_dir, const char *dst_dir, const char *entry_name) {
    char *src_path = join_path(src_dir, entry_name);
    char *dst_path = join_path(dst_dir, entry_name);

    struct stat st;
    int result = 0;

    if (stat(src_path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            result = copy_directory_recursive(src_path, dst_path);
        } else if (S_ISREG(st.st_mode)) {
            result = copy_file(src_path, dst_path);
        }
    }

    free(src_path);
    free(dst_path);
    return result;
}

static int copy_directory_recursive(const char *src_dir, const char *dst_dir) {
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
        error_count += process_entry_copy(src_dir, dst_dir, entry->d_name);
    }

    closedir(dir);
    return error_count;
}
static int copy_custom_entry(SiteBuilder *builder, const char *custom_dir, const char *entry_name) {
    char *src_path = join_path(custom_dir, entry_name);
    char *dst_path = join_path(builder->output_dir, entry_name);

    struct stat st;
    int result = 0;

    if (stat(src_path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            result = copy_directory_recursive(src_path, dst_path);
            if (result == 0) {
                printf("  Copied directory: %s\n", entry_name);
            }
        } else {
            result = copy_file(src_path, dst_path);
            if (result == 0) {
                printf("  Copied file: %s\n", entry_name);
            }
        }
    }

    free(src_path);
    free(dst_path);
    return result;
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
        error_count += copy_custom_entry(builder, custom_dir, entry->d_name);
    }

    closedir(dir);
    free(custom_dir);

    return error_count;
}
