#ifndef TEMPLATE_H
#define TEMPLATE_H

#include "org-string.h"

typedef struct {
    char *key;
    char *value;
} TemplateVar;

typedef struct {
    String *content;
    TemplateVar *vars;
    int var_count;
    int var_capacity;
} Template;

Template *template_create(const char *filename, const char *template_dir);
void template_free(Template *t);
void template_set_var(Template *t, const char *key, const char *value);
void template_render(Template *t, String *output);

#endif
