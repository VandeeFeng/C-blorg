#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/org-string.h"

void test_string_basic() {
    printf("Testing string basic operations...\n");

    String *s = string_create(10);
    assert(s != NULL);
    assert(s->len == 0);
    assert(s->cap == 10);
    assert(s->data[0] == '\0');

    string_append_cstr(s, "Hello");
    assert(s->len == 5);
    assert(strcmp(s->data, "Hello") == 0);

    char *cstr = string_to_cstr(s);
    assert(cstr != NULL);
    assert(strcmp(cstr, "Hello") == 0);
    free(cstr);

    string_free(s);
    printf("  ✓ string basic operations passed\n");
}

void test_string_growth() {
    printf("Testing string growth...\n");

    String *s = string_create(10);
    assert(s != NULL);

    string_append_cstr(s, "1234567890");
    assert(s->len == 10);
    assert(s->cap == 20);

    string_append_cstr(s, "ABCDE");
    assert(s->len == 15);
    assert(s->cap == 20);

    assert(strcmp(s->data, "1234567890ABCDE") == 0);

    string_free(s);
    printf("  ✓ string growth passed\n");
}

void test_string_append() {
    printf("Testing string append...\n");

    String *s = string_create(16);
    assert(s != NULL);

    string_append_cstr(s, "Hello");
    assert(strcmp(s->data, "Hello") == 0);

    string_append_cstr(s, " ");
    assert(strcmp(s->data, "Hello ") == 0);

    string_append_cstr(s, "World");
    assert(strcmp(s->data, "Hello World") == 0);

    string_append(s, "!", 1);
    assert(strcmp(s->data, "Hello World!") == 0);

    string_free(s);
    printf("  ✓ string append passed\n");
}

void test_string_edge_cases() {
    printf("Testing string edge cases...\n");

    String *s = string_create(16);
    assert(s != NULL);

    string_append_cstr(s, "");
    assert(s->len == 0);
    assert(s->data[0] == '\0');

    string_append_cstr(NULL, "test");
    assert(s->len == 0);

    string_append_cstr(s, NULL);
    assert(s->len == 0);

    string_append(s, NULL, 0);
    assert(s->len == 0);

    char *cstr = string_to_cstr(NULL);
    assert(cstr == NULL);

    string_free(s);
    printf("  ✓ string edge cases passed\n");
}

void test_string_large_append() {
    printf("Testing large string append...\n");

    String *s = string_create(16);
    assert(s != NULL);

    char large[1000];
    memset(large, 'A', 999);
    large[999] = '\0';

    string_append_cstr(s, large);
    assert(s->len == 999);
    assert(s->data[0] == 'A');
    assert(s->data[998] == 'A');
    assert(s->data[999] == '\0');

    string_free(s);
    printf("  ✓ large string append passed\n");
}

int main() {
    printf("=== String Utility Tests ===\n\n");

    test_string_basic();
    test_string_growth();
    test_string_append();
    test_string_edge_cases();
    test_string_large_append();

    printf("\n✓ All string tests passed!\n");
    return 0;
}
