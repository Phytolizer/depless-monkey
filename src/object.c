#include "monkey/object.h"

#include <inttypes.h>
#include <stdlib.h>

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
    object_free_callback_t* free_callback
) {
    struct object object = {
        .type = type,
        .inspect_callback = inspect_callback,
        .free_callback = free_callback,
    };
    return object;
}

struct string object_inspect(const struct object* object) {
    return object->inspect_callback(object);
}

void object_free(struct object* object) {
    if (object == NULL) return;
    object->free_callback(object);
}

static struct string int64_inspect(const struct object* obj) {
    const struct object_int64* self = (const struct object_int64*)obj;
    return string_printf("%" PRId64, self->value);
}

static void int64_free(MONKEY_UNUSED struct object* obj) {}

struct object_int64* object_int64_init(int64_t value) {
    struct object_int64* self = malloc(sizeof(*self));
    self->object = object_init(OBJECT_INT64, int64_inspect, int64_free);
    self->value = value;
    return self;
}

static struct string boolean_inspect(const struct object* obj) {
    const struct object_boolean* self = (const struct object_boolean*)obj;
    return string_printf("%s", self->value ? "true" : "false");
}

static void boolean_free(MONKEY_UNUSED struct object* obj) {}

struct object_boolean* object_boolean_init(bool value) {
    struct object_boolean* self = malloc(sizeof(*self));
    self->object = object_init(OBJECT_BOOLEAN, boolean_inspect, boolean_free);
    self->value = value;
    return self;
}

static struct string null_inspect(MONKEY_UNUSED const struct object* obj) {
    return STRING_REF("null");
}

static void null_free(MONKEY_UNUSED struct object* obj) {}

extern struct object_null* object_null_init(void) {
    struct object_null* self = malloc(sizeof(*self));
    self->object = object_init(OBJECT_NULL, null_inspect, null_free);
    return self;
}
