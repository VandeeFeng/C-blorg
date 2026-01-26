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
