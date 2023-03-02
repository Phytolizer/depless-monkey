#ifndef MONKEY_OBJECT_H_
#define MONKEY_OBJECT_H_

#include <stdbool.h>
#include <stdint.h>

#include "monkey/ast.h"
#include "monkey/buf.h"
#include "monkey/string.h"

enum object_type {
#define X(x) OBJECT_##x,
#include "monkey/private/object_types.inc"
#undef X
};

extern struct string object_type_string(enum object_type type);

struct object;

struct object_hash_key {
    enum object_type type;
    uint64_t value;
};

extern bool object_hash_key_equal(struct object_hash_key a, struct object_hash_key b);

typedef struct string object_inspect_callback_t(const struct object* object);
typedef void object_free_callback_t(struct object* object);
typedef struct object* object_dup_callback_t(const struct object* object);
typedef struct object_hash_key object_hash_key_callback_t(const struct object* object);

struct object {
    enum object_type type;
    object_inspect_callback_t* inspect_callback;
    object_free_callback_t* free_callback;
    object_dup_callback_t* dup_callback;
    object_hash_key_callback_t* hash_key_callback;
};

extern struct object object_init(
    enum object_type type,
    object_inspect_callback_t* inspect_callback,
    object_free_callback_t* free_callback,
    object_dup_callback_t* dup_callback,
    object_hash_key_callback_t* hash_key_callback
);
extern struct string object_inspect(const struct object* object);
extern void object_free(struct object* object);
extern struct object* object_dup(const struct object* object);
extern struct object_hash_key object_hash_key(const struct object* object);
static inline bool object_is_hashable(const struct object* object) {
    return object->hash_key_callback != NULL;
}

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

struct object_return_value {
    struct object object;
    struct object* value;
};

extern struct object_return_value* object_return_value_init(struct object* value);
static inline struct object* object_return_value_init_base(struct object* value) {
    return &object_return_value_init(value)->object;
}
extern struct object* object_return_value_unwrap(struct object* object);

struct object_error {
    struct object object;
    struct string message;
};

extern struct object_error* object_error_init(struct string message);
static inline struct object* object_error_init_base(struct string message) {
    return &object_error_init(message)->object;
}

struct environment;

struct object_function {
    struct object object;
    struct function_parameter_buf parameters;
    struct ast_block_statement* body;
    struct environment* env;
};

extern struct object_function* object_function_init(
    struct function_parameter_buf parameters,
    struct ast_block_statement* body,
    struct environment* env
);
static inline struct object* object_function_init_base(
    struct function_parameter_buf parameters,
    struct ast_block_statement* body,
    struct environment* env
) {
    return &object_function_init(parameters, body, env)->object;
}

struct object_string {
    struct object object;
    struct string value;
};

extern struct object_string* object_string_init(struct string value);
static inline struct object* object_string_init_base(struct string value) {
    return &object_string_init(value)->object;
}

BUF_T(struct object*, object);
typedef struct object* builtin_function_callback_t(struct object_buf args);

struct object_builtin {
    struct object object;
    builtin_function_callback_t* fn;
};

extern struct object_builtin* object_builtin_init(builtin_function_callback_t* fn);
static inline struct object* object_builtin_init_base(builtin_function_callback_t* fn) {
    return &object_builtin_init(fn)->object;
}

struct object_array {
    struct object object;
    struct object_buf elements;
};

extern struct object_array* object_array_init(struct object_buf elements);
static inline struct object* object_array_init_base(struct object_buf elements) {
    return &object_array_init(elements)->object;
}

struct object_hash_pair {
    struct object* key;
    struct object* value;
};

struct object_hash_bucket {
    struct object_hash_key key;
    struct object_hash_pair value;
};

BUF_T(struct object_hash_bucket, object_hash_bucket);

struct object_hash_table {
    struct object_hash_bucket_buf buckets;
    size_t count;
};

extern void object_hash_table_init(struct object_hash_table* table);
extern void object_hash_table_free(struct object_hash_table* table);
extern void object_hash_table_insert(
    struct object_hash_table* table,
    struct object_hash_key hash_key,
    struct object* key,
    struct object* value
);
extern struct object_hash_pair*
object_hash_table_get(struct object_hash_table* table, struct object* key);

struct object_hash {
    struct object object;
    struct object_hash_table pairs;
};

extern struct object_hash* object_hash_init(struct object_hash_table pairs);
static inline struct object* object_hash_init_base(struct object_hash_table pairs) {
    return &object_hash_init(pairs)->object;
}

#endif  // MONKEY_OBJECT_H_
