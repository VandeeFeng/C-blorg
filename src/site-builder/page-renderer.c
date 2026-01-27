#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "site-builder/page-renderer.h"
#include "site-builder.h"
#include "site-builder/filesystem.h"
#include "template.h"
#include "org-string.h"

void set_template_common_vars(Template *tpl, SiteBuilder *builder, const char *title, const char *description, const char *date, const char *tags, const char *filename) {
    template_set_var(tpl, "title", title);
    template_set_var(tpl, "description", description);
    template_set_var(tpl, "site_title", builder->site_title);
    template_set_var(tpl, "blog_base_url", builder->blog_base_url);
    template_set_var(tpl, "date", date);
    template_set_var(tpl, "tags", tags);
    template_set_var(tpl, "filename", filename);
}

Template *load_base_template(SiteBuilder *builder) {
    char *template_path = join_path(builder->template_dir, "base.html");
    Template *tpl = template_create(template_path, builder->template_dir);
    free(template_path);
    return tpl;
}

int write_html_file(const char *path, String *content, const char *name) {
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

void set_page_content(Template *tpl, String *content) {
    if (!tpl || !content) return;
    char *html_content = string_to_cstr(content);
    template_set_var(tpl, "content", html_content);
    free(html_content);
}

int render_and_write_page(Template *tpl, String *content, const char *output_path, const char *output_name) {
    set_page_content(tpl, content);

    String *output = string_create(OUTPUT_BUFFER_SIZE);
    template_render(tpl, output);

    int result = write_html_file(output_path, output, output_name);
    string_free(output);
    return result;
}
