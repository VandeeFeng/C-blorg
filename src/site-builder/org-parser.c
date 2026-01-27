#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "site-builder/org-parser.h"
#include "site-builder.h"
#include "site-builder/page-renderer.h"
#include "site-builder/filesystem.h"
#include "site-builder/post-management.h"
#include "org-ffi.h"
#include "org-string.h"

int read_org_file(const char *path, char **out_content, size_t *out_size) {
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

void free_org_file_resources(OrgFileResources *r, int free_post_tpl) {
    free(r->filename);
    free(r->formatted_date);
    free(r->content);
    if (r->meta) org_free_metadata(r->meta);
    if (r->html) org_free_string(r->html);
    if (r->base_tpl) template_free(r->base_tpl);
    if (free_post_tpl && r->post_tpl) template_free(r->post_tpl);
}

char *format_date(const char *raw_date) {
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

String *generate_tags_html(const char *tags) {
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

int render_post_page(SiteBuilder *builder, OrgFileResources *r, const char *title, const char *description, const char *tags, const char *filename_only, const char *output_path) {
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

    int result = render_post_page(builder, &r, title, description, tags, filename_only, output_path);
    free_org_file_resources(&r, 1);
    return result;
}
