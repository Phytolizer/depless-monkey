#include "monkey/test/evaluator.h"

#include <inttypes.h>

#include "monkey/evaluator.h"
#include "monkey/lexer.h"
#include "monkey/object.h"
#include "monkey/parser.h"
#include "monkey/test/value.h"

#define S(x) STRING_REF(x)

static struct object* test_eval(struct string input) {
    struct lexer l;
    lexer_init(&l, input);
    struct parser p;
    parser_init(&p, &l);
    struct ast_program* program = parse_program(&p);

    return eval(&program->node);
}

static SUBTEST_FUNC(state, integer_object, struct object* evaluated, int64_t expected) {
    TEST_ASSERT(
        state,
        evaluated->type == OBJECT_INT64,
        NO_CLEANUP,
        "object is not integer. got=" STRING_FMT,
        STRING_ARG(object_type_string(evaluated->type))
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

static TEST_FUNC(state, integer_expression, struct string input, int64_t expected) {
    struct object* evaluated = test_eval(input);
    RUN_SUBTEST(state, integer_object, CLEANUP(object_free(evaluated)), evaluated, expected);
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
    };
    for (size_t i = 0; i < sizeof(integer_expression_tests) / sizeof(*integer_expression_tests);
         i++) {
        RUN_TEST(
            state,
            integer_expression,
            STRING_REF("integer expression"),
            integer_expression_tests[i].input,
            integer_expression_tests[i].expected
        );
    }
}
