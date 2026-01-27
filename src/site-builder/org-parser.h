#ifndef ORG_PARSER_H
#define ORG_PARSER_H

#include "site-builder.h"
#include "org-ffi.h"
#include "org-string.h"
#include "template.h"

typedef struct {
    char *filename;
    char *formatted_date;
    char *content;
    OrgMetadata *meta;
    char *html;
    Template *base_tpl;
    Template *post_tpl;
} OrgFileResources;

int read_org_file(const char *path, char **out_content, size_t *out_size);
void free_org_file_resources(OrgFileResources *r, int free_post_tpl);
char *format_date(const char *raw_date);
String *generate_tags_html(const char *tags);
int render_post_page(SiteBuilder *builder, OrgFileResources *r, const char *title, const char *description, const char *tags, const char *filename_only, const char *output_path);
int process_org_file(SiteBuilder *builder, const char *input_path, const char *output_path);

#endif
