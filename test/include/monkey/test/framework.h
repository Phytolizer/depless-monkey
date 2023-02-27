#ifndef MONKEY_TEST_FRAMEWORK_H_
#define MONKEY_TEST_FRAMEWORK_H_

#include <monkey/string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifndef TEST_ABORT_ON_FAILURE
#define TEST_ABORT_ON_FAILURE 0
#endif

struct test_state {
    uint64_t passed;
    uint64_t failed;
    uint64_t skipped;
    uint64_t assertions;
    bool verbose;
};

#define Z_TEST_RESULTS \
    X(PASS) \
    X(FAIL) \
    X(SKIP)

enum test_result_type {
#define X(x) TEST_RESULT_##x,
    Z_TEST_RESULTS
#undef X
};

struct test_result {
    enum test_result_type type;
    struct string error_message;
};

#define TEST_FAIL(msg) ((struct test_result){.type = TEST_RESULT_FAIL, .error_message = msg})
#define TEST_PASS() ((struct test_result){.type = TEST_RESULT_PASS})
#define TEST_SKIP() ((struct test_result){.type = TEST_RESULT_SKIP})

#if TEST_ABORT_ON_FAILURE
#define TEST_ABORT() abort()
#else
#define TEST_ABORT()
#endif

#define FAIL(state, cleanup, ...) \
    do { \
        struct string message_ = string_printf(__VA_ARGS__); \
        cleanup; \
        TEST_ABORT(); \
        return TEST_FAIL(message_); \
    } while (false)

#define TEST_ASSERT(state, test, cleanup, ...) \
    do { \
        ++(state)->assertions; \
        if (!(test)) { \
            FAIL(state, cleanup, __VA_ARGS__); \
        } \
    } while (false)

#define NO_CLEANUP (void)0
#define CLEANUP(x) x

#define RUN_SUITE(state, name, displayname) \
    do { \
        (void)fprintf(stderr, "SUITE " STRING_FMT "\n", STRING_ARG(displayname)); \
        STRING_FREE(displayname); \
        name##_test_suite(state); \
    } while (false)

#define TEST_CALL(state, name, ...) name##_test(state, __VA_ARGS__)
#define TEST_CALL0(state, name) name##_test(state)

#define DO_RUN_TEST(state, name, call, displayname) \
    do { \
        struct string disp = displayname; \
        if ((state)->verbose) { \
            (void)fprintf(stderr, "TEST  " STRING_FMT "\n", STRING_ARG(disp)); \
        } \
        struct test_result result = call; \
        switch (result.type) { \
            case TEST_RESULT_FAIL: \
                ++(state)->failed; \
                (void)fprintf( \
                    stderr, \
                    "FAIL  " STRING_FMT ": " STRING_FMT "\n", \
                    STRING_ARG(disp), \
                    STRING_ARG(result.error_message) \
                ); \
                STRING_FREE(result.error_message); \
                break; \
            case TEST_RESULT_PASS: \
                ++(state)->passed; \
                break; \
            case TEST_RESULT_SKIP: \
                ++(state)->skipped; \
                (void)fprintf(stderr, "SKIP  " STRING_FMT "\n", STRING_ARG(disp)); \
                break; \
        } \
        STRING_FREE(disp); \
    } while (false)

#define RUN_TEST(state, name, displayname, ...) \
    DO_RUN_TEST(state, name, TEST_CALL(state, name, __VA_ARGS__), displayname)

#define RUN_TEST0(state, name, displayname) \
    DO_RUN_TEST(state, name, TEST_CALL0(state, name), displayname)

#define DO_RUN_SUBTEST(state, name, call, cleanup) \
    do { \
        struct test_result result = call; \
        if (result.type != TEST_RESULT_PASS) { \
            cleanup; \
            return result; \
        } \
    } while (false)

#define SUBTEST_CALL(state, name, ...) name##_subtest(state, __VA_ARGS__)
#define SUBTEST_CALL0(state, name) name##_subtest(state)

#define RUN_SUBTEST(state, name, cleanup, ...) \
    DO_RUN_SUBTEST(state, name, SUBTEST_CALL(state, name, __VA_ARGS__), cleanup)

#define RUN_SUBTEST0(state, name, cleanup) \
    DO_RUN_SUBTEST(state, name, SUBTEST_CALL0(state, name), cleanup)

#define SUITE_FUNC(state, name) void name##_test_suite(struct test_state* state)

#define TEST_FUNC(state, name, ...) \
    struct test_result name##_test(struct test_state* state, __VA_ARGS__)
#define TEST_FUNC0(state, name) struct test_result name##_test(struct test_state* state)

#define SUBTEST_FUNC(state, name, ...) \
    struct test_result name##_subtest(struct test_state* state, __VA_ARGS__)
#define SUBTEST_FUNC0(state, name) struct test_result name##_subtest(struct test_state* state)

#define PASS() return TEST_PASS()

#define SKIP() return TEST_SKIP()

#endif  // MONKEY_TEST_FRAMEWORK_H_
