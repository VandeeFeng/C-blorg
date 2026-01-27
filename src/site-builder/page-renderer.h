#ifndef PAGE_RENDERER_H
#define PAGE_RENDERER_H

#include "template.h"
#include "site-builder.h"
#include "org-string.h"

void set_template_common_vars(Template *tpl, SiteBuilder *builder, const char *title, const char *description, const char *date, const char *tags, const char *filename);
Template *load_base_template(SiteBuilder *builder);
void set_page_content(Template *tpl, String *content);
int render_and_write_page(Template *tpl, String *content, const char *output_path, const char *output_name);
int write_html_file(const char *path, String *content, const char *name);

#endif
