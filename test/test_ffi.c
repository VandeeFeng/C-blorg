#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/org-ffi.h"

void test_parse_to_html(void) {
    printf("  test_parse_to_html...");

    char *input = "#+title: Test\n\n* Heading\n\nParagraph text.";
    char *html = org_parse_to_html(input, strlen(input));

    assert(html != NULL);
    assert(strstr(html, "Heading") != NULL);
    assert(strstr(html, "Paragraph") != NULL);

    org_free_string(html);
    printf("  OK\n");
}

void test_parse_from_file(void) {
    printf("  test_parse_from_file...");

    FILE *f = fopen("test/test_issues.org", "r");
    if (!f) {
        printf("  SKIP (file not found)\n");
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);

    char *html = org_parse_to_html(content, strlen(content));
    assert(html != NULL);
    assert(strstr(html, "First Heading") != NULL);
    assert(strstr(html, "https://example.com") != NULL);

    org_free_string(html);
    free(content);
    printf("  OK\n");
}

void test_parse_empty(void) {
    printf("  test_parse_empty...");

    char *html = org_parse_to_html("", 0);
    assert(html == NULL);

    html = org_parse_to_html(NULL, 0);
    assert(html == NULL);

    printf("  OK\n");
}

void test_extract_metadata_title(void) {
    printf("  test_extract_metadata_title...");

    char *input = "#+title: My Test Document\n\n* Heading";
    OrgMetadata *meta = org_extract_metadata(input, strlen(input));

    assert(meta != NULL);
    const char *title = org_meta_get_title(meta);
    assert(title != NULL);
    assert(strcmp(title, "My Test Document") == 0);

    org_free_metadata(meta);
    printf("  OK\n");
}

void test_extract_metadata_from_file(void) {
    printf("  test_extract_metadata_from_file...");

    FILE *f = fopen("test/test_issues.org", "r");
    if (!f) {
        printf("  SKIP (file not found)\n");
        return;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);

    OrgMetadata *meta = org_extract_metadata(content, strlen(content));
    assert(meta != NULL);

    const char *title = org_meta_get_title(meta);
    const char *date = org_meta_get_date(meta);
    const char *tags = org_meta_get_tags(meta);

    assert(title != NULL);
    assert(strstr(title, "Test Document") != NULL);
    assert(date != NULL);
    assert(tags != NULL);

    org_free_metadata(meta);
    free(content);
    printf("  OK\n");
}

void test_extract_metadata_tags(void) {
    printf("  test_extract_metadata_tags...");

    char *input = "#+filetags: tag1 tag2 tag3\n\n* Heading";
    OrgMetadata *meta = org_extract_metadata(input, strlen(input));

    assert(meta != NULL);
    const char *tags = org_meta_get_tags(meta);
    assert(tags != NULL);
    assert(strstr(tags, "tag1") != NULL);
    assert(strstr(tags, "tag2") != NULL);

    size_t count;
    const char **tags_array = org_meta_get_tags_array(meta, &count);
    assert(tags_array != NULL);
    assert(count == 3);

    org_free_metadata(meta);
    printf("  OK\n");
}

void test_extract_metadata_description(void) {
    printf("  test_extract_metadata_description...");

    char *input = "#+description: This is a test description\n\n* Heading";
    OrgMetadata *meta = org_extract_metadata(input, strlen(input));

    assert(meta != NULL);
    const char *description = org_meta_get_description(meta);
    assert(description != NULL);
    assert(strcmp(description, "") == 0);

    org_free_metadata(meta);
    printf("  OK\n");
}

void test_extract_metadata_missing(void) {
    printf("  test_extract_metadata_missing...");

    char *input = "* Heading\n\nParagraph";
    OrgMetadata *meta = org_extract_metadata(input, strlen(input));

    assert(meta != NULL);
    assert(org_meta_get_title(meta) == NULL);
    assert(org_meta_get_tags(meta) == NULL);

    org_free_metadata(meta);
    printf("  OK\n");
}

void test_extract_metadata_empty(void) {
    printf("  test_extract_metadata_empty...");

    OrgMetadata *meta = org_extract_metadata("", 0);
    assert(meta == NULL);

    meta = org_extract_metadata(NULL, 0);
    assert(meta == NULL);

    printf("  OK\n");
}

void test_complex_document(void) {
    printf("  test_complex_document...");

    char *input = "#+title: Complex Test\n#+filetags: rust ffi\n\n"
                 "* First Heading\n\nText here.\n\n"
                 "** Sub Heading\n\nMore text.\n\n"
                 "- List item 1\n- List item 2\n\n"
                 "#+begin_src rust\n"
                 "fn main() {}\n"
                 "#+end_src";

    char *html = org_parse_to_html(input, strlen(input));
    assert(html != NULL);
    assert(strstr(html, "First Heading") != NULL);
    assert(strstr(html, "Sub Heading") != NULL);
    assert(strstr(html, "List item") != NULL);
    assert(strstr(html, "fn main()") != NULL);

    OrgMetadata *meta = org_extract_metadata(input, strlen(input));
    assert(meta != NULL);
    assert(strcmp(org_meta_get_title(meta), "Complex Test") == 0);
    assert(strstr(org_meta_get_tags(meta), "rust") != NULL);

    org_free_string(html);
    org_free_metadata(meta);
    printf("  OK\n");
}

void test_body_extraction(void) {
    printf("  test_body_extraction...");

    char *input = "#+title: Body Test\n\n* Main Heading\n\nParagraph content.";
    char *html = org_parse_to_html(input, strlen(input));

    assert(html != NULL);
    assert(strstr(html, "Main Heading") != NULL);
    assert(strstr(html, "Paragraph content") != NULL);

    assert(strstr(html, "<!DOCTYPE>") == NULL);
    assert(strstr(html, "<!doctype>") == NULL);
    assert(strstr(html, "<html>") == NULL);
    assert(strstr(html, "</html>") == NULL);
    assert(strstr(html, "<head>") == NULL);
    assert(strstr(html, "</head>") == NULL);
    assert(strstr(html, "<body>") == NULL);
    assert(strstr(html, "</body>") == NULL);

    org_free_string(html);
    printf("  OK\n");
}

int main(void) {
    printf("Running Rust FFI Tests\n");
    printf("======================\n\n");

    test_parse_to_html();
    test_parse_from_file();
    test_parse_empty();
    test_extract_metadata_title();
    test_extract_metadata_from_file();
    test_extract_metadata_tags();
    test_extract_metadata_description();
    test_extract_metadata_missing();
    test_extract_metadata_empty();
    test_complex_document();
    test_body_extraction();

    printf("\nAll tests passed!\n");
    return 0;
}
