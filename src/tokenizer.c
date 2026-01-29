/*
 * ============================================================================
 * DEPRECATED
 * ============================================================================
 *
 * This file is deprecated and no longer used in the build system.
 *
 * The parsing and rendering functionality has been replaced by the Rust FFI layer.
 *
 * Please use:
 *   - ffi/src/lib.rs - Rust implementation (uses orgize library)
 *   - include/org-ffi.h - C API for the FFI layer
 *   - src/site-builder/org-parser.c - Wrapper for FFI parsing
 *
 * This file is kept for reference purposes only.
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tokenizer.h"

Tokenizer *tokenizer_create(const char *filename) {
    Tokenizer *t = malloc(sizeof(Tokenizer));
    if (!t) return NULL;

    t->filename = strdup(filename);
    if (!t->filename) {
        free(t);
        return NULL;
    }

    t->file = fopen(filename, "r");
    if (!t->file) {
        free(t->filename);
        free(t);
        return NULL;
    }

    t->line = 1;
    t->line_buf_cap = DEFAULT_LINE_BUFFER_SIZE;
    t->line_buffer = malloc(t->line_buf_cap);
    if (!t->line_buffer) {
        fclose(t->file);
        free(t->filename);
        free(t);
        return NULL;
    }

    return t;
}

static Token *create_token(TokenType type, Tokenizer *t) {
    Token *tok = malloc(sizeof(Token));
    if (!tok) return NULL;

    tok->type = type;
    tok->value = NULL;
    tok->level = 0;
    tok->line = t ? t->line : 0;
    tok->filename = t ? t->filename : NULL;
    return tok;
}

static int is_heading(const char *line, size_t len, int *level) {
    if (len == 0 || line[0] != '*') return 0;

    int lvl = 1;
    while (lvl < (int)len && line[lvl] == '*') lvl++;

    if (lvl <= 6 && (lvl == (int)len || line[lvl] == ' ')) {
        *level = lvl;
        return 1;
    }
    return 0;
}

static int is_code_block_start(const char *line, size_t len) {
    return len >= 11 && strncmp(line, "#+begin_src", 11) == 0;
}

static int is_code_block_end(const char *line, size_t len) {
    return len >= 9 && strncmp(line, "#+end_src", 9) == 0;
}

static int is_blockquote_start(const char *line, size_t len) {
    return len >= 13 && strncmp(line, "#+begin_quote", 13) == 0;
}

static int is_blockquote_end(const char *line, size_t len) {
    return len >= 10 && strncmp(line, "#+end_quote", 10) == 0;
}

static int is_metadata(const char *line, size_t len) {
    if (len <= 2 || line[0] != '#' || line[1] != '+') return 0;
    return strchr(line, ':') != NULL;
}

static int is_list_item(const char *line, size_t len) {
    size_t i = 0;
    while (i < len && line[i] == ' ') i++;

    if (i >= len) return 0;
    return line[i] == '-' || isdigit((unsigned char)line[i]);
}

static Token *create_token_with_value(TokenType type, Tokenizer *t, const char *value, size_t len) {
    Token *tok = create_token(type, t);
    if (!tok) return NULL;

    tok->value = string_create(len);
    string_append_cstr(tok->value, value);
    return tok;
}

Token *tokenizer_next_token(Tokenizer *t) {
    if (!t || !t->file) {
        return create_token(TOK_EOF, t);
    }

    char *line = fgets(t->line_buffer, t->line_buf_cap, t->file);
    if (!line) {
        return create_token(TOK_EOF, t);
    }

    size_t len = strlen(line);
    if (len > 0 && line[len-1] == '\n') {
        line[len-1] = '\0';
        len--;
    }
    if (len > 0 && line[len-1] == '\r') {
        line[len-1] = '\0';
        len--;
    }

    t->line++;

    Token *tok = create_token(TOK_TEXT, t);
    if (!tok) return NULL;

    if (len == 0) {
        tok->type = TOK_NEWLINE;
        tok->value = string_create(0);
        return tok;
    }

    int level;
    if (is_heading(line, len, &level)) {
        token_free(tok);
        tok = create_token(TOK_HEADING, t);
        if (!tok) return NULL;

        tok->level = level;
        const char *text = (level == (int)len) ? "" : line + level + 1;
        tok->value = string_create(len);
        string_append_cstr(tok->value, text);
        return tok;
    }

    if (is_code_block_start(line, len)) {
        token_free(tok);
        return create_token_with_value(TOK_CODE_BLOCK_START, t, line, len);
    }

    if (is_code_block_end(line, len)) {
        token_free(tok);
        return create_token_with_value(TOK_CODE_BLOCK_END, t, line, len);
    }

    if (is_blockquote_start(line, len)) {
        token_free(tok);
        return create_token_with_value(TOK_BLOCKQUOTE_START, t, line, len);
    }

    if (is_blockquote_end(line, len)) {
        token_free(tok);
        return create_token_with_value(TOK_BLOCKQUOTE_END, t, line, len);
    }

    if (is_metadata(line, len)) {
        token_free(tok);
        return create_token_with_value(TOK_METADATA, t, line, len);
    }

    if (is_list_item(line, len)) {
        token_free(tok);
        return create_token_with_value(TOK_LIST_ITEM, t, line, len);
    }

    token_free(tok);
    return create_token_with_value(TOK_TEXT, t, line, len);
}

void token_free(Token *tok) {
    if (tok) {
        if (tok->value) {
            string_free(tok->value);
        }
        free(tok);
    }
}

void tokenizer_free(Tokenizer *t) {
    if (t) {
        if (t->file) {
            fclose(t->file);
        }
        if (t->line_buffer) {
            free(t->line_buffer);
        }
        free((void*)t->filename);
        free(t);
    }
}
