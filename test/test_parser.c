#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "parser.h"
#include "org-string.h"

void create_test_file(const char *filename, const char *content) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Failed to create test file: %s\n", filename);
        exit(1);
    }
    fputs(content, f);
    fclose(f);
}

void test_simple_document() {
    printf("Testing simple document parsing...\n");

    create_test_file("test_simple.org",
        "#+title: Test Document\n"
        "#+date: 2024-01-01\n"
        "\n"
        "* Heading 1\n"
        "\n"
        "This is a paragraph.\n"
        "\n"
        "** Heading 2\n"
        "\n"
        "Another paragraph.\n"
    );

    Tokenizer *t = tokenizer_create("test_simple.org");
    assert(t != NULL);

    Parser *p = parser_create(t);
    assert(p != NULL);

    Node *doc = parser_parse(p);
    assert(doc != NULL);
    assert(doc->type == NODE_DOCUMENT);

    printf("  Document has %d children\n", doc->child_count);

    int metadata_count = 0;
    int heading_count = 0;
    int paragraph_count = 0;

    for (int i = 0; i < doc->child_count; i++) {
        Node *child = doc->children[i];
        if (child->type == NODE_METADATA) {
            metadata_count++;
            printf("  Metadata: key=%s, value=%s\n", child->key, child->value->data);
        } else if (child->type == NODE_HEADING) {
            heading_count++;
            printf("  Heading: level=%d, text=%s\n", child->level, child->value->data);
        } else if (child->type == NODE_PARAGRAPH) {
            paragraph_count++;
            printf("  Paragraph with %d children\n", child->child_count);
        }
    }

    assert(metadata_count == 2);
    assert(heading_count == 2);
    assert(paragraph_count == 2);

    node_free(doc);
    parser_free(p);
    tokenizer_free(t);
    remove("test_simple.org");

    printf("  ✓ simple document parsing passed\n");
}

void test_code_blocks() {
    printf("Testing code block parsing...\n");

    create_test_file("test_code.org",
        "#+title: Code Test\n"
        "\n"
        "* Example\n"
        "\n"
        "#+begin_src c\n"
        "int main() {\n"
        "    return 0;\n"
        "}\n"
        "#+end_src\n"
    );

    Tokenizer *t = tokenizer_create("test_code.org");
    assert(t != NULL);

    Parser *p = parser_create(t);
    assert(p != NULL);

    Node *doc = parser_parse(p);
    assert(doc != NULL);
    assert(doc->type == NODE_DOCUMENT);

    Node *code_node = NULL;
    for (int i = 0; i < doc->child_count; i++) {
        if (doc->children[i]->type == NODE_CODE_BLOCK) {
            code_node = doc->children[i];
            break;
        }
    }

    assert(code_node != NULL);
    assert(code_node->language != NULL);
    assert(strcmp(code_node->language, "c") == 0);

    printf("  Code block language: %s\n", code_node->language);
    printf("  Code content:\n%s\n", code_node->value->data);

    node_free(doc);
    parser_free(p);
    tokenizer_free(t);
    remove("test_code.org");

    printf("  ✓ code block parsing passed\n");
}

void test_blockquote() {
    printf("Testing blockquote parsing...\n");

    create_test_file("test_quote.org",
        "#+title: Quote Test\n"
        "\n"
        "* Quote Section\n"
        "\n"
        "#+begin_quote\n"
        "This is a quote.\n"
        "It spans multiple lines.\n"
        "#+end_quote\n"
    );

    Tokenizer *t = tokenizer_create("test_quote.org");
    assert(t != NULL);

    Parser *p = parser_create(t);
    assert(p != NULL);

    Node *doc = parser_parse(p);
    assert(doc != NULL);
    assert(doc->type == NODE_DOCUMENT);

    Node *quote_node = NULL;
    for (int i = 0; i < doc->child_count; i++) {
        if (doc->children[i]->type == NODE_BLOCKQUOTE) {
            quote_node = doc->children[i];
            break;
        }
    }

    assert(quote_node != NULL);
    printf("  Blockquote has %d children\n", quote_node->child_count);

    node_free(doc);
    parser_free(p);
    tokenizer_free(t);
    remove("test_quote.org");

    printf("  ✓ blockquote parsing passed\n");
}

void test_list() {
    printf("Testing list parsing...\n");

    create_test_file("test_list.org",
        "#+title: List Test\n"
        "\n"
        "* Items\n"
        "\n"
        "- First item\n"
        "- Second item\n"
        "- Third item\n"
    );

    Tokenizer *t = tokenizer_create("test_list.org");
    assert(t != NULL);

    Parser *p = parser_create(t);
    assert(p != NULL);

    Node *doc = parser_parse(p);
    assert(doc != NULL);
    assert(doc->type == NODE_DOCUMENT);

    Node *list_node = NULL;
    for (int i = 0; i < doc->child_count; i++) {
        if (doc->children[i]->type == NODE_LIST) {
            list_node = doc->children[i];
            break;
        }
    }

    assert(list_node != NULL);
    assert(list_node->child_count == 3);

    printf("  List has %d items\n", list_node->child_count);

    node_free(doc);
    parser_free(p);
    tokenizer_free(t);
    remove("test_list.org");

    printf("  ✓ list parsing passed\n");
}

void test_heading_followed_by_text() {
    printf("Testing heading followed by text without blank line...\n");

    create_test_file("test_heading_text.org",
        "* Heading 1\n"
        "This is a paragraph.\n"
    );

    Tokenizer *t = tokenizer_create("test_heading_text.org");
    assert(t != NULL);

    Parser *p = parser_create(t);
    assert(p != NULL);

    Node *doc = parser_parse(p);
    assert(doc != NULL);
    assert(doc->type == NODE_DOCUMENT);

    int heading_count = 0;
    int paragraph_count = 0;
    Node *heading_node = NULL;

    for (int i = 0; i < doc->child_count; i++) {
        Node *child = doc->children[i];
        if (child->type == NODE_HEADING) {
            heading_count++;
            heading_node = child;
        } else if (child->type == NODE_PARAGRAPH) {
            paragraph_count++;
        }
    }

    assert(heading_count == 1);
    assert(paragraph_count == 1);
    assert(heading_node != NULL);

    char *heading_text = string_to_cstr(heading_node->value);
    assert(strcmp(heading_text, "Heading 1") == 0);
    free(heading_text);

    printf("  Heading text: 'Heading 1'\n");
    printf("  Document has 1 heading and 1 paragraph\n");

    node_free(doc);
    parser_free(p);
    tokenizer_free(t);
    remove("test_heading_text.org");

    printf("  ✓ heading followed by text parsing passed\n");
}

void test_multiple_consecutive_headings() {
    printf("Testing multiple consecutive headings without blank lines...\n");

    create_test_file("test_multi_headings.org",
        "* Heading 1\n"
        "** Heading 2\n"
        "*** Heading 3\n"
    );

    Tokenizer *t = tokenizer_create("test_multi_headings.org");
    assert(t != NULL);

    Parser *p = parser_create(t);
    assert(p != NULL);

    Node *doc = parser_parse(p);
    assert(doc != NULL);
    assert(doc->type == NODE_DOCUMENT);

    assert(doc->child_count == 3);

    char *h1_text = string_to_cstr(doc->children[0]->value);
    char *h2_text = string_to_cstr(doc->children[1]->value);
    char *h3_text = string_to_cstr(doc->children[2]->value);

    assert(strcmp(h1_text, "Heading 1") == 0);
    assert(strcmp(h2_text, "Heading 2") == 0);
    assert(strcmp(h3_text, "Heading 3") == 0);

    printf("  H1: '%s'\n", h1_text);
    printf("  H2: '%s'\n", h2_text);
    printf("  H3: '%s'\n", h3_text);

    free(h1_text);
    free(h2_text);
    free(h3_text);

    node_free(doc);
    parser_free(p);
    tokenizer_free(t);
    remove("test_multi_headings.org");

    printf("  ✓ multiple consecutive headings parsing passed\n");
}

int main() {
    printf("=== Parser Tests ===\n\n");

    test_simple_document();
    test_code_blocks();
    test_blockquote();
    test_list();
    test_heading_followed_by_text();
    test_multiple_consecutive_headings();

    printf("\n✓ All parser tests passed!\n");
    return 0;
}
