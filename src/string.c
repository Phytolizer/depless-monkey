#include "monkey/string.h"

#include <assert.h>
#include <iso646.h>
#include <stdarg.h>
#include <stdio.h>

struct string string_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int length = vsnprintf(0, 0, fmt, args);
    va_end(args);

    struct string str = {
        .data = malloc(length + 1),
        .length = length,
        .capacity = length + 1,
    };

    va_start(args, fmt);
    vsnprintf(str.data, str.capacity, fmt, args);
    va_end(args);

    str.length = length;

    return str;
}

struct string string_dup(struct string s) {
    char* result = malloc(s.length);
    memcpy(result, s.data, s.length);
    return STRING_OWN_DATA(result, s.length);
}

void string_append(struct string* buf, struct string arg) {
    if (arg.length == 0) return;

    size_t old_cap = buf->capacity;
    while (buf->length + arg.length > buf->capacity) {
        buf->capacity = buf->capacity * 2 + 1;
    }
    if (buf->capacity > old_cap) {
        buf->data = realloc(buf->data, buf->capacity);
    }
    memcpy(buf->data + buf->length, arg.data, arg.length);
    buf->length += arg.length;
}

void string_append_printf(struct string* buf, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int length = vsnprintf(0, 0, fmt, args);
    va_end(args);

    size_t old_cap = buf->capacity;
    while (buf->length + length > buf->capacity) {
        buf->capacity = buf->capacity * 2 + 1;
    }
    if (buf->capacity > old_cap) {
        buf->data = realloc(buf->data, buf->capacity);
    }

    va_start(args, fmt);
    vsnprintf(buf->data + buf->length, buf->capacity - buf->length, fmt, args);
    va_end(args);

    buf->length += length;
}

bool string_getline(struct string* line, FILE* in) {
    line->length = 0;

    while (true) {
        char c = fgetc(in);
        if (c == '\n' or c == EOF) {
            return c == '\n';
        }

        if (line->length == line->capacity) {
            line->capacity = line->capacity * 2 + 1;
            char* temp = realloc(line->data, line->capacity);
            assert(temp != NULL);
            line->data = temp;
        }

        line->data[line->length] = c;
        line->length += 1;
    }
}
