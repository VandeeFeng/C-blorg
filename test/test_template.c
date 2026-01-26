#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "template.h"
#include "org-string.h"

static void create_test_template(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("ERROR: Failed to create test template\n");
        exit(1);
    }
    fprintf(f, "<!DOCTYPE html>\n");
    fprintf(f, "<html>\n");
    fprintf(f, "<head>\n");
    fprintf(f, "  <title>{{title}}</title>\n");
    fprintf(f, "</head>\n");
    fprintf(f, "<body>\n");
    fprintf(f, "  <h1>{{heading}}</h1>\n");
    fprintf(f, "  <p>{{content}}</p>\n");
    fprintf(f, "  <footer>{{footer}}</footer>\n");
    fprintf(f, "</body>\n");
    fprintf(f, "</html>\n");
    fclose(f);
}

static void test_template_creation() {
    printf("Testing template creation...\n");

    create_test_template("/tmp/test_template.html");

    Template *t = template_create("/tmp/test_template.html", "/tmp");
    if (!t) {
        printf("ERROR: Failed to create template\n");
        return;
    }

    assert(t->content != NULL);
    char *content = string_to_cstr(t->content);
    assert(strstr(content, "{{title}}") != NULL);
    assert(strstr(content, "{{heading}}") != NULL);
    assert(strstr(content, "{{content}}") != NULL);
    free(content);

    template_free(t);
    printf("Template creation: PASS\n");
}

static void test_template_set_var() {
    printf("\nTesting template variable setting...\n");

    create_test_template("/tmp/test_template2.html");

    Template *t = template_create("/tmp/test_template2.html", "/tmp");
    if (!t) {
        printf("ERROR: Failed to create template\n");
        return;
    }

    template_set_var(t, "title", "Test Title");
    template_set_var(t, "heading", "Test Heading");
    template_set_var(t, "content", "Test Content");
    template_set_var(t, "footer", "Test Footer");

    assert(t->var_count == 4);
    assert(strcmp(t->vars[0].value, "Test Title") == 0);
    assert(strcmp(t->vars[1].value, "Test Heading") == 0);
    assert(strcmp(t->vars[2].value, "Test Content") == 0);
    assert(strcmp(t->vars[3].value, "Test Footer") == 0);

    template_free(t);
    printf("Template variable setting: PASS\n");
}

static void test_template_render() {
    printf("\nTesting template rendering...\n");

    create_test_template("/tmp/test_template3.html");

    Template *t = template_create("/tmp/test_template3.html", "/tmp");
    if (!t) {
        printf("ERROR: Failed to create template\n");
        return;
    }

    template_set_var(t, "title", "My Blog Post");
    template_set_var(t, "heading", "Welcome");
    template_set_var(t, "content", "This is the content.");
    template_set_var(t, "footer", "Copyright 2024");

    String *output = string_create(1024);
    template_render(t, output);

    char *result = string_to_cstr(output);
    printf("Rendered output:\n%s\n", result);
    fflush(stdout);
    assert(strstr(result, "<title>My Blog Post</title>") != NULL);
    assert(strstr(result, "<h1>Welcome</h1>") != NULL);
    assert(strstr(result, "<p>This is the content.</p>") != NULL);
    assert(strstr(result, "<footer>Copyright 2024</footer>") != NULL);

    printf("Rendered output:\n%s\n", result);

    free(result);
    string_free(output);
    template_free(t);
    printf("Template rendering: PASS\n");
}

static void test_template_var_update() {
    printf("\nTesting template variable update...\n");

    create_test_template("/tmp/test_template4.html");

    Template *t = template_create("/tmp/test_template4.html", "/tmp");
    if (!t) {
        printf("ERROR: Failed to create template\n");
        return;
    }

    template_set_var(t, "title", "Original Title");
    assert(strcmp(t->vars[0].value, "Original Title") == 0);

    template_set_var(t, "title", "Updated Title");
    assert(t->var_count == 1);
    assert(strcmp(t->vars[0].value, "Updated Title") == 0);

    template_free(t);
    printf("Template variable update: PASS\n");
}

static void test_template_missing_var() {
    printf("\nTesting template with missing variable...\n");

    create_test_template("/tmp/test_template5.html");

    Template *t = template_create("/tmp/test_template5.html", "/tmp");
    if (!t) {
        printf("ERROR: Failed to create template\n");
        return;
    }

    template_set_var(t, "title", "Test Title");
    template_set_var(t, "content", "Test Content");

    String *output = string_create(1024);
    template_render(t, output);

    char *result = string_to_cstr(output);

    assert(strstr(result, "<title>Test Title</title>") != NULL);
    assert(strstr(result, "<p>Test Content</p>") != NULL);
    assert(strstr(result, "<h1></h1>") != NULL);

    free(result);
    string_free(output);
    template_free(t);
    printf("Template with missing variable: PASS\n");
}

int main() {
    printf("=== Template System Tests ===\n\n");

    test_template_creation();
    test_template_set_var();
    test_template_render();
    test_template_var_update();
    test_template_missing_var();

    printf("\n=== All template tests passed! ===\n");
    return 0;
}
