#include "monkey/test/evaluator.h"

#include <inttypes.h>
#include <iso646.h>

#include "monkey/evaluator.h"
#include "monkey/lexer.h"
#include "monkey/object.h"
#include "monkey/parser.h"
#include "monkey/test/value.h"

#define S(x) STRING_REF(x)

static struct string show_obj_type(struct object* obj) {
    return obj != NULL ? object_type_string(obj->type) : S("<NULL>");
}

static struct object* test_eval(struct string input) {
    struct lexer l;
    lexer_init(&l, input);
    struct parser p;
    parser_init(&p, &l);
    struct ast_program* program = parse_program(&p);

    struct object* result = eval(&program->node);
    ast_node_free(&program->node);
    return result;
}

static SUBTEST_FUNC(state, integer_object, struct object* evaluated, int64_t expected) {
    TEST_ASSERT(
        state,
        evaluated and evaluated->type == OBJECT_INTEGER,
        NO_CLEANUP,
        "object is not integer. got=" STRING_FMT,
        STRING_ARG(show_obj_type(evaluated))
    );
    struct object_int64* int_obj = (struct object_int64*)evaluated;
    TEST_ASSERT(
        state,
        int_obj->value == expected,
        NO_CLEANUP,
        "object has wrong value. got=%" PRId64 ", want=%" PRId64,
        int_obj->value,
        expected
    );

    PASS();
}

static SUBTEST_FUNC(state, boolean_object, struct object* evaluated, bool expected) {
    TEST_ASSERT(
        state,
        evaluated and evaluated->type == OBJECT_BOOLEAN,
        NO_CLEANUP,
        "object is not boolean. got=" STRING_FMT,
        STRING_ARG(show_obj_type(evaluated))
    );
    struct object_boolean* bool_obj = (struct object_boolean*)evaluated;
    TEST_ASSERT(
        state,
        bool_obj->value == expected,
        NO_CLEANUP,
        "object has wrong value. got=%s, want=%s",
        bool_obj->value ? "true" : "false",
        expected ? "true" : "false"
    );

    PASS();
}

static SUBTEST_FUNC(state, null_object, struct object* evaluated) {
    TEST_ASSERT(
        state,
        evaluated and evaluated->type == OBJECT_NULL,
        NO_CLEANUP,
        "object is not null. got=" STRING_FMT,
        STRING_ARG(show_obj_type(evaluated))
    );
    PASS();
}

static TEST_FUNC(state, integer_expression, struct string input, int64_t expected) {
    struct object* evaluated = test_eval(input);
    RUN_SUBTEST(state, integer_object, CLEANUP(object_free(evaluated)), evaluated, expected);
    object_free(evaluated);
    PASS();
}

static TEST_FUNC(state, boolean_expression, struct string input, bool expected) {
    struct object* evaluated = test_eval(input);
    RUN_SUBTEST(state, boolean_object, CLEANUP(object_free(evaluated)), evaluated, expected);
    object_free(evaluated);
    PASS();
}

static TEST_FUNC(state, object, struct string input, struct test_value expected) {
    struct object* evaluated = test_eval(input);
    switch (expected.type) {
        case TEST_VALUE_NULL:
            RUN_SUBTEST(state, null_object, CLEANUP(object_free(evaluated)), evaluated);
            break;
        case TEST_VALUE_INT64:
            RUN_SUBTEST(
                state,
                integer_object,
                CLEANUP(object_free(evaluated)),
                evaluated,
                expected.int64
            );
            break;
        case TEST_VALUE_BOOLEAN:
            RUN_SUBTEST(
                state,
                boolean_object,
                CLEANUP(object_free(evaluated)),
                evaluated,
                expected.boolean
            );
            break;
        case TEST_VALUE_STRING:
            // [TODO] implement string object checking
            FAIL(state, CLEANUP(object_free(evaluated)), "string object checking not implemented");
    }
    object_free(evaluated);
    PASS();
}

static TEST_FUNC(state, error, struct string input, struct string expected_message) {
    struct object* evaluated = test_eval(input);

    TEST_ASSERT(
        state,
        evaluated and evaluated->type == OBJECT_ERROR,
        CLEANUP(object_free(evaluated)),
        "object is not error. got=" STRING_FMT,
        STRING_ARG(show_obj_type(evaluated))
    );

    struct object_error* error = (struct object_error*)evaluated;

    TEST_ASSERT(
        state,
        STRING_EQUAL(error->message, expected_message),
        CLEANUP(object_free(evaluated)),
        "wrong error message. expected=\"" STRING_FMT "\", got=\"" STRING_FMT "\"",
        STRING_ARG(expected_message),
        STRING_ARG(error->message)
    );

    object_free(evaluated);
    PASS();
}

SUITE_FUNC(state, evaluator) {
    struct {
        struct string input;
        int64_t expected;
    } integer_expression_tests[] = {
        {S("5"), 5},
        {S("10"), 10},
        {S("-5"), -5},
        {S("-10"), -10},
        {S("5 + 5 + 5 + 5 - 10"), 10},
        {S("2 * 2 * 2 * 2 * 2"), 32},
        {S("-50 + 100 + -50"), 0},
        {S("5 * 2 + 10"), 20},
        {S("5 + 2 * 10"), 25},
        {S("20 + 2 * -10"), 0},
        {S("50 / 2 * 2 + 10"), 60},
        {S("2 * (5 + 10)"), 30},
        {S("3 * 3 * 3 + 10"), 37},
        {S("3 * (3 * 3) + 10"), 37},
        {S("(5 + 10 * 2 + 15 / 3) * 2 + -10"), 50},
    };
    for (size_t i = 0; i < sizeof(integer_expression_tests) / sizeof(*integer_expression_tests);
         i++) {
        RUN_TEST(
            state,
            integer_expression,
            string_printf(
                "integer expression (\"" STRING_FMT "\")",
                STRING_ARG(integer_expression_tests[i].input)
            ),
            integer_expression_tests[i].input,
            integer_expression_tests[i].expected
        );
    }

    struct {
        struct string input;
        bool expected;
    } boolean_expression_tests[] = {
        {S("true"), true},
        {S("false"), false},
        {S("1 < 2"), true},
        {S("1 > 2"), false},
        {S("1 < 1"), false},
        {S("1 > 1"), false},
        {S("1 == 1"), true},
        {S("1 != 1"), false},
        {S("1 == 2"), false},
        {S("1 != 2"), true},
        {S("true == true"), true},
        {S("false == false"), true},
        {S("true == false"), false},
        {S("true != false"), true},
        {S("false != true"), true},
        {S("(1 < 2) == true"), true},
        {S("(1 < 2) == false"), false},
        {S("(1 > 2) == true"), false},
        {S("(1 > 2) == false"), true},
    };
    for (size_t i = 0; i < sizeof(boolean_expression_tests) / sizeof(*boolean_expression_tests);
         i++) {
        RUN_TEST(
            state,
            boolean_expression,
            string_printf(
                "boolean expression (\"" STRING_FMT "\")",
                STRING_ARG(boolean_expression_tests[i].input)
            ),
            boolean_expression_tests[i].input,
            boolean_expression_tests[i].expected
        );
    }

    struct {
        struct string input;
        bool expected;
    } bang_operator_tests[] = {
        {S("!true"), false},
        {S("!false"), true},
        {S("!5"), false},
        {S("!!true"), true},
        {S("!!false"), false},
        {S("!!5"), true},
    };
    for (size_t i = 0; i < sizeof(bang_operator_tests) / sizeof(*bang_operator_tests); i++) {
        RUN_TEST(
            state,
            boolean_expression,
            string_printf(
                "bang operator (\"" STRING_FMT "\")",
                STRING_ARG(bang_operator_tests[i].input)
            ),
            bang_operator_tests[i].input,
            bang_operator_tests[i].expected
        );
    }

    struct {
        struct string input;
        struct test_value expected;
    } if_else_expression_tests[] = {
        {S("if (true) { 10 }"), test_value_int64(10)},
        {S("if (false) { 10 }"), test_value_null()},
        {S("if (1) { 10 }"), test_value_int64(10)},
        {S("if (1 < 2) { 10 }"), test_value_int64(10)},
        {S("if (1 > 2) { 10 }"), test_value_null()},
        {S("if (1 > 2) { 10 } else { 20 }"), test_value_int64(20)},
        {S("if (1 < 2) { 10 } else { 20 }"), test_value_int64(10)},
    };
    for (size_t i = 0; i < sizeof(if_else_expression_tests) / sizeof(*if_else_expression_tests);
         i++) {
        RUN_TEST(
            state,
            object,
            string_printf(
                "if/else expression (\"" STRING_FMT "\")",
                STRING_ARG(if_else_expression_tests[i].input)
            ),
            if_else_expression_tests[i].input,
            if_else_expression_tests[i].expected
        );
    }

    struct {
        struct string input;
        int64_t expected;
    } return_statement_tests[] = {
        {S("return 10;"), 10},
        {S("return 10; 9;"), 10},
        {S("return 2 * 5; 9;"), 10},
        {S("9; return 2 * 5; 9;"), 10},
        {S("if (10 > 1) {\n"
           "  if (10 > 1) {\n"
           "    return 10;\n"
           "  }\n"
           "  return 1;\n"
           "}\n"),
         10},
    };
    for (size_t i = 0; i < sizeof(return_statement_tests) / sizeof(*return_statement_tests); i++) {
        RUN_TEST(
            state,
            integer_expression,
            string_printf(
                "return statement (\"" STRING_FMT "\")",
                STRING_ARG(return_statement_tests[i].input)
            ),
            return_statement_tests[i].input,
            return_statement_tests[i].expected
        );
    }

    struct {
        struct string input;
        struct string expected_message;
    } error_handling_tests[] = {
        {S("5 + true;"), S("type mismatch: INTEGER + BOOLEAN")},
        {S("5 + true; 5;"), S("type mismatch: INTEGER + BOOLEAN")},
        {S("-true"), S("unknown operator: -BOOLEAN")},
        {S("true + false;"), S("unknown operator: BOOLEAN + BOOLEAN")},
        {S("5; true + false; 5"), S("unknown operator: BOOLEAN + BOOLEAN")},
        {S("if (10 > 1) { true + false; }"), S("unknown operator: BOOLEAN + BOOLEAN")},
        {S("if (10 > 1) {\n"
           "  if (10 > 1) {\n"
           "    return true + false;\n"
           "  }\n"
           "  return 1;\n"
           "}\n"),
         S("unknown operator: BOOLEAN + BOOLEAN")},
    };
    for (size_t i = 0; i < sizeof(error_handling_tests) / sizeof(*error_handling_tests); i++) {
        RUN_TEST(
            state,
            error,
            string_printf(
                "error handling (\"" STRING_FMT "\")",
                STRING_ARG(error_handling_tests[i].input)
            ),
            error_handling_tests[i].input,
            error_handling_tests[i].expected_message
        );
    }
}
