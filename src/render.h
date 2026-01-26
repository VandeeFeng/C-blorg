#ifndef RENDER_H
#define RENDER_H

#include "parser.h"
#include "org-string.h"

void render_node_to_html(Node *node, String *output);
void render_document_content(Node *doc, String *output);
void render_heading(Node *heading, String *output);
void render_paragraph(Node *paragraph, String *output);
void render_code_block(Node *code, String *output);
void render_blockquote(Node *quote, String *output);
void render_list(Node *list, String *output);
void render_metadata(Node *meta, String *output);
void render_text(Node *text, String *output);
void render_image(Node *image, String *output);

#endif
