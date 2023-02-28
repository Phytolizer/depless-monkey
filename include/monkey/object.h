#ifndef MONKEY_OBJECT_H_
#define MONKEY_OBJECT_H_

#include <stdbool.h>
#include <stdint.h>

#include "monkey/string.h"

enum object_type {
#define X(x) OBJECT_##x,
#include "monkey/private/object_types.inc"
#undef X
};

extern struct string object_type_string(enum object_type type);

struct object;

typedef struct string object_inspect_callback_t(const struct object* object);
typedef void object_free_callback_t(struct object* object);

struct object {
    enum object_type type;
    object_inspect_callback_t* inspect_callback;
    object_free_callback_t* free_callback;
};

extern struct object object_init(
    enum object_type type,
    object_inspect_callback_t* inspect_callback,
    object_free_callback_t* free_callback
);
extern struct string object_inspect(const struct object* object);
extern void object_free(struct object* object);

struct object_int64 {
    struct object object;
    int64_t value;
};

extern struct object_int64* object_int64_init(int64_t value);
static inline struct object* object_int64_init_base(int64_t value) {
    return &object_int64_init(value)->object;
}

struct object_boolean {
    struct object object;
    bool value;
};

extern struct object_boolean* object_boolean_init(bool value);
static inline struct object* object_boolean_init_base(bool value) {
    return &object_boolean_init(value)->object;
}

struct object_null {
    struct object object;
};

extern struct object_null* object_null_init(void);
static inline struct object* object_null_init_base(void) {
    return &object_null_init()->object;
}

#endif  // MONKEY_OBJECT_H_
