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
    if (obj == NULL) return S("<NULL>");
    switch (obj->type) {
        case OBJECT_ERROR:
            return string_printf(
                "ERROR: " STRING_FMT,
                STRING_ARG(((struct object_error*)obj)->message)
            );
        default:
            return object_type_string(obj->type);
    }
}

static struct object* test_eval(struct string input) {
    struct lexer l;
    lexer_init(&l, input);
    struct parser p;
    parser_init(&p, &l);
    struct ast_program* program = parse_program(&p);
    struct environment env;
    environment_init(&env);

    struct object* result = eval(&program->node, &env);
    environment_free(env);
    ast_node_decref(&program->node);
    return result;
}

static SUBTEST_FUNC(state, integer_object, struct object* evaluated, int64_t expected) {
    struct string type = show_obj_type(evaluated);
    TEST_ASSERT(
        state,
        evaluated and evaluated->type == OBJECT_INTEGER,
        CLEANUP(STRING_FREE(type)),
        "object is not integer. got=" STRING_FMT,
        STRING_ARG(type)
    );
    STRING_FREE(type);
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
    struct string type = show_obj_type(evaluated);
    TEST_ASSERT(
        state,
        evaluated and evaluated->type == OBJECT_BOOLEAN,
        CLEANUP(STRING_FREE(type)),
        "object is not boolean. got=" STRING_FMT,
        STRING_ARG(type)
    );
    STRING_FREE(type);
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
    struct string type = show_obj_type(evaluated);
    TEST_ASSERT(
        state,
        evaluated and evaluated->type == OBJECT_NULL,
        CLEANUP(STRING_FREE(type)),
        "object is not null. got=" STRING_FMT,
        STRING_ARG(type)
    );
    STRING_FREE(type);
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

static SUBTEST_FUNC(state, error, struct object* evaluated, struct string expected_message) {
    struct string type = show_obj_type(evaluated);
    TEST_ASSERT(
        state,
        evaluated and evaluated->type == OBJECT_ERROR,
        CLEANUP(STRING_FREE(type)),
        "object is not error. got=" STRING_FMT,
        STRING_ARG(type)
    );
    STRING_FREE(type);

    struct object_error* error = (struct object_error*)evaluated;

    TEST_ASSERT(
        state,
        STRING_EQUAL(error->message, expected_message),
        NO_CLEANUP,
        "wrong error message. expected=\"" STRING_FMT "\", got=\"" STRING_FMT "\"",
        STRING_ARG(expected_message),
        STRING_ARG(error->message)
    );

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
        case TEST_VALUE_ERROR:
            RUN_SUBTEST(state, error, CLEANUP(object_free(evaluated)), evaluated, expected.string);
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

    RUN_SUBTEST(state, error, CLEANUP(object_free(evaluated)), evaluated, expected_message);

    object_free(evaluated);
    PASS();
}

static TEST_FUNC0(state, function_object) {
    const struct string input = S("fn(x) { x + 2; };");
    struct object* evaluated = test_eval(input);

    struct string type = show_obj_type(evaluated);
    TEST_ASSERT(
        state,
        evaluated and evaluated->type == OBJECT_FUNCTION,
        CLEANUP(STRING_FREE(type); object_free(evaluated)),
        "object is not function. got=" STRING_FMT,
        STRING_ARG(type)
    );
    STRING_FREE(type);

    struct object_function* function = (struct object_function*)evaluated;

    TEST_ASSERT(
        state,
        function->parameters.len == 1,
        CLEANUP(object_free(evaluated)),
        "function has wrong parameters. Parameters=%zu",
        function->parameters.len
    );

    struct string param_str = ast_expression_string(&function->parameters.ptr[0]->expression);
    TEST_ASSERT(
        state,
        STRING_EQUAL(param_str, S("x")),
        CLEANUP(STRING_FREE(param_str); object_free(evaluated)),
        "parameter is not 'x'. got=\"" STRING_FMT "\"",
        STRING_ARG(param_str)
    );
    STRING_FREE(param_str);

    struct string body_str = ast_statement_string(&function->body->statement);
    struct string expected_body = S("(x + 2)");
    TEST_ASSERT(
        state,
        STRING_EQUAL(body_str, expected_body),
        CLEANUP(STRING_FREE(body_str); object_free(evaluated)),
        "body is not \"" STRING_FMT "\". got=\"" STRING_FMT "\"",
        STRING_ARG(expected_body),
        STRING_ARG(body_str)
    );
    STRING_FREE(body_str);

    object_free(evaluated);
    PASS();
}

static TEST_FUNC(state, string_literal, struct string input, struct string expected) {
    struct object* evaluated = test_eval(input);

    struct string type = show_obj_type(evaluated);
    TEST_ASSERT(
        state,
        evaluated and evaluated->type == OBJECT_STRING,
        CLEANUP(STRING_FREE(type); object_free(evaluated)),
        "object is not string. got=" STRING_FMT,
        STRING_ARG(type)
    );
    STRING_FREE(type);

    struct object_string* string = (struct object_string*)evaluated;

    TEST_ASSERT(
        state,
        STRING_EQUAL(string->value, expected),
        CLEANUP(object_free(evaluated)),
        "string has wrong value. expected=\"" STRING_FMT "\", got=\"" STRING_FMT "\"",
        STRING_ARG(expected),
        STRING_ARG(string->value)
    );

    object_free(evaluated);
    PASS();
}

static TEST_FUNC0(state, array_literals) {
    const struct string input = S("[1, 2 * 2, 3 + 3]");
    struct object* evaluated = test_eval(input);

    struct string type = show_obj_type(evaluated);
    TEST_ASSERT(
        state,
        evaluated and evaluated->type == OBJECT_ARRAY,
        CLEANUP(STRING_FREE(type); object_free(evaluated)),
        "object is not array. got=" STRING_FMT,
        STRING_ARG(type)
    );
    STRING_FREE(type);

    struct object_array* array = (struct object_array*)evaluated;

    TEST_ASSERT(
        state,
        array->elements.len == 3,
        CLEANUP(object_free(evaluated)),
        "array has wrong number of elements. got=%zu",
        array->elements.len
    );

    RUN_SUBTEST(state, integer_object, CLEANUP(object_free(evaluated)), array->elements.ptr[0], 1);
    RUN_SUBTEST(state, integer_object, CLEANUP(object_free(evaluated)), array->elements.ptr[1], 4);
    RUN_SUBTEST(state, integer_object, CLEANUP(object_free(evaluated)), array->elements.ptr[2], 6);

    object_free(evaluated);
    PASS();
}

static TEST_FUNC0(state, hash_literals) {
    const struct string input =
        S("let two = \"two\";"
          "{\n"
          "    \"one\": 10 - 9,\n"
          "    two: 1 + 1,\n"
          "    \"thr\" + \"ee\": 6 / 2,\n"
          "    4: 4,\n"
          "    true: 5,\n"
          "    false: 6\n"
          "}");
    struct object* evaluated = test_eval(input);

    struct string type = show_obj_type(evaluated);
    TEST_ASSERT(
        state,
        evaluated and evaluated->type == OBJECT_HASH,
        CLEANUP(STRING_FREE(type); object_free(evaluated)),
        "object is not hash. got=" STRING_FMT,
        STRING_ARG(type)
    );
    STRING_FREE(type);

    struct object_hash* hash = (struct object_hash*)evaluated;

    TEST_ASSERT(
        state,
        hash->pairs.count == 6,
        CLEANUP(object_free(evaluated)),
        "hash has wrong number of pairs. got=%zu",
        hash->pairs.count
    );

    typedef struct {
        struct object_hash_key key;
        int64_t value;
    } expected_t;
    expected_t expected[] = {
        {.value = 1},
        {.value = 2},
        {.value = 3},
        {.value = 4},
        {.value = 5},
        {.value = 6},
    };
    struct object* temp;
    temp = object_string_init_base(S("one"));
    expected[0].key = object_hash_key(temp);
    object_free(temp);
    temp = object_string_init_base(S("two"));
    expected[1].key = object_hash_key(temp);
    object_free(temp);
    temp = object_string_init_base(S("three"));
    expected[2].key = object_hash_key(temp);
    object_free(temp);
    temp = object_int64_init_base(4);
    expected[3].key = object_hash_key(temp);
    object_free(temp);
    temp = object_boolean_init_base(true);
    expected[4].key = object_hash_key(temp);
    object_free(temp);
    temp = object_boolean_init_base(false);
    expected[5].key = object_hash_key(temp);
    object_free(temp);

    for (size_t i = 0; i < hash->pairs.buckets.len; ++i) {
        struct object_hash_bucket bucket = hash->pairs.buckets.ptr[i];
        if (bucket.value.key == NULL) continue;

        bool found = false;
        for (size_t j = 0; j < sizeof(expected) / sizeof(expected[0]); ++j) {
            if (object_hash_key_equal(bucket.key, expected[j].key)) {
                found = true;
                RUN_SUBTEST(
                    state,
                    integer_object,
                    CLEANUP(object_free(evaluated)),
                    bucket.value.value,
                    expected[j].value
                );
            }
        }

        TEST_ASSERT(state, found, CLEANUP(object_free(evaluated)), "unexpected key in hash");
    }

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
        {S("foobar"), S("identifier not found: foobar")},
        {S("\"Hello\" - \"World\""), S("unknown operator: STRING - STRING")},
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

    struct {
        struct string input;
        int64_t expected;
    } let_statement_tests[] = {
        {S("let a = 5; a;"), 5},
        {S("let a = 5 * 5; a;"), 25},
        {S("let a = 5; let b = a; b;"), 5},
        {S("let a = 5; let b = a; let c = a + b + 5; c;"), 15},
    };
    for (size_t i = 0; i < sizeof(let_statement_tests) / sizeof(*let_statement_tests); i++) {
        RUN_TEST(
            state,
            integer_expression,
            string_printf(
                "let statement (\"" STRING_FMT "\")",
                STRING_ARG(let_statement_tests[i].input)
            ),
            let_statement_tests[i].input,
            let_statement_tests[i].expected
        );
    }

    RUN_TEST0(state, function_object, S("function object"));

    struct {
        struct string input;
        int64_t expected;
    } function_application_tests[] = {
        {S("let identity = fn(x) { x; }; identity(5);"), 5},
        {S("let identity = fn(x) { return x; }; identity(5);"), 5},
        {S("let double = fn(x) { x * 2; }; double(5);"), 10},
        {S("let add = fn(x, y) { x + y; }; add(5, 5);"), 10},
        {S("let add = fn(x, y) { x + y; }; add(5 + 5, add(5, 5));"), 20},
        {S("fn(x) { x; }(5)"), 5},
    };
    for (size_t i = 0; i < sizeof(function_application_tests) / sizeof(*function_application_tests);
         i++) {
        RUN_TEST(
            state,
            integer_expression,
            string_printf(
                "function application (\"" STRING_FMT "\")",
                STRING_ARG(function_application_tests[i].input)
            ),
            function_application_tests[i].input,
            function_application_tests[i].expected
        );
    }
    const struct string closure_input =
        S("let newAdder = fn(x) {\n"
          "  fn(y) { x + y };\n"
          "};\n"
          "\n"
          "let addTwo = newAdder(2);\n"
          "addTwo(2);\n");
    RUN_TEST(state, integer_expression, S("closure"), closure_input, 4);

    RUN_TEST(
        state,
        string_literal,
        S("basic string literal"),
        S("\"hello world\""),
        S("hello world")
    );
    RUN_TEST(
        state,
        string_literal,
        S("string concatenation"),
        S("\"hello\" + \" \" + \"world\""),
        S("hello world")
    );

    struct {
        struct string input;
        struct test_value expected;
    } builtin_function_tests[] = {
        {S("len(\"\")"), test_value_int64(0)},
        {S("len(\"four\")"), test_value_int64(4)},
        {S("len(\"hello world\")"), test_value_int64(11)},
        {S("len(1)"), test_value_error(S("argument to `len` not supported, got INTEGER"))},
        {S("len(\"one\", \"two\")"),
         test_value_error(S("wrong number of arguments. got=2, want=1"))},
    };
    for (size_t i = 0; i < sizeof(builtin_function_tests) / sizeof(*builtin_function_tests); i++) {
        RUN_TEST(
            state,
            object,
            string_printf(
                "builtin function (\"" STRING_FMT "\")",
                STRING_ARG(builtin_function_tests[i].input)
            ),
            builtin_function_tests[i].input,
            builtin_function_tests[i].expected
        );
    }

    RUN_TEST0(state, array_literals, S("array literals"));

    struct {
        struct string input;
        struct test_value expected;
    } array_index_expression_tests[] = {
        {S("[1, 2, 3][0]"), test_value_int64(1)},
        {S("[1, 2, 3][1]"), test_value_int64(2)},
        {S("[1, 2, 3][2]"), test_value_int64(3)},
        {S("let i = 0; [1][i];"), test_value_int64(1)},
        {S("[1, 2, 3][1 + 1];"), test_value_int64(3)},
        {S("let myArray = [1, 2, 3]; myArray[2];"), test_value_int64(3)},
        {S("let myArray = [1, 2, 3]; myArray[0] + myArray[1] + myArray[2];"), test_value_int64(6)},
        {S("let myArray = [1, 2, 3]; let i = myArray[0]; myArray[i];"), test_value_int64(2)},
        {S("[1, 2, 3][3]"), test_value_null()},
        {S("[1, 2, 3][-1]"), test_value_null()},
    };
    for (size_t i = 0;
         i < sizeof(array_index_expression_tests) / sizeof(*array_index_expression_tests);
         i++) {
        RUN_TEST(
            state,
            object,
            string_printf(
                "array index expression (\"" STRING_FMT "\")",
                STRING_ARG(array_index_expression_tests[i].input)
            ),
            array_index_expression_tests[i].input,
            array_index_expression_tests[i].expected
        );
    }

    RUN_TEST0(state, hash_literals, S("hash literals"));
}
