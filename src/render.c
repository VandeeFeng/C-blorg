#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "render.h"

static void append_escaped(String *output, const char *str) {
    for (size_t i = 0; str[i]; i++) {
        switch (str[i]) {
            case '<': string_append_cstr(output, "&lt;"); break;
            case '>': string_append_cstr(output, "&gt;"); break;
            case '&': string_append_cstr(output, "&amp;"); break;
            case '"': string_append_cstr(output, "&quot;"); break;
            case '\'': string_append_cstr(output, "&#39;"); break;
            default: string_append(output, &str[i], 1);
        }
    }
}

static void render_value_escaped(Node *node, String *output) {
    if (!node || !node->value || !output) return;

    char *cstr = string_to_cstr(node->value);
    append_escaped(output, cstr);
    free(cstr);
}

static void render_tag_with_children(Node *node, String *output, const char *open_tag, const char *close_tag) {
    if (!node || !output) return;

    string_append_cstr(output, open_tag);

    if (node->value) {
        render_value_escaped(node, output);
    }

    for (int i = 0; i < node->child_count; i++) {
        render_node_to_html(node->children[i], output);
    }

    string_append_cstr(output, close_tag);
}

void render_node_to_html(Node *node, String *output) {
    if (!node || !output) return;

    switch (node->type) {
        case NODE_DOCUMENT:
            for (int i = 0; i < node->child_count; i++) {
                render_node_to_html(node->children[i], output);
            }
            break;
        case NODE_HEADING:
            render_heading(node, output);
            break;
        case NODE_PARAGRAPH:
            render_paragraph(node, output);
            break;
        case NODE_CODE_BLOCK:
            render_code_block(node, output);
            break;
        case NODE_BLOCKQUOTE:
            render_blockquote(node, output);
            break;
        case NODE_LIST:
            render_list(node, output);
            break;
        case NODE_METADATA:
            render_metadata(node, output);
            break;
        case NODE_TEXT:
            render_text(node, output);
            break;
        case NODE_LINK:
            string_append_cstr(output, "<a href=\"");
            render_value_escaped(node, output);
            string_append_cstr(output, "\">");
            for (int i = 0; i < node->child_count; i++) {
                render_node_to_html(node->children[i], output);
            }
            string_append_cstr(output, "</a>");
            break;
        case NODE_IMAGE:
            render_image(node, output);
            break;
        case NODE_LIST_ITEM:
            render_tag_with_children(node, output, "<li>", "</li>");
            break;
        default:
            break;
    }
}

void render_document_content(Node *doc, String *output) {
    if (!doc || !output) return;
    if (doc->type != NODE_DOCUMENT) {
        render_node_to_html(doc, output);
        return;
    }

    for (int i = 0; i < doc->child_count; i++) {
        if (doc->children[i]->type != NODE_METADATA) {
            render_node_to_html(doc->children[i], output);
        }
    }
}

void render_heading(Node *heading, String *output) {
    if (!heading || !output) return;

    int level = heading->level + 1;
    if (level < 1) level = 1;
    if (level > 6) level = 6;

    char tag[3] = {'h', '0' + level, '\0'};
    string_append_cstr(output, "<");
    string_append_cstr(output, tag);
    string_append_cstr(output, ">");
    render_value_escaped(heading, output);
    string_append_cstr(output, "</");
    string_append_cstr(output, tag);
    string_append_cstr(output, ">\n");
}

void render_paragraph(Node *paragraph, String *output) {
    if (!paragraph || !output) return;

    string_append_cstr(output, "<p>");

    if (paragraph->child_count == 0) {
        render_value_escaped(paragraph, output);
    } else {
        for (int i = 0; i < paragraph->child_count; i++) {
            render_node_to_html(paragraph->children[i], output);
        }
    }

    string_append_cstr(output, "</p>\n");
}

void render_code_block(Node *code, String *output) {
    if (!code || !output) return;

    string_append_cstr(output, "<pre><code");

    if (code->language) {
        string_append_cstr(output, " class=\"language-");
        string_append_cstr(output, code->language);
        string_append_cstr(output, "\"");
    }

    string_append_cstr(output, ">");
    render_value_escaped(code, output);
    string_append_cstr(output, "</code></pre>\n");
}

void render_blockquote(Node *quote, String *output) {
    if (!quote || !output) return;

    render_tag_with_children(quote, output, "<blockquote>", "</blockquote>\n");
}

void render_list(Node *list, String *output) {
    if (!list || !output) return;

    render_tag_with_children(list, output, "<ul>", "</ul>\n");
}

void render_metadata(Node *meta, String *output) {
    (void)meta;
    (void)output;
}

void render_text(Node *text, String *output) {
    if (!text || !output) return;
    render_value_escaped(text, output);
}

void render_image(Node *image, String *output) {
    if (!image || !output) return;

    string_append_cstr(output, "<img src=\"");

    if (image->value) {
        char *src = string_to_cstr(image->value);
        const char *to_render = (strncmp(src, "file:", 5) == 0) ? src + 5 : src;
        append_escaped(output, to_render);
        free(src);
    }

    string_append_cstr(output, "\">");
}
