#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "org-string.h"

/* Constants */
#define INITIAL_CHILD_CAPACITY 4
#define DEFAULT_STRING_CAPACITY 1024

/* Image extensions */
static const char *IMAGE_EXTENSIONS[] = {
    ".png", ".jpg", ".jpeg", ".gif", ".svg", ".webp", ".bmp", ".tiff", NULL
};

Parser *parser_create(Tokenizer *tokenizer) {
    Parser *p = malloc(sizeof(Parser));
    if (!p) return NULL;

    p->tokenizer = tokenizer;
    p->peek_token = tokenizer_next_token(tokenizer);
    return p;
}

void parser_free(Parser *p) {
    if (p) {
        if (p->peek_token) {
            token_free(p->peek_token);
        }
        free(p);
    }
}

Node *node_create(NodeType type) {
    Node *node = malloc(sizeof(Node));
    if (!node) return NULL;

    node->type = type;
    node->value = NULL;
    node->level = 0;
    node->key = NULL;
    node->language = NULL;
    node->children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;
    node->parent = NULL;
    return node;
}

void node_add_child(Node *parent, Node *child) {
    if (!parent || !child) return;

    if (parent->child_count >= parent->child_capacity) {
        int new_cap = parent->child_capacity == 0 ? INITIAL_CHILD_CAPACITY : parent->child_capacity * 2;
        Node **new_children = realloc(parent->children, new_cap * sizeof(Node*));
        if (!new_children) return;

        parent->children = new_children;
        parent->child_capacity = new_cap;
    }

    parent->children[parent->child_count] = child;
    child->parent = parent;
    parent->child_count++;
}

void node_free(Node *node) {
    if (!node) return;

    if (node->value) {
        string_free(node->value);
    }

    if (node->key) {
        free(node->key);
    }

    if (node->language) {
        free(node->language);
    }

    for (int i = 0; i < node->child_count; i++) {
        node_free(node->children[i]);
    }

    if (node->children) {
        free(node->children);
    }

    free(node);
}

static Token *parser_peek(Parser *p) {
    return p->peek_token;
}

static Token *parser_next(Parser *p) {
    Token *current = p->peek_token;
    p->peek_token = tokenizer_next_token(p->tokenizer);
    return current;
}

static void collect_text_tokens(Parser *p, String *output) {
    Token *next_tok;
    while ((next_tok = parser_peek(p))->type == TOK_TEXT) {
        if (output->len > 0) {
            string_append_cstr(output, " ");
        }
        string_append_cstr(output, next_tok->value->data);
        token_free(parser_next(p));
    }
}

static char *skip_whitespace(char *str) {
    while (*str == ' ') str++;
    return str;
}

static char *get_colon_value(char *line) {
    char *colon = strchr(line, ':');
    if (!colon) return NULL;
    return skip_whitespace(colon + 1);
}

static char *get_metadata_key(char *line) {
    char *key_start = skip_whitespace(line);
    if (key_start[0] == '#' && key_start[1] == '+') {
        key_start += 2;
    }
    key_start = skip_whitespace(key_start);

    char *colon = strchr(key_start, ':');
    if (!colon) return NULL;

    size_t key_len = colon - key_start;
    char *key = malloc(key_len + 1);
    if (key) {
        strncpy(key, key_start, key_len);
        key[key_len] = '\0';
    }
    return key;
}

static char *collapse_whitespace(const char *path) {
    if (!path) return NULL;

    size_t len = strlen(path);
    char *collapsed = malloc(len + 1);
    if (!collapsed) return NULL;

    size_t j = 0;
    int in_whitespace = 0;
    for (size_t i = 0; i < len; i++) {
        char c = path[i];
        if (c == ' ' || c == '\t' || c == '\n') {
            if (!in_whitespace) {
                collapsed[j++] = ' ';
                in_whitespace = 1;
            }
        } else {
            collapsed[j++] = c;
            in_whitespace = 0;
        }
    }

    if (j > 0 && collapsed[j-1] == ' ') {
        j--;
    }
    collapsed[j] = '\0';

    return collapsed;
}

static int is_image_path(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return 0;

    for (int i = 0; IMAGE_EXTENSIONS[i] != NULL; i++) {
        if (strncasecmp(ext, IMAGE_EXTENSIONS[i], strlen(IMAGE_EXTENSIONS[i])) == 0) {
            return 1;
        }
    }
    return 0;
}

static const char *determine_link_type(const char *path) {
    if (strncmp(path, "https:", 6) == 0) return "https";
    if (strncmp(path, "http:", 5) == 0) return "http";
    if (strncmp(path, "file:", 5) == 0) return "file";
    if (strncmp(path, "id:", 3) == 0) return "id";
    if (path[0] == '#') return "custom-id";
    if (path[0] == '/' || path[0] == '.' || path[0] == '~') return "file";

    return "fuzzy";
}

static Node *parse_heading(Parser *p, Token *tok) {
    Node *node = node_create(NODE_HEADING);
    if (!node) {
        token_free(tok);
        return NULL;
    }

    node->level = tok->level;
    node->value = string_create(tok->value->len);
    string_append_cstr(node->value, tok->value->data);
    token_free(tok);

    return node;
}

static Node *parse_metadata(Parser *p, Token *tok) {
    (void)p;
    Node *node = node_create(NODE_METADATA);
    if (!node) {
        token_free(tok);
        return NULL;
    }

    node->key = get_metadata_key(tok->value->data);
    char *value = get_colon_value(tok->value->data);

    if (value) {
        node->value = string_create(strlen(value));
        string_append_cstr(node->value, value);
    }

    token_free(tok);
    return node;
}

static Node *parse_code_block(Parser *p, Token *tok) {
    Node *node = node_create(NODE_CODE_BLOCK);
    if (!node) {
        token_free(tok);
        return NULL;
    }

    char *space = strchr(tok->value->data, ' ');
    if (space) {
        char *lang = skip_whitespace(space + 1);
        if (*lang != '\0') {
            node->language = strdup(lang);
        }
    }
    token_free(tok);

    String *code = string_create(DEFAULT_STRING_CAPACITY);
    Token *next_tok = parser_next(p);
    while (next_tok->type != TOK_CODE_BLOCK_END && next_tok->type != TOK_EOF) {
        if (next_tok->type != TOK_NEWLINE) {
            string_append_cstr(code, next_tok->value->data);
            string_append_cstr(code, "\n");
        }
        token_free(next_tok);
        next_tok = parser_next(p);
    }
    token_free(next_tok);

    node->value = code;
    return node;
}

static Node *parse_blockquote(Parser *p) {
    Node *node = node_create(NODE_BLOCKQUOTE);
    if (!node) return NULL;

    Token *tok = parser_next(p);
    token_free(tok);

    Token *next_tok = parser_next(p);
    while (next_tok->type != TOK_BLOCKQUOTE_END && next_tok->type != TOK_EOF) {
        if (next_tok->type == TOK_TEXT || next_tok->type == TOK_NEWLINE) {
            if (next_tok->type == TOK_TEXT && node->child_count > 0) {
                Node *space_node = node_create(NODE_TEXT);
                space_node->value = string_create(1);
                string_append_cstr(space_node->value, " ");
                node_add_child(node, space_node);
            }
            if (next_tok->type == TOK_TEXT) {
                Node *text_node = node_create(NODE_TEXT);
                text_node->value = string_create(next_tok->value->len);
                string_append_cstr(text_node->value, next_tok->value->data);
                node_add_child(node, text_node);
            }
        }
        token_free(next_tok);
        next_tok = parser_next(p);
    }
    token_free(next_tok);

    return node;
}

static Node *parse_list(Parser *p) {
    Node *list_node = node_create(NODE_LIST);
    if (!list_node) return NULL;

    Token *tok;
    while ((tok = parser_peek(p))->type == TOK_LIST_ITEM || tok->type == TOK_NEWLINE) {
        if (tok->type == TOK_LIST_ITEM) {
            Node *item_node = node_create(NODE_LIST_ITEM);

            char *value_start = tok->value->data;
            size_t i = 0;
            while (i < tok->value->len && (value_start[i] == ' ' || value_start[i] == '-' || value_start[i] == '.')) i++;
            while (i < tok->value->len && value_start[i] == ' ') i++;

            if (i < tok->value->len) {
                item_node->value = string_create(tok->value->len - i);
                string_append(item_node->value, value_start + i, tok->value->len - i);
            } else {
                item_node->value = string_create(0);
            }

            node_add_child(list_node, item_node);
        }
        token_free(parser_next(p));
    }

    return list_node;
}

static Node *create_text_node(const char *text, size_t len) {
    Node *text_node = node_create(NODE_TEXT);
    if (text_node) {
        text_node->value = string_create(len);
        string_append(text_node->value, text, len);
    }
    return text_node;
}

static void add_text_node(Node *parent, const char *text, size_t len) {
    if (!parent) return;

    Node *text_node = create_text_node(text, len);
    if (text_node) {
        node_add_child(parent, text_node);
    }
}

static char *extract_link_path(const char *link_start, const char *path_end) {
    size_t path_len = path_end - link_start;
    char *path = strndup(link_start, path_len);
    return collapse_whitespace(path);
}

static char *extract_link_description(const char *path_end) {
    if (path_end[1] != '[') return NULL;

    const char *desc_start = path_end + 2;
    const char *desc_end = strchr(desc_start, ']');
    if (!desc_end) return NULL;

    return strndup(desc_start, desc_end - desc_start);
}

static Node *create_link_node(const char *path, const char *description) {
    const char *link_type = determine_link_type(path);
    int is_image = is_image_path(path);

    NodeType node_type = (is_image && strcmp(link_type, "file") == 0) ? NODE_IMAGE : NODE_LINK;
    Node *inline_node = node_create(node_type);
    if (!inline_node) return NULL;

    inline_node->value = string_create(strlen(path));
    string_append_cstr(inline_node->value, path);

    if (description) {
        Node *desc_node = create_text_node(description, strlen(description));
        if (desc_node) {
            node_add_child(inline_node, desc_node);
        }
    }

    return inline_node;
}

static const char *parse_org_link(const char *pos, Node *paragraph, const char **start) {
    const char *link_start = pos + 2;
    const char *path_end = strchr(link_start, ']');
    if (!path_end) return pos + 1;

    char *description = extract_link_description(path_end);
    const char *desc_end = description ? strchr(path_end + 2, ']') : NULL;
    char *path = extract_link_path(link_start, path_end);

    if (!path) {
        free(description);
        return desc_end ? desc_end + 2 : path_end + 2;
    }

    Node *inline_node = create_link_node(path, description);
    if (inline_node) {
        node_add_child(paragraph, inline_node);
        *start = desc_end ? desc_end + 2 : path_end + 2;
    } else {
        *start = pos + 1;
    }

    free(path);
    free(description);
    return *start;
}

static void parse_inline_elements(Parser *p, Node *paragraph, const char *text) {
    (void)p;
    if (!text || *text == '\0') {
        add_text_node(paragraph, "", 0);
        return;
    }

    const char *start = text;
    const char *pos = text;

    while (*pos) {
        if (pos[0] == '[' && pos[1] == '[') {
            if (start < pos) {
                size_t text_len = pos - start;
                add_text_node(paragraph, start, text_len);
            }
            pos = parse_org_link(pos, paragraph, &start);
        } else {
            pos++;
        }
    }

    if (start < pos) {
        size_t text_len = pos - start;
        add_text_node(paragraph, start, text_len);
    }
}

static Node *parse_paragraph(Parser *p, Token *tok) {
    Node *node = node_create(NODE_PARAGRAPH);
    if (!node) {
        token_free(tok);
        return NULL;
    }

    String *text = string_create(tok->value->len + DEFAULT_STRING_CAPACITY);
    string_append_cstr(text, tok->value->data);
    token_free(tok);

    collect_text_tokens(p, text);

    char *text_cstr = string_to_cstr(text);
    parse_inline_elements(p, node, text_cstr);
    free(text_cstr);

    string_free(text);

    return node;
}

Node *parser_parse(Parser *p) {
    Node *document = node_create(NODE_DOCUMENT);
    if (!document) return NULL;

    Token *tok;
    while ((tok = parser_peek(p))->type != TOK_EOF) {
        Node *node = NULL;

        switch (tok->type) {
            case TOK_HEADING:
                node = parse_heading(p, parser_next(p));
                break;
            case TOK_METADATA:
                node = parse_metadata(p, parser_next(p));
                break;
            case TOK_CODE_BLOCK_START:
                node = parse_code_block(p, parser_next(p));
                break;
            case TOK_BLOCKQUOTE_START:
                node = parse_blockquote(p);
                break;
            case TOK_LIST_ITEM:
                node = parse_list(p);
                break;
            case TOK_TEXT:
                node = parse_paragraph(p, parser_next(p));
                break;
            case TOK_NEWLINE:
                token_free(parser_next(p));
                break;
            default:
                token_free(parser_next(p));
                break;
        }

        if (node) {
            node_add_child(document, node);
        }
    }

    return document;
}
