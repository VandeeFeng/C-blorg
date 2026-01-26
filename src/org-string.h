#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <string.h>

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} String;

String *string_create(size_t initial_capacity);
void string_append(String *s, const char *data, size_t len);
void string_append_cstr(String *s, const char *str);
char *string_to_cstr(const String *s);
void string_free(String *s);

#endif
