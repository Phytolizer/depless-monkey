#ifndef MONKEY_ENVIRONMENT_H_
#define MONKEY_ENVIRONMENT_H_

#include "monkey/buf.h"
#include "monkey/object.h"
#include "monkey/string.h"

struct environment_entry {
    struct string name;
    struct object* value;
};

BUF_T(struct environment_entry, environment_entry);

struct environment {
    struct environment_entry_buf entries;
    size_t count;
};

extern void environment_init(struct environment* env);
extern void environment_free(struct environment env);

extern void environment_set(struct environment* env, struct string name, struct object* value);
extern struct object* environment_get(struct environment* env, struct string name);

#endif  // MONKEY_ENVIRONMENT_H_
