#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "org-string.h"

/* Use constants from site-builder.h */
#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 512
#endif

#ifndef DEFAULT_LINE_BUFFER_SIZE
#define DEFAULT_LINE_BUFFER_SIZE 1024
#endif

typedef enum {
    TOK_EOF,
    TOK_HEADING,
    TOK_METADATA,
    TOK_TEXT,
    TOK_CODE_BLOCK_START,
    TOK_CODE_BLOCK_END,
    TOK_BLOCKQUOTE_START,
    TOK_BLOCKQUOTE_END,
    TOK_LIST_ITEM,
    TOK_LINK,
    TOK_BOLD,
    TOK_ITALIC,
    TOK_CODE_INLINE,
    TOK_TABLE_ROW,
    TOK_NEWLINE
} TokenType;

typedef struct {
    TokenType type;
    String *value;
    int level;
    char *filename;
    int line;
} Token;

typedef struct {
    char *filename;
    FILE *file;
    int line;
    char *line_buffer;
    size_t line_buf_cap;
} Tokenizer;

Tokenizer *tokenizer_create(const char *filename);
Token *tokenizer_next_token(Tokenizer *t);
void token_free(Token *tok);
void tokenizer_free(Tokenizer *t);

#endif
