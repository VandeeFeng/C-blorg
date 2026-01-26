#include <stdlib.h>
#include <string.h>
#include "org-string.h"

#define SDS_MAX_PREALLOC (1024 * 1024)

String *string_create(size_t initial_capacity) {
    String *s = malloc(sizeof(String));
    if (!s) return NULL;

    s->data = malloc(initial_capacity);
    if (!s->data) {
        free(s);
        return NULL;
    }

    s->len = 0;
    s->cap = initial_capacity;
    s->data[0] = '\0';
    return s;
}

static size_t calculate_new_capacity(String *s, size_t len) {
    size_t needed = s->len + len + 1;

    if (s->cap >= needed) return s->cap;

    size_t new_cap = s->cap * 2;

    if (new_cap >= SDS_MAX_PREALLOC) {
        new_cap = needed + SDS_MAX_PREALLOC;
    }

    if (new_cap < needed) {
        new_cap = needed;
    }

    return new_cap;
}

void string_append(String *s, const char *data, size_t len) {
    if (!s || !data) return;

    size_t new_cap = calculate_new_capacity(s, len);

    if (new_cap > s->cap) {
        char *new_data = realloc(s->data, new_cap);
        if (!new_data) return;

        s->data = new_data;
        s->cap = new_cap;
    }

    memcpy(s->data + s->len, data, len);
    s->len += len;
    s->data[s->len] = '\0';
}

void string_append_cstr(String *s, const char *str) {
    if (!s || !str) return;

    size_t len = strlen(str);
    string_append(s, str, len);
}

char *string_to_cstr(const String *s) {
    if (!s) return NULL;

    char *result = malloc(s->len + 1);
    if (!result) return NULL;

    memcpy(result, s->data, s->len);
    result[s->len] = '\0';
    return result;
}

void string_free(String *s) {
    if (s) {
        if (s->data) {
            free(s->data);
        }
        free(s);
    }
}
