#include "monkey/object.h"

#include <inttypes.h>
#include <stdlib.h>

#include "monkey/environment.h"
#include "monkey/private/stdc.h"

#define DOWNCAST(T, obj) \
    ({ \
        if (obj == NULL) return; \
        (T*)obj; \
    })

struct string object_type_string(enum object_type type) {
    switch (type) {
#define X(x) \
    case OBJECT_##x: \
        return STRING_REF(#x);
#include "monkey/private/object_types.inc"
#undef X
    }
    abort();
}

struct object object_init(
    enum object_type type,
    object_inspect_callback_t* inspect_callback,
    object_free_callback_t* free_callback,
    object_dup_callback_t* dup_callback
) {
    struct object object = {
        .type = type,
        .inspect_callback = inspect_callback,
        .free_callback = free_callback,
        .dup_callback = dup_callback,
    };
    return object;
}

struct string object_inspect(const struct object* object) {
    return object->inspect_callback(object);
}

void object_free(struct object* object) {
    if (object == NULL) return;
    object->free_callback(object);
    free(object);
}

struct object* object_dup(const struct object* object) {
    if (object == NULL) return NULL;
    return object->dup_callback(object);
}

static struct string int64_inspect(const struct object* obj) {
    const struct object_int64* self = (const struct object_int64*)obj;
    return string_printf("%" PRId64, self->value);
}

static void int64_free(MONKEY_UNUSED struct object* obj) {}
static struct object* int64_dup(const struct object* obj) {
    const struct object_int64* self = (const struct object_int64*)obj;
    return object_int64_init_base(self->value);
}

struct object_int64* object_int64_init(int64_t value) {
    struct object_int64* self = malloc(sizeof(*self));
    self->object = object_init(OBJECT_INTEGER, int64_inspect, int64_free, int64_dup);
    self->value = value;
    return self;
}

static struct string boolean_inspect(const struct object* obj) {
    const struct object_boolean* self = (const struct object_boolean*)obj;
    return string_printf("%s", self->value ? "true" : "false");
}

static void boolean_free(MONKEY_UNUSED struct object* obj) {}

static struct object* boolean_dup(const struct object* obj) {
    const struct object_boolean* self = (const struct object_boolean*)obj;
    return object_boolean_init_base(self->value);
}

struct object_boolean* object_boolean_init(bool value) {
    struct object_boolean* self = malloc(sizeof(*self));
    self->object = object_init(OBJECT_BOOLEAN, boolean_inspect, boolean_free, boolean_dup);
    self->value = value;
    return self;
}

static struct string null_inspect(MONKEY_UNUSED const struct object* obj) {
    return STRING_REF("null");
}

static void null_free(MONKEY_UNUSED struct object* obj) {}

static struct object* null_dup(MONKEY_UNUSED const struct object* obj) {
    return object_null_init_base();
}

struct object_null* object_null_init(void) {
    struct object_null* self = malloc(sizeof(*self));
    self->object = object_init(OBJECT_NULL, null_inspect, null_free, null_dup);
    return self;
}

static struct string return_value_inspect(const struct object* obj) {
    const struct object_return_value* self = (const struct object_return_value*)obj;
    return object_inspect(self->value);
}

static void return_value_free(struct object* obj) {
    struct object_return_value* self = DOWNCAST(struct object_return_value, obj);
    object_free(self->value);
}

static struct object* return_value_dup(const struct object* obj) {
    const struct object_return_value* self = (const struct object_return_value*)obj;
    return object_return_value_init_base(object_dup(self->value));
}

struct object_return_value* object_return_value_init(struct object* value) {
    struct object_return_value* self = malloc(sizeof(*self));
    self->object =
        object_init(OBJECT_RETURN_VALUE, return_value_inspect, return_value_free, return_value_dup);
    self->value = value;
    return self;
}

struct object* object_return_value_unwrap(struct object* object) {
    if (object == NULL) return NULL;
    struct object_return_value* self = (struct object_return_value*)object;
    struct object* value = self->value;
    self->value = NULL;
    object_free(object);
    return value;
}

static struct string error_inspect(const struct object* obj) {
    auto self = (const struct object_error*)obj;
    return string_printf("ERROR: " STRING_FMT, STRING_ARG(self->message));
}

static void error_free(struct object* obj) {
    auto self = DOWNCAST(struct object_error, obj);
    STRING_FREE(self->message);
}

static struct object* error_dup(const struct object* obj) {
    auto self = (const struct object_error*)obj;
    return object_error_init_base(self->message);
}

struct object_error* object_error_init(struct string message) {
    struct object_error* self = malloc(sizeof(*self));
    self->object = object_init(OBJECT_ERROR, error_inspect, error_free, error_dup);
    self->message = message;
    return self;
}

static struct string function_inspect(const struct object* obj) {
    auto self = (const struct object_function*)obj;
    struct string out = string_dup(STRING_REF("fn("));
    for (size_t i = 0; i < self->parameters.len; i++) {
        struct string param = ast_expression_string(&self->parameters.ptr[i]->expression);
        string_append(&out, param);
        if (i < self->parameters.len - 1) {
            string_append(&out, STRING_REF(", "));
        }
        STRING_FREE(param);
    }
    string_append(&out, STRING_REF(") {\n"));
    struct string body_str = ast_statement_string(&self->body->statement);
    string_append(&out, body_str);
    STRING_FREE(body_str);
    string_append(&out, STRING_REF("\n}"));
    return out;
}

static void function_free(struct object* obj) {
    auto self = DOWNCAST(struct object_function, obj);
    if (ast_statement_decref(&self->body->statement) == 0) {
        free(self->body);
    }
    for (size_t i = 0; i < self->parameters.len; i++) {
        if (ast_expression_decref(&self->parameters.ptr[i]->expression) == 0) {
            free(self->parameters.ptr[i]);
        }
    }
    BUF_FREE(self->parameters);
}

static struct object* function_dup(const struct object* obj) {
    auto self = (const struct object_function*)obj;
    auto body = (struct ast_block_statement*)ast_node_incref(&self->body->statement.node);
    struct function_parameter_buf parameters = {0};
    for (size_t i = 0; i < self->parameters.len; i++) {
        auto id =
            (struct ast_identifier*)ast_node_incref(&self->parameters.ptr[i]->expression.node);
        BUF_PUSH(&parameters, id);
    }
    return object_function_init_base(parameters, body, self->env);
}

extern struct object_function* object_function_init(
    struct function_parameter_buf parameters,
    struct ast_block_statement* body,
    struct environment* env
) {
    struct object_function* self = malloc(sizeof(*self));
    self->object = object_init(OBJECT_FUNCTION, function_inspect, function_free, function_dup);
    self->parameters = parameters;
    self->body = body;
    self->env = env;
    return self;
}

static struct string string_inspect(const struct object* obj) {
    auto self = (const struct object_string*)obj;
    return string_dup(self->value);
}

static void string_free(struct object* obj) {
    auto self = DOWNCAST(struct object_string, obj);
    STRING_FREE(self->value);
}

static struct object* o_string_dup(const struct object* obj) {
    auto self = (const struct object_string*)obj;
    return object_string_init_base(self->value);
}

struct object_string* object_string_init(struct string value) {
    struct object_string* self = malloc(sizeof(*self));
    self->object = object_init(OBJECT_STRING, string_inspect, string_free, o_string_dup);
    self->value = value;
    return self;
}

static struct string builtin_inspect(MONKEY_UNUSED const struct object* obj) {
    return STRING_REF("builtin function");
}

static void builtin_free(MONKEY_UNUSED struct object* obj) {
    // nothing to do
}

static struct object* builtin_dup(const struct object* obj) {
    auto self = (const struct object_builtin*)obj;
    return object_builtin_init_base(self->fn);
}

struct object_builtin* object_builtin_init(builtin_function_callback_t* fn) {
    struct object_builtin* self = malloc(sizeof(*self));
    self->object = object_init(OBJECT_BUILTIN, builtin_inspect, builtin_free, builtin_dup);
    self->fn = fn;
    return self;
}
