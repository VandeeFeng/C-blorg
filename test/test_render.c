#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "org-string.h"
#include "parser.h"
#include "render.h"

static void test_simple_document() {
    printf("Testing simple document rendering...\n");

    const char *input = "#+title: Test Document\n"
                         "#+date: 2024-01-01\n"
                         "\n"
                         "* Heading 1\n"
                         "\n"
                         "This is a paragraph.\n"
                         "\n"
                         "** Heading 2\n"
                         "\n"
                         "Another paragraph.\n";

    FILE *f = fopen("/tmp/test_render_simple.org", "w");
    if (f) {
        fputs(input, f);
        fclose(f);
    }

    Tokenizer *t = tokenizer_create("/tmp/test_render_simple.org");
    if (!t) {
        printf("ERROR: Failed to create tokenizer\n");
        return;
    }

    Parser *p = parser_create(t);
    if (!p) {
        printf("ERROR: Failed to create parser\n");
        tokenizer_free(t);
        return;
    }

    Node *doc = parser_parse(p);
    if (!doc) {
        printf("ERROR: Failed to parse document\n");
        parser_free(p);
        tokenizer_free(t);
        return;
    }

    String *output = string_create(1024);
    render_document_content(doc, output);

    char *result = string_to_cstr(output);

    assert(strstr(result, "<title>") == NULL);
    assert(strstr(result, "<h2>Heading 1</h2>") != NULL);
    assert(strstr(result, "<h3>Heading 2</h3>") != NULL);
    assert(strstr(result, "<p>This is a paragraph.</p>") != NULL);

    printf("Simple document rendering: PASS\n");

    free(result);
    string_free(output);
    node_free(doc);
    parser_free(p);
    tokenizer_free(t);
}

static void test_code_block() {
    printf("\nTesting code block rendering...\n");

    const char *input = "#+begin_src c\n"
                         "int main() {\n"
                         "    return 0;\n"
                         "}\n"
                         "#+end_src\n";

    FILE *f = fopen("/tmp/test_render_code.org", "w");
    if (f) {
        fputs(input, f);
        fclose(f);
    }

    Tokenizer *t = tokenizer_create("/tmp/test_render_code.org");
    Parser *p = parser_create(t);
    Node *doc = parser_parse(p);

    String *output = string_create(1024);
    render_node_to_html(doc, output);

    char *result = string_to_cstr(output);

    assert(strstr(result, "<pre><code") != NULL);
    assert(strstr(result, "class=\"language-c\"") != NULL);
    assert(strstr(result, "int main()") != NULL);

    printf("Code block rendering: PASS\n");
    printf("Output:\n%s\n", result);

    free(result);
    string_free(output);
    node_free(doc);
    parser_free(p);
    tokenizer_free(t);
}

static void test_blockquote() {
    printf("\nTesting blockquote rendering...\n");

    const char *input = "#+begin_quote\n"
                         "This is a quote.\n"
                         "It can span multiple lines.\n"
                         "#+end_quote\n";

    FILE *f = fopen("/tmp/test_render_quote.org", "w");
    if (f) {
        fputs(input, f);
        fclose(f);
    }

    Tokenizer *t = tokenizer_create("/tmp/test_render_quote.org");
    Parser *p = parser_create(t);
    Node *doc = parser_parse(p);

    String *output = string_create(1024);
    render_node_to_html(doc, output);

    char *result = string_to_cstr(output);

    assert(strstr(result, "<blockquote>") != NULL);
    assert(strstr(result, "</blockquote>") != NULL);
    assert(strstr(result, "This is a quote") != NULL);

    printf("Blockquote rendering: PASS\n");
    printf("Output:\n%s\n", result);

    free(result);
    string_free(output);
    node_free(doc);
    parser_free(p);
    tokenizer_free(t);
}

static void test_list() {
    printf("\nTesting list rendering...\n");

    const char *input = "- First item\n"
                         "- Second item\n"
                         "- Third item\n";

    FILE *f = fopen("/tmp/test_render_list.org", "w");
    if (f) {
        fputs(input, f);
        fclose(f);
    }

    Tokenizer *t = tokenizer_create("/tmp/test_render_list.org");
    Parser *p = parser_create(t);
    Node *doc = parser_parse(p);

    String *output = string_create(1024);
    render_node_to_html(doc, output);

    char *result = string_to_cstr(output);

    assert(strstr(result, "<ul>") != NULL);
    assert(strstr(result, "</ul>") != NULL);
    assert(strstr(result, "<li>First item</li>") != NULL);
    assert(strstr(result, "<li>Second item</li>") != NULL);

    printf("List rendering: PASS\n");
    printf("Output:\n%s\n", result);

    free(result);
    string_free(output);
    node_free(doc);
    parser_free(p);
    tokenizer_free(t);
}

static void test_escaping() {
    printf("\nTesting HTML escaping...\n");

    const char *input = "* Heading with <script>alert('XSS')</script>\n";

    FILE *f = fopen("/tmp/test_render_escape.org", "w");
    if (f) {
        fputs(input, f);
        fclose(f);
    }

    Tokenizer *t = tokenizer_create("/tmp/test_render_escape.org");
    Parser *p = parser_create(t);
    Node *doc = parser_parse(p);

    String *output = string_create(1024);
    render_node_to_html(doc, output);

    char *result = string_to_cstr(output);

    assert(strstr(result, "&lt;") != NULL);
    assert(strstr(result, "&gt;") != NULL);
    assert(strstr(result, "<script>") == NULL);

    printf("HTML escaping: PASS\n");
    printf("Output:\n%s\n", result);

    free(result);
    string_free(output);
    node_free(doc);
    parser_free(p);
    tokenizer_free(t);
}

static void test_heading_followed_by_text() {
    printf("\nTesting heading followed by text without blank line...\n");

    const char *input = "* Heading 1\n"
                         "This is a paragraph.\n";

    FILE *f = fopen("/tmp/test_render_heading_text.org", "w");
    if (f) {
        fputs(input, f);
        fclose(f);
    }

    Tokenizer *t = tokenizer_create("/tmp/test_render_heading_text.org");
    Parser *p = parser_create(t);
    Node *doc = parser_parse(p);

    String *output = string_create(1024);
    render_node_to_html(doc, output);

    char *result = string_to_cstr(output);

    assert(strstr(result, "<h2>Heading 1</h2>") != NULL);
    assert(strstr(result, "<p>This is a paragraph.</p>") != NULL);
    assert(strstr(result, "<h2>Heading 1 This is a paragraph.</h2>") == NULL);

    printf("Heading followed by text: PASS\n");
    printf("Output:\n%s\n", result);

    free(result);
    string_free(output);
    node_free(doc);
    parser_free(p);
    tokenizer_free(t);
}

static void test_multiple_consecutive_headings() {
    printf("\nTesting multiple consecutive headings without blank lines...\n");

    const char *input = "* Heading 1\n"
                         "** Heading 2\n"
                         "*** Heading 3\n";

    FILE *f = fopen("/tmp/test_render_multi_headings.org", "w");
    if (f) {
        fputs(input, f);
        fclose(f);
    }

    Tokenizer *t = tokenizer_create("/tmp/test_render_multi_headings.org");
    Parser *p = parser_create(t);
    Node *doc = parser_parse(p);

    String *output = string_create(1024);
    render_node_to_html(doc, output);

    char *result = string_to_cstr(output);

    assert(strstr(result, "<h2>Heading 1</h2>") != NULL);
    assert(strstr(result, "<h3>Heading 2</h3>") != NULL);
    assert(strstr(result, "<h4>Heading 3</h4>") != NULL);

    printf("Multiple consecutive headings: PASS\n");
    printf("Output:\n%s\n", result);

    free(result);
    string_free(output);
    node_free(doc);
    parser_free(p);
    tokenizer_free(t);
}

static void test_issues_file() {
    printf("\nTesting test_issues.org file (bug fix verification)...\n");

    Tokenizer *t = tokenizer_create("./test_issues.org");
    if (!t) {
        printf("ERROR: Failed to create tokenizer\n");
        return;
    }

    Parser *p = parser_create(t);
    if (!p) {
        printf("ERROR: Failed to create parser\n");
        tokenizer_free(t);
        return;
    }

    Node *doc = parser_parse(p);
    if (!doc) {
        printf("ERROR: Failed to parse document\n");
        parser_free(p);
        tokenizer_free(t);
        return;
    }

    String *output = string_create(4096);
    render_node_to_html(doc, output);

    char *result = string_to_cstr(output);

    assert(strstr(result, "<h2>First Heading</h2>") != NULL);
    assert(strstr(result, "<h2>Second Heading with inline link</h2>") != NULL);
    assert(strstr(result, "Bug Fix Test: Heading followed by text without blank line") != NULL);
    assert(strstr(result, "<p>This should be a separate paragraph") != NULL);
    assert(strstr(result, "Bug Fix Test: Multiple consecutive headings") != NULL);
    assert(strstr(result, "<h3>Level 2 heading</h3>") != NULL);
    assert(strstr(result, "<h4>Level 3 heading</h4>") != NULL);

    printf("Test issues file: PASS\n");
    printf("Output:\n%s\n", result);

    free(result);
    string_free(output);
    node_free(doc);
    parser_free(p);
    tokenizer_free(t);
}

int main() {
    printf("=== Renderer Tests ===\n\n");

    test_simple_document();
    test_code_block();
    test_blockquote();
    test_list();
    test_escaping();
    test_heading_followed_by_text();
    test_multiple_consecutive_headings();
    test_issues_file();

    printf("\n=== All renderer tests passed! ===\n");
    return 0;
}
