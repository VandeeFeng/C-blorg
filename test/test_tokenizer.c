#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/tokenizer.h"

void create_test_file(const char *filename, const char *content) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Failed to create test file: %s\n", filename);
        exit(1);
    }
    fputs(content, f);
    fclose(f);
}

void test_heading_token() {
    printf("Testing heading tokens...\n");

    create_test_file("test_heading.org",
        "* H1 Heading\n"
        "** H2 Heading\n"
        "*** H3 Heading\n"
        "**** H4 Heading\n"
    );

    Tokenizer *t = tokenizer_create("test_heading.org");
    assert(t != NULL);

    Token *tok = tokenizer_next_token(t);
    assert(tok->type == TOK_HEADING);
    assert(tok->level == 1);
    assert(strcmp(tok->value->data, "H1 Heading") == 0);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_HEADING);
    assert(tok->level == 2);
    assert(strcmp(tok->value->data, "H2 Heading") == 0);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_HEADING);
    assert(tok->level == 3);
    assert(strcmp(tok->value->data, "H3 Heading") == 0);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_HEADING);
    assert(tok->level == 4);
    assert(strcmp(tok->value->data, "H4 Heading") == 0);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_EOF);
    token_free(tok);

    tokenizer_free(t);
    remove("test_heading.org");

    printf("  ✓ heading tokens passed\n");
}

void test_metadata_token() {
    printf("Testing metadata tokens...\n");

    create_test_file("test_metadata.org",
        "#+title: My Post\n"
        "#+date: <2024-01-24 10:00>\n"
        "#+description: A test post\n"
        "#+filetags: Blog Emacs\n"
    );

    Tokenizer *t = tokenizer_create("test_metadata.org");
    assert(t != NULL);

    Token *tok = tokenizer_next_token(t);
    assert(tok->type == TOK_METADATA);
    assert(strncmp(tok->value->data, "#+title:", 8) == 0);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_METADATA);
    assert(strncmp(tok->value->data, "#+date:", 7) == 0);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_METADATA);
    assert(strncmp(tok->value->data, "#+description:", 13) == 0);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_METADATA);
    assert(strncmp(tok->value->data, "#+filetags:", 11) == 0);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_EOF);
    token_free(tok);

    tokenizer_free(t);
    remove("test_metadata.org");

    printf("  ✓ metadata tokens passed\n");
}

void test_code_block_tokens() {
    printf("Testing code block tokens...\n");

    create_test_file("test_code.org",
        "Here is some text\n"
        "#+begin_src python\n"
        "def hello():\n"
        "    print('hello')\n"
        "#+end_src\n"
        "More text\n"
    );

    Tokenizer *t = tokenizer_create("test_code.org");
    assert(t != NULL);

    Token *tok = tokenizer_next_token(t);
    assert(tok->type == TOK_TEXT);
    assert(strcmp(tok->value->data, "Here is some text") == 0);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_CODE_BLOCK_START);
    assert(strncmp(tok->value->data, "#+begin_src", 11) == 0);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_TEXT);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_TEXT);
    token_free(tok);

    tok = tokenizer_next_token(t);
    printf("Token before CODE_BLOCK_END: type=%d, value='%s'\n", tok->type, tok->value->data);
    assert(tok->type == TOK_CODE_BLOCK_END);
    assert(strncmp(tok->value->data, "#+end_src", 9) == 0);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_TEXT);
    assert(strcmp(tok->value->data, "More text") == 0);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_EOF);
    token_free(tok);

    tokenizer_free(t);
    remove("test_code.org");

    printf("  ✓ code block tokens passed\n");
}

void test_blockquote_tokens() {
    printf("Testing blockquote tokens...\n");

    create_test_file("test_quote.org",
        "Normal text\n"
        "#+begin_quote\n"
        "Quoted content\n"
        "#+end_quote\n"
        "Back to normal\n"
    );

    Tokenizer *t = tokenizer_create("test_quote.org");
    assert(t != NULL);

    Token *tok = tokenizer_next_token(t);
    assert(tok->type == TOK_TEXT);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_BLOCKQUOTE_START);
    assert(strncmp(tok->value->data, "#+begin_quote", 13) == 0);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_TEXT);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_BLOCKQUOTE_END);
    assert(strncmp(tok->value->data, "#+end_quote", 10) == 0);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_TEXT);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_EOF);
    token_free(tok);

    tokenizer_free(t);
    remove("test_quote.org");

    printf("  ✓ blockquote tokens passed\n");
}

void test_list_item_tokens() {
    printf("Testing list item tokens...\n");

    create_test_file("test_list.org",
        "- Unordered item 1\n"
        "- Unordered item 2\n"
        "  - Nested item\n"
        "1. Ordered item 1\n"
        "2. Ordered item 2\n"
    );

    Tokenizer *t = tokenizer_create("test_list.org");
    assert(t != NULL);

    Token *tok = tokenizer_next_token(t);
    assert(tok->type == TOK_LIST_ITEM);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_LIST_ITEM);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_LIST_ITEM);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_LIST_ITEM);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_LIST_ITEM);
    token_free(tok);

    tok = tokenizer_next_token(t);
    assert(tok->type == TOK_EOF);
    token_free(tok);

    tokenizer_free(t);
    remove("test_list.org");

    printf("  ✓ list item tokens passed\n");
}

void test_real_org_file() {
    printf("Testing real org file...\n");

    Tokenizer *t = tokenizer_create("../posts/2024-01-20-2024-emacs-config.org");
    if (!t) {
        printf("  ! Skipping real file test (file not found)\n");
        return;
    }

    int count = 0;
    Token *tok;
    while ((tok = tokenizer_next_token(t))->type != TOK_EOF) {
        if (count < 5) {
            printf("  Token %d: type=%d, line=%d\n", count, tok->type, tok->line);
        }
        token_free(tok);
        count++;
    }
    token_free(tok);
    tokenizer_free(t);

    printf("  ✓ Parsed %d tokens from real org file\n", count);
}

int main() {
    printf("=== Tokenizer Tests ===\n\n");

    test_heading_token();
    test_metadata_token();
    test_code_block_tokens();
    test_blockquote_tokens();
    test_list_item_tokens();
    test_real_org_file();

    printf("\n✓ All tokenizer tests passed!\n");
    return 0;
}
