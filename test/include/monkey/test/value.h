#ifndef MONKEY_TEST_VALUE_H_
#define MONKEY_TEST_VALUE_H_

#include <stdbool.h>
#include <stdint.h>

#include "monkey/string.h"

enum test_value_type {
    TEST_VALUE_INT64,
    TEST_VALUE_STRING,
    TEST_VALUE_BOOLEAN,
};

struct test_value {
    enum test_value_type type;
    union {
        int64_t int64;
        struct string string;
        bool boolean;
    };
};

static inline struct test_value test_value_int64(int64_t value) {
    return (struct test_value){TEST_VALUE_INT64, .int64 = value};
}

static inline struct test_value test_value_string(struct string value) {
    return (struct test_value){TEST_VALUE_STRING, .string = value};
}

static inline struct test_value test_value_boolean(bool value) {
    return (struct test_value){TEST_VALUE_BOOLEAN, .boolean = value};
}

#endif  // MONKEY_TEST_VALUE_H_
