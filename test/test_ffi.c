#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/org-ffi.h"

static void assert_contains(const char *haystack, const char *needle) {
    assert(haystack != NULL);
    assert(strstr(haystack, needle) != NULL);
}

static char *parse_html(const char *input) {
    char *html = org_parse_to_html(input, strlen(input));
    assert(html != NULL);
    return html;
}

static char *extract_toc(const char *input) {
    char *toc = org_extract_toc(input, strlen(input));
    assert(toc != NULL);
    return toc;
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *content = malloc(size + 1);
    if (!content) {
        fclose(f);
        return NULL;
    }

    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);

    return content;
}

static char *read_file_or_skip(const char *path) {
    char *content = read_file(path);
    if (!content) {
        printf("  SKIP (file not found)\n");
    }
    return content;
}

void test_parse_to_html(void) {
    printf("  test_parse_to_html...");

    char *input = "#+title: Test\n\n* Heading\n\nParagraph text.";
    char *html = parse_html(input);

    assert_contains(html, "Heading");
    assert_contains(html, "Paragraph");

    org_free_string(html);
    printf("  OK\n");
}

void test_parse_from_file(void) {
    printf("  test_parse_from_file...");

    char *content = read_file_or_skip("test/test_issues.org");
    if (!content) return;

    char *html = parse_html(content);
    assert_contains(html, "First Heading");
    assert_contains(html, "https://example.com");
    assert_contains(html, "<img src=\"https://example.com/images/img.png\"");
    assert_contains(html, "class=\"img\"");
    assert_contains(html, "width=\"70%\"");
    assert_contains(html, "height=\"70%\"");

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

    char *content = read_file_or_skip("test/test_issues.org");
    if (!content) return;

    OrgMetadata *meta = org_extract_metadata(content, strlen(content));
    assert(meta != NULL);

    const char *title = org_meta_get_title(meta);
    const char *date = org_meta_get_date(meta);
    const char *tags = org_meta_get_tags(meta);

    assert_contains(title, "Test Document");
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

    char *html = parse_html(input);
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
    char *html = parse_html(input);
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

void test_url_punctuation(void) {
    printf("  test_url_punctuation...");

    char *input = "* URL Punctuation Test\n\n"
        "English punctuation: https://example.com. https://example.org, https://example.net!\n"
        "Chinese punctuation: https://example.com。 https://example.org， https://example.net！\n"
        "Parentheses: (https://example.com) （https://example.org）\n"
        "Mixed: https://example.com，测试文本.";

    char *html = parse_html(input);

    assert(strstr(html, "href=\"https://example.com\"") != NULL);
    assert(strstr(html, "href=\"https://example.org\"") != NULL);
    assert(strstr(html, "href=\"https://example.net\"") != NULL);

    assert(strstr(html, ">https://example.com</a>.") != NULL);
    assert(strstr(html, ">https://example.org</a>,") != NULL);
    assert(strstr(html, ">https://example.net</a>!") != NULL);

    assert(strstr(html, ">https://example.com</a>。") != NULL);
    assert(strstr(html, ">https://example.org</a>，") != NULL);
    assert(strstr(html, ">https://example.net</a>！") != NULL);

    assert(strstr(html, "(<a href=\"https://example.com\">https://example.com</a>)") != NULL);
    assert(strstr(html, "（<a href=\"https://example.org\">https://example.org</a>）") != NULL);

    assert(strstr(html, ">https://example.com</a>，测试文本") != NULL);

    org_free_string(html);
    printf("  OK\n");
}

void test_url_balanced_parentheses(void) {
    printf("  test_url_balanced_parentheses...");

    char *input = "* URL Balanced Parentheses Test\n\n"
        "URL with balanced parens: http://example.com/test(1).html\n"
        "URL with nested balanced parens: http://example.com/path(foo(bar)).html\n"
        "API method with parens: https://example.com/api/method(arg1,arg2).\n"
        "MSDN versioned URL: http://msdn.microsoft.com/en-us/library/aa752574(VS.85).aspx\n"
        "Multiple balanced parens: https://example.com/(nested)/(path).html\n"
        "Unmatched closing paren: http://example.com/test).html should stop at )";

    char *html = parse_html(input);

    assert(strstr(html, "href=\"http://example.com/test(1).html\"") != NULL);
    assert(strstr(html, ">http://example.com/test(1).html</a>") != NULL);

    assert(strstr(html, "href=\"http://example.com/path(foo(bar)).html\"") != NULL);
    assert(strstr(html, ">http://example.com/path(foo(bar)).html</a>") != NULL);

    assert(strstr(html, "href=\"https://example.com/api/method(arg1,arg2)\"") != NULL);
    assert(strstr(html, ">https://example.com/api/method(arg1,arg2)</a>.") != NULL);

    assert(strstr(html, "href=\"http://msdn.microsoft.com/en-us/library/aa752574(VS.85).aspx\"") != NULL);
    assert(strstr(html, ">http://msdn.microsoft.com/en-us/library/aa752574(VS.85).aspx</a>") != NULL);

    assert(strstr(html, "href=\"https://example.com/(nested)/(path).html\"") != NULL);

    assert(strstr(html, "href=\"http://example.com/test\"") != NULL);
    assert(strstr(html, ">http://example.com/test</a>).html") != NULL);

    org_free_string(html);
    printf("  OK\n");
}

void test_verbatim_url_no_link(void) {
    printf("  test_verbatim_url_no_link...");

    char *input = "* Verbatim URL Test\n\n"
        "URL in verbatim: ~https://example.com~\n"
        "URL in code: =https://example.com=\n"
        "Plain URL should still be link: https://example.com";

    char *html = org_parse_to_html(input, strlen(input));

    assert(html != NULL);

    const char *code_start = strstr(html, "<code>https://example.com</code>");
    assert(code_start != NULL);

    const char *code_end = strstr(code_start, "</code>");
    assert(code_end != NULL);

    size_t code_section_len = code_end - code_start + 7;
    char *code_section = malloc(code_section_len + 1);
    strncpy(code_section, code_start, code_section_len);
    code_section[code_section_len] = '\0';

    assert(strstr(code_section, "<a href") == NULL);

    free(code_section);

    assert(strstr(html, "href=\"https://example.com\"") != NULL);

    org_free_string(html);
    printf("  OK\n");
}

void test_extract_toc_basic(void) {
    printf("  test_extract_toc_basic...");

    char *input = "* First Heading\n\n* Second Heading\n\n* Third Heading";

    char *toc = extract_toc(input);
    assert(strstr(toc, "class=\"toc\"") != NULL);
    assert(strstr(toc, "First Heading") != NULL);
    assert(strstr(toc, "Second Heading") != NULL);
    assert(strstr(toc, "Third Heading") != NULL);
    assert(strstr(toc, "<ul>") != NULL);
    assert(strstr(toc, "<li>") != NULL);
    assert(strstr(toc, "<a href=") != NULL);

    org_free_string(toc);
    printf("  OK\n");
}

void test_extract_toc_nested(void) {
    printf("  test_extract_toc_nested...");

    char *input = "* Level 1\n\n** Level 2\n\n*** Level 3\n\n** Another Level 2\n\n* Back to Level 1";

    char *toc = extract_toc(input);
    assert(strstr(toc, "Level 1") != NULL);
    assert(strstr(toc, "Level 2") != NULL);
    assert(strstr(toc, "Level 3") != NULL);
    assert(strstr(toc, "Another Level 2") != NULL);
    assert(strstr(toc, "Back to Level 1") != NULL);

    int ul_count = 0;
    const char *pos = toc;
    while ((pos = strstr(pos, "<ul>")) != NULL) {
        ul_count++;
        pos += 4;
    }
    assert(ul_count >= 2);

    org_free_string(toc);
    printf("  OK\n");
}

void test_extract_toc_empty(void) {
    printf("  test_extract_toc_empty...");

    char *input = "Just some text without headings.";

    char *toc = extract_toc(input);
    assert(strstr(toc, "class=\"toc\"") != NULL);

    org_free_string(toc);

    toc = org_extract_toc("", 0);
    assert(toc == NULL);

    toc = org_extract_toc(NULL, 0);
    assert(toc == NULL);

    printf("  OK\n");
}

void test_html_with_ids(void) {
    printf("  test_html_with_ids...");

    char *input = "#+title: Test\n\n* First Heading\n\nSome content.\n\n** Sub Heading\n\nMore content.";

    char *html = parse_html(input);
    assert(strstr(html, "id=\"first-heading\"") != NULL);
    assert(strstr(html, "id=\"sub-heading\"") != NULL);
    assert(strstr(html, "<h2 id=\"first-heading\">First Heading</h2>") != NULL);

    org_free_string(html);
    printf("  OK\n");
}

void test_toc_and_html_consistency(void) {
    printf("  test_toc_and_html_consistency...");

    char *input = "* Introduction\n\n* Getting Started\n\n** Installation\n\n** Configuration\n\n* Conclusion";

    char *toc = extract_toc(input);
    char *html = parse_html(input);

    const char *href_intro = strstr(toc, "href=\"#introduction\"");
    assert(href_intro != NULL);

    assert(strstr(html, "id=\"introduction\"") != NULL);

    const char *href_install = strstr(toc, "href=\"#installation\"");
    assert(href_install != NULL);

    assert(strstr(html, "id=\"installation\"") != NULL);

    org_free_string(toc);
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
    test_url_punctuation();
    test_url_balanced_parentheses();
    test_verbatim_url_no_link();
    test_extract_toc_basic();
    test_extract_toc_nested();
    test_extract_toc_empty();
    test_html_with_ids();
    test_toc_and_html_consistency();

    printf("\nAll tests passed!\n");
    return 0;
}
