#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void test_generated_page_structure(void) {
    printf("Testing generated page structure...\n");

    FILE *f = fopen("output/test-post.html", "r");
    if (!f) {
        printf("SKIP (output file not found)\n");
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);

    printf("  Checking DOCTYPE declaration...");
    assert(strstr(content, "<!DOCTYPE html>") != NULL);
    printf(" OK\n");

    printf("  Checking HTML structure...");
    assert(strstr(content, "<html lang=\"en\">") != NULL);
    assert(strstr(content, "</html>") != NULL);
    printf(" OK\n");

    printf("  Checking HEAD section with metadata...");
    assert(strstr(content, "<head>") != NULL);
    assert(strstr(content, "</head>") != NULL);
    assert(strstr(content, "<meta charset=\"UTF-8\">") != NULL);
    assert(strstr(content, "<meta name=\"viewport\"") != NULL);
    assert(strstr(content, "<title>First Blog Post</title>") != NULL);
    assert(strstr(content, "blog-style.css") != NULL);
    printf(" OK\n");

    printf("  Checking BODY structure...");
    assert(strstr(content, "<body>") != NULL);
    assert(strstr(content, "</body>") != NULL);
    printf(" OK\n");

    printf("  Checking HEADER with navigation...");
    assert(strstr(content, "<div id=\"preamble\"") != NULL);
    assert(strstr(content, "<header>") != NULL);
    assert(strstr(content, "<h1><a href=\"https://www.vandee.art/blog/\">Vandee's Blog</a></h1>") != NULL);
    assert(strstr(content, "<nav>") != NULL);
    assert(strstr(content, "<a href=\"https://www.vandee.art/blog/\">Home</a>") != NULL);
    assert(strstr(content, "<a href=\"archive.html\">Archive</a>") != NULL);
    assert(strstr(content, "<a href=\"tags.html\">Tags</a>") != NULL);
    assert(strstr(content, "shiba_idle_8fps.gif") != NULL);
    printf(" OK\n");

    printf("  Checking CONTENT div with orgize body...");
    assert(strstr(content, "<div id=\"content\">") != NULL);
    assert(strstr(content, "<main>") != NULL);
    assert(strstr(content, "<h1>Introduction</h1>") != NULL);
    assert(strstr(content, "<h1>Features</h1>") != NULL);
    assert(strstr(content, "<pre><code class=\"language-rust\">") != NULL);
    assert(strstr(content, "fn hello()") != NULL);
    printf(" OK\n");

    printf("  Checking FOOTER with copyright...");
    assert(strstr(content, "<div id=\"postamble\"") != NULL);
    assert(strstr(content, "<footer>") != NULL);
    assert(strstr(content, "© 2022-<script>document.write(new Date().getFullYear())</script> Vandee") != NULL);
    printf(" OK\n");

    printf("  Checking JavaScript files...");
    assert(strstr(content, "app.js") != NULL);
    assert(strstr(content, "shiba.js") != NULL);
    assert(strstr(content, "copyCode.js") != NULL);
    assert(strstr(content, "search.js") != NULL);
    assert(strstr(content, "pangu") != NULL);
    printf(" OK\n");

    printf("  Checking top link button...");
    assert(strstr(content, "<a href=\"#top\"") != NULL);
    assert(strstr(content, "class=\"top-link\"") != NULL);
    assert(strstr(content, "ri-arrow-up-double-line") != NULL);
    printf(" OK\n");

    free(content);
    printf("\nFull page structure test: PASS\n");
}

void test_no_duplicate_html_tags(void) {
    printf("\nTesting no duplicate HTML structure tags...\n");

    FILE *f = fopen("output/test-post.html", "r");
    if (!f) {
        printf("SKIP (output file not found)\n");
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);

    int doctype_count = 0;
    int html_count = 0;
    int head_count = 0;
    int body_count = 0;

    char *pos = content;
    while ((pos = strstr(pos, "<!DOCTYPE html>")) != NULL) {
        doctype_count++;
        pos++;
    }

    pos = content;
    while ((pos = strstr(pos, "<html")) != NULL) {
        html_count++;
        pos++;
    }

    pos = content;
    while ((pos = strstr(pos, "<head>")) != NULL) {
        head_count++;
        pos++;
    }

    pos = content;
    while ((pos = strstr(pos, "<body>")) != NULL) {
        body_count++;
        pos++;
    }

    printf("  DOCTYPE count: %d (expected: 1)\n", doctype_count);
    printf("  <html> count: %d (expected: 1)\n", html_count);
    printf("  <head> count: %d (expected: 1)\n", head_count);
    printf("  <body> count: %d (expected: 1)\n", body_count);

    assert(doctype_count == 1);
    assert(html_count == 1);
    assert(head_count == 1);
    assert(body_count == 1);

    free(content);
    printf("No duplicate HTML tags test: PASS\n");
}

int main(void) {
    printf("=== Page Structure Tests ===\n\n");

    test_generated_page_structure();
    test_no_duplicate_html_tags();

    printf("\n=== All page structure tests passed! ===\n");
    return 0;
}
