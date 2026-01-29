/*
 * ============================================================================
 * DEPRECATED
 * ============================================================================
 *
 * This header is deprecated and no longer used in the build system.
 *
 * The parsing functionality has been replaced by the Rust FFI layer.
 *
 * Please use:
 *   - ffi/src/lib.rs - Rust implementation (uses orgize library)
 *   - include/org-ffi.h - C API for the FFI layer
 *   - src/site-builder/org-parser.c - Wrapper for FFI parsing
 *
 * This file is kept for reference purposes only.
 */

#ifndef PARSER_H
#define PARSER_H

#include "tokenizer.h"

typedef enum {
    NODE_DOCUMENT,
    NODE_HEADING,
    NODE_METADATA,
    NODE_PARAGRAPH,
    NODE_CODE_BLOCK,
    NODE_BLOCKQUOTE,
    NODE_LIST,
    NODE_LIST_ITEM,
    NODE_TEXT,
    NODE_LINK,
    NODE_IMAGE
} NodeType;

typedef struct Node Node;

struct Node {
    NodeType type;
    String *value;
    int level;
    char *key;
    char *language;
    Node **children;
    int child_count;
    int child_capacity;
    Node *parent;
};

typedef struct {
    Tokenizer *tokenizer;
    Token *peek_token;
} Parser;

Parser *parser_create(Tokenizer *tokenizer);
void parser_free(Parser *parser);
Node *parser_parse(Parser *parser);
Node *node_create(NodeType type);
void node_add_child(Node *parent, Node *child);
void node_free(Node *node);

#endif
