#include "monkey/test/evaluator.h"

#include <inttypes.h>

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
        evaluated && evaluated->type == OBJECT_INT64,
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
        evaluated && evaluated->type == OBJECT_BOOLEAN,
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

static TEST_FUNC(state, integer_expression, struct string input, int64_t expected) {
    struct object* evaluated = test_eval(input);
    RUN_SUBTEST(
        state,
        integer_object,
        CLEANUP(object_free(evaluated); free(evaluated)),
        evaluated,
        expected
    );
    object_free(evaluated);
    free(evaluated);
    PASS();
}

static TEST_FUNC(state, boolean_expression, struct string input, bool expected) {
    struct object* evaluated = test_eval(input);
    RUN_SUBTEST(
        state,
        boolean_object,
        CLEANUP(object_free(evaluated); free(evaluated)),
        evaluated,
        expected
    );
    object_free(evaluated);
    free(evaluated);
    PASS();
}

SUITE_FUNC(state, evaluator) {
    struct {
        struct string input;
        int64_t expected;
    } integer_expression_tests[] = {
        {S("5"), 5},
        {S("10"), 10},
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
}
