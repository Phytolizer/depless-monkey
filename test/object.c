#include "monkey/test/object.h"

#include <monkey/object.h>

#include "monkey/test/framework.h"

static TEST_FUNC0(state, hash_key) {
    struct object* hello1 = object_string_init_base(STRING_REF("Hello World"));
    struct object* hello2 = object_string_init_base(STRING_REF("Hello World"));
    struct object* diff1 = object_string_init_base(STRING_REF("My name is johnny"));
    struct object* diff2 = object_string_init_base(STRING_REF("My name is johnny"));

    TEST_ASSERT(
        state,
        object_hash_key_equal(object_hash_key(hello1), object_hash_key(hello2)),
        CLEANUP(object_free(hello1); object_free(hello2); object_free(diff1); object_free(diff2)),
        "strings with same content have different hash keys"
    );
    TEST_ASSERT(
        state,
        object_hash_key_equal(object_hash_key(diff1), object_hash_key(diff2)),
        CLEANUP(object_free(hello1); object_free(hello2); object_free(diff1); object_free(diff2)),
        "strings with same content have different hash keys"
    );
    TEST_ASSERT(
        state,
        !object_hash_key_equal(object_hash_key(hello1), object_hash_key(diff1)),
        CLEANUP(object_free(hello1); object_free(hello2); object_free(diff1); object_free(diff2)),
        "strings with different content have same hash keys"
    );

    object_free(hello1);
    object_free(hello2);
    object_free(diff1);
    object_free(diff2);
    PASS();
}

SUITE_FUNC(state, object) {
    RUN_TEST0(state, hash_key, STRING_REF("hash_key()"));
}
