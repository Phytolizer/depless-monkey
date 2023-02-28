#include "monkey/test/parser.h"

#include <inttypes.h>
#include <monkey/lexer.h>
#include <monkey/parser.h>

#define S(s) STRING_REF(s)

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

static SUBTEST_FUNC(state, check_parser_errors, struct parser* p) {
    (void)state;
    if (p->errors.len == 0) {
        PASS();
    }

    struct string msg = string_printf("parser has %zu error(s)\n", p->errors.len);
    for (size_t i = 0; i < p->errors.len; i++) {
        struct string temp =
            string_printf("parser error: \"" STRING_FMT "\"\n", STRING_ARG(p->errors.ptr[i]));
        string_append(&msg, temp);
        STRING_FREE(temp);
    }
    FAIL(state, CLEANUP(STRING_FREE(msg)), STRING_FMT, STRING_ARG(msg));
}

static SUBTEST_FUNC(state, let_statement, struct ast_statement* s, struct string name) {
    TEST_ASSERT(
        state,
        STRING_EQUAL(ast_node_token_literal(&s->node), S("let")),
        NO_CLEANUP,
        "s.token_literal() not 'let'. got=\"" STRING_FMT "\"",
        STRING_ARG(ast_node_token_literal(&s->node))
    );

    TEST_ASSERT(
        state,
        s->type == AST_STATEMENT_LET,
        NO_CLEANUP,
        "s not ast_let_statement*. got=" STRING_FMT,
        STRING_ARG(ast_statement_type_string(s->type))
    );

    struct ast_let_statement* let_stmt = (struct ast_let_statement*)s;
    TEST_ASSERT(
        state,
        STRING_EQUAL(let_stmt->name->value, name),
        NO_CLEANUP,
        "let_stmt.name.value not '" STRING_FMT "'. got=" STRING_FMT,
        STRING_ARG(name),
        STRING_ARG(let_stmt->name->value)
    );
    TEST_ASSERT(
        state,
        STRING_EQUAL(ast_node_token_literal(&let_stmt->name->expression.node), name),
        NO_CLEANUP,
        "let_stmt.name.token_literal() not '" STRING_FMT "'. got=" STRING_FMT,
        STRING_ARG(name),
        STRING_ARG(ast_node_token_literal(&let_stmt->name->expression.node))
    );

    PASS();
}

static SUBTEST_FUNC(state, integer_literal, struct ast_expression* exp, int64_t value) {
    TEST_ASSERT(
        state,
        exp->type == AST_EXPRESSION_INTEGER_LITERAL,
        NO_CLEANUP,
        "exp not ast_integer_literal*. got=" STRING_FMT,
        STRING_ARG(ast_expression_type_string(exp->type))
    );
    struct ast_integer_literal* integ = (struct ast_integer_literal*)exp;
    TEST_ASSERT(
        state,
        integ->value == value,
        NO_CLEANUP,
        "integ.value not %" PRId64 ". got=%" PRId64,
        value,
        integ->value
    );

    char tempbuf[32];
    snprintf(tempbuf, sizeof(tempbuf), "%" PRId64, value);
    struct string value_str = STRING_REF_FROM_C(tempbuf);
    TEST_ASSERT(
        state,
        STRING_EQUAL(ast_node_token_literal(&integ->expression.node), value_str),
        NO_CLEANUP,
        "integ.token_literal() not '" STRING_FMT "'. got=" STRING_FMT,
        STRING_ARG(value_str),
        STRING_ARG(ast_node_token_literal(&integ->expression.node))
    );
    PASS();
}

static SUBTEST_FUNC(state, identifier, struct ast_expression* exp, struct string value) {
    TEST_ASSERT(
        state,
        exp->type == AST_EXPRESSION_IDENTIFIER,
        NO_CLEANUP,
        "exp not ast_identifier*. got=" STRING_FMT,
        STRING_ARG(ast_expression_type_string(exp->type))
    );
    struct ast_identifier* ident = (struct ast_identifier*)exp;
    TEST_ASSERT(
        state,
        STRING_EQUAL(ident->value, value),
        NO_CLEANUP,
        "integ.value not " STRING_FMT ". got=" STRING_FMT,
        value,
        ident->value
    );

    TEST_ASSERT(
        state,
        STRING_EQUAL(ast_node_token_literal(&ident->expression.node), value),
        NO_CLEANUP,
        "integ.token_literal() not '" STRING_FMT "'. got=" STRING_FMT,
        STRING_ARG(value),
        STRING_ARG(ast_node_token_literal(&ident->expression.node))
    );
    PASS();
}

static SUBTEST_FUNC(state, boolean_literal, struct ast_expression* exp, bool value) {
    TEST_ASSERT(
        state,
        exp->type == AST_EXPRESSION_BOOLEAN,
        NO_CLEANUP,
        "exp not ast_boolean*. got=" STRING_FMT,
        STRING_ARG(ast_expression_type_string(exp->type))
    );
    struct ast_boolean* boolean = (struct ast_boolean*)exp;
    TEST_ASSERT(
        state,
        boolean->value == value,
        NO_CLEANUP,
        "boolean.value not %s. got=%s",
        value ? "true" : "false",
        boolean->value ? "true" : "false"
    );

    struct string value_str = value ? S("true") : S("false");
    TEST_ASSERT(
        state,
        STRING_EQUAL(ast_node_token_literal(&boolean->expression.node), value_str),
        NO_CLEANUP,
        "boolean.token_literal() not '" STRING_FMT "'. got=" STRING_FMT,
        STRING_ARG(value_str),
        STRING_ARG(ast_node_token_literal(&boolean->expression.node))
    );
    PASS();
}

static SUBTEST_FUNC(
    state,
    literal_expression,
    struct ast_expression* exp,
    struct test_value expected
) {
    switch (expected.type) {
        case TEST_VALUE_INT64:
            RUN_SUBTEST(state, integer_literal, NO_CLEANUP, exp, expected.int64);
            break;
        case TEST_VALUE_STRING:
            RUN_SUBTEST(state, identifier, NO_CLEANUP, exp, expected.string);
            break;
        case TEST_VALUE_BOOLEAN:
            RUN_SUBTEST(state, boolean_literal, NO_CLEANUP, exp, expected.boolean);
            break;
    }
    PASS();
}

static SUBTEST_FUNC(
    state,
    infix_expression,
    struct ast_expression* exp,
    struct test_value left,
    struct string op,
    struct test_value right
) {
    TEST_ASSERT(
        state,
        exp->type == AST_EXPRESSION_INFIX,
        NO_CLEANUP,
        "exp not ast_infix_expression*. got=" STRING_FMT,
        STRING_ARG(ast_expression_type_string(exp->type))
    );

    struct ast_infix_expression* infix = (struct ast_infix_expression*)exp;
    RUN_SUBTEST(state, literal_expression, NO_CLEANUP, infix->left, left);
    TEST_ASSERT(
        state,
        STRING_EQUAL(infix->op, op),
        NO_CLEANUP,
        "infix.op is not '" STRING_FMT "'. got=" STRING_FMT,
        STRING_ARG(op),
        STRING_ARG(infix->op)
    );
    RUN_SUBTEST(state, literal_expression, NO_CLEANUP, infix->right, right);
    PASS();
}

static TEST_FUNC0(state, let_statements) {
    const struct string input =
        S("let x = 5;\n"
          "let y = 10;\n"
          "let foobar = 838383;\n");
    struct lexer l;
    lexer_init(&l, input);
    struct parser p;
    parser_init(&p, &l);
    struct ast_program* program = parse_program(&p);
    RUN_SUBTEST(
        state,
        check_parser_errors,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        &p
    );

    TEST_ASSERT(
        state,
        program->statements.len == 3,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program.statements does not contain 3 statements. got=%zu",
        program->statements.len
    );

    const struct {
        struct string expected_identifier;
    } tests[] = {
        {S("x")},
        {S("y")},
        {S("foobar")},
    };

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        struct ast_statement* stmt = program->statements.ptr[i];
        RUN_SUBTEST(
            state,
            let_statement,
            CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
            stmt,
            tests[i].expected_identifier
        );
    }

    ast_node_free(&program->node);
    parser_deinit(&p);
    PASS();
}

static TEST_FUNC0(state, return_statements) {
    const struct string input =
        S("return 5;\n"
          "return 10;\n"
          "return 993322;\n");

    struct lexer l;
    lexer_init(&l, input);
    struct parser p;
    parser_init(&p, &l);

    struct ast_program* program = parse_program(&p);
    RUN_SUBTEST(
        state,
        check_parser_errors,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        &p
    );

    TEST_ASSERT(
        state,
        program->statements.len == 3,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program.statements does not contain 3 statements. got=%zu",
        program->statements.len
    );

    for (size_t i = 0; i < program->statements.len; i++) {
        TEST_ASSERT(
            state,
            program->statements.ptr[i]->type == AST_STATEMENT_RETURN,
            CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
            "stmt not ast_return_statement*. got=" STRING_FMT,
            STRING_ARG(ast_statement_type_string(program->statements.ptr[i]->type))
        );

        struct ast_return_statement* return_stmt =
            (struct ast_return_statement*)program->statements.ptr[i];
        TEST_ASSERT(
            state,
            STRING_EQUAL(ast_node_token_literal(&return_stmt->statement.node), S("return")),
            CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
            "return_stmt.token_literal() not 'return', got \"" STRING_FMT "\"",
            STRING_ARG(ast_node_token_literal(&return_stmt->statement.node))
        );
    }

    ast_node_free(&program->node);
    parser_deinit(&p);
    PASS();
}

static TEST_FUNC0(state, identifier_expression) {
    const struct string input = S("foobar;");

    struct lexer l;
    lexer_init(&l, input);
    struct parser p;
    parser_init(&p, &l);
    struct ast_program* program = parse_program(&p);
    RUN_SUBTEST(
        state,
        check_parser_errors,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        &p
    );

    TEST_ASSERT(
        state,
        program->statements.len == 1,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program has not enough statements. got=%zu",
        program->statements.len
    );

    TEST_ASSERT(
        state,
        program->statements.ptr[0]->type == AST_STATEMENT_EXPRESSION,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program.statements[0] is not ast_expression_statement*. got=" STRING_FMT,
        STRING_ARG(ast_statement_type_string(program->statements.ptr[0]->type))
    );

    struct ast_expression_statement* stmt =
        (struct ast_expression_statement*)program->statements.ptr[0];
    RUN_SUBTEST(
        state,
        identifier,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        stmt->expression,
        S("foobar")
    );

    ast_node_free(&program->node);
    parser_deinit(&p);
    PASS();
}

static TEST_FUNC0(state, integer_literal_expression) {
    const struct string input = S("5;");

    struct lexer l;
    lexer_init(&l, input);
    struct parser p;
    parser_init(&p, &l);
    struct ast_program* program = parse_program(&p);
    RUN_SUBTEST(
        state,
        check_parser_errors,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        &p
    );

    TEST_ASSERT(
        state,
        program->statements.len == 1,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program has not enough statements. got=%zu",
        program->statements.len
    );

    TEST_ASSERT(
        state,
        program->statements.ptr[0]->type == AST_STATEMENT_EXPRESSION,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program.statements[0] is not ast_expression_statement*. got=" STRING_FMT,
        STRING_ARG(ast_statement_type_string(program->statements.ptr[0]->type))
    );

    struct ast_expression_statement* stmt =
        (struct ast_expression_statement*)program->statements.ptr[0];
    RUN_SUBTEST(
        state,
        integer_literal,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        stmt->expression,
        5
    );

    ast_node_free(&program->node);
    parser_deinit(&p);
    PASS();
}

static TEST_FUNC(state, boolean_literal_expression, struct string input, bool value) {
    struct lexer l;
    lexer_init(&l, input);
    struct parser p;
    parser_init(&p, &l);
    struct ast_program* program = parse_program(&p);
    RUN_SUBTEST(
        state,
        check_parser_errors,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        &p
    );

    TEST_ASSERT(
        state,
        program->statements.len == 1,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program has not enough statements. got=%zu",
        program->statements.len
    );

    TEST_ASSERT(
        state,
        program->statements.ptr[0]->type == AST_STATEMENT_EXPRESSION,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program.statements[0] is not ast_expression_statement*. got=" STRING_FMT,
        STRING_ARG(ast_statement_type_string(program->statements.ptr[0]->type))
    );

    struct ast_expression_statement* stmt =
        (struct ast_expression_statement*)program->statements.ptr[0];
    RUN_SUBTEST(
        state,
        literal_expression,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        stmt->expression,
        test_value_boolean(value)
    );

    ast_node_free(&program->node);
    parser_deinit(&p);
    PASS();
}

static TEST_FUNC(
    state,
    prefix_expression,
    struct string input,
    struct string op,
    struct test_value value
) {
    struct lexer l;
    lexer_init(&l, input);
    struct parser p;
    parser_init(&p, &l);
    struct ast_program* program = parse_program(&p);
    RUN_SUBTEST(
        state,
        check_parser_errors,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        &p
    );

    TEST_ASSERT(
        state,
        program->statements.len == 1,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program has not enough statements. got=%zu",
        program->statements.len
    );

    TEST_ASSERT(
        state,
        program->statements.ptr[0]->type == AST_STATEMENT_EXPRESSION,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program.statements[0] is not ast_expression_statement*. got=" STRING_FMT,
        STRING_ARG(ast_statement_type_string(program->statements.ptr[0]->type))
    );

    struct ast_expression_statement* stmt =
        (struct ast_expression_statement*)program->statements.ptr[0];
    TEST_ASSERT(
        state,
        stmt->expression->type == AST_EXPRESSION_PREFIX,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "stmt.expression is not ast_prefix_expression*. got=" STRING_FMT,
        STRING_ARG(ast_expression_type_string(stmt->expression->type))
    );

    struct ast_prefix_expression* exp = (struct ast_prefix_expression*)stmt->expression;
    TEST_ASSERT(
        state,
        STRING_EQUAL(exp->op, op),
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "exp.op is not '" STRING_FMT "'. got=" STRING_FMT,
        STRING_ARG(op),
        STRING_ARG(exp->op)
    );
    RUN_SUBTEST(
        state,
        literal_expression,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        exp->right,
        value
    );

    ast_node_free(&program->node);
    parser_deinit(&p);
    PASS();
}

static TEST_FUNC(
    state,
    infix_expression,
    struct string input,
    struct test_value left_value,
    struct string op,
    struct test_value right_value
) {
    struct lexer l;
    lexer_init(&l, input);
    struct parser p;
    parser_init(&p, &l);
    struct ast_program* program = parse_program(&p);
    RUN_SUBTEST(
        state,
        check_parser_errors,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        &p
    );

    TEST_ASSERT(
        state,
        program->statements.len == 1,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program has not enough statements. got=%zu",
        program->statements.len
    );

    TEST_ASSERT(
        state,
        program->statements.ptr[0]->type == AST_STATEMENT_EXPRESSION,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program.statements[0] is not ast_expression_statement*. got=" STRING_FMT,
        STRING_ARG(ast_statement_type_string(program->statements.ptr[0]->type))
    );

    struct ast_expression_statement* stmt =
        (struct ast_expression_statement*)program->statements.ptr[0];
    RUN_SUBTEST(
        state,
        infix_expression,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        stmt->expression,
        left_value,
        op,
        right_value
    );

    ast_node_free(&program->node);
    parser_deinit(&p);
    PASS();
}

static TEST_FUNC(state, operator_precedence, struct string input, struct string expected) {
    struct lexer l;
    lexer_init(&l, input);
    struct parser p;
    parser_init(&p, &l);
    struct ast_program* program = parse_program(&p);
    RUN_SUBTEST(
        state,
        check_parser_errors,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        &p
    );

    struct string actual = ast_node_string(&program->node);
    TEST_ASSERT(
        state,
        STRING_EQUAL(actual, expected),
        CLEANUP(STRING_FREE(actual); ast_node_free(&program->node); parser_deinit(&p)),
        "expected=\"" STRING_FMT "\", got=\"" STRING_FMT "\"",
        STRING_ARG(expected),
        STRING_ARG(actual)
    );

    STRING_FREE(actual);
    ast_node_free(&program->node);
    parser_deinit(&p);
    PASS();
}

static TEST_FUNC0(state, if_expression) {
    const struct string input = S("if (x < y) { x }");
    struct lexer l;
    lexer_init(&l, input);
    struct parser p;
    parser_init(&p, &l);
    struct ast_program* program = parse_program(&p);
    RUN_SUBTEST(
        state,
        check_parser_errors,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        &p
    );

    TEST_ASSERT(
        state,
        program->statements.len == 1,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program has not enough statements. got=%zu",
        program->statements.len
    );

    TEST_ASSERT(
        state,
        program->statements.ptr[0]->type == AST_STATEMENT_EXPRESSION,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program.statements[0] is not ast_expression_statement*. got=" STRING_FMT,
        STRING_ARG(ast_statement_type_string(program->statements.ptr[0]->type))
    );

    struct ast_expression_statement* stmt =
        (struct ast_expression_statement*)program->statements.ptr[0];
    TEST_ASSERT(
        state,
        stmt->expression->type == AST_EXPRESSION_IF,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "stmt.expression is not ast_if_expression*. got=" STRING_FMT,
        STRING_ARG(ast_expression_type_string(stmt->expression->type))
    );

    struct ast_if_expression* exp = (struct ast_if_expression*)stmt->expression;
    RUN_SUBTEST(
        state,
        infix_expression,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        exp->condition,
        test_value_string(S("x")),
        S("<"),
        test_value_string(S("y"))
    );

    TEST_ASSERT(
        state,
        exp->consequence->statements.len == 1,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "consequence is not 1 statements. got=%zu",
        exp->consequence->statements.len
    );

    TEST_ASSERT(
        state,
        exp->consequence->statements.ptr[0]->type == AST_STATEMENT_EXPRESSION,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "consequence.statements[0] is not ast_expression_statement*. got=" STRING_FMT,
        STRING_ARG(ast_statement_type_string(exp->consequence->statements.ptr[0]->type))
    );

    struct ast_expression_statement* consequence =
        (struct ast_expression_statement*)exp->consequence->statements.ptr[0];
    RUN_SUBTEST(
        state,
        identifier,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        consequence->expression,
        S("x")
    );

    TEST_ASSERT(
        state,
        exp->alternative == NULL,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "exp.alternative was not NULL. got=" STRING_FMT,
        STRING_ARG(ast_statement_string(&exp->alternative->statement))
    );

    ast_node_free(&program->node);
    parser_deinit(&p);
    PASS();
}

static TEST_FUNC0(state, if_else_expression) {
    const struct string input = S("if (x < y) { x } else { y }");
    struct lexer l;
    lexer_init(&l, input);
    struct parser p;
    parser_init(&p, &l);
    struct ast_program* program = parse_program(&p);
    RUN_SUBTEST(
        state,
        check_parser_errors,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        &p
    );

    TEST_ASSERT(
        state,
        program->statements.len == 1,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program has not enough statements. got=%zu",
        program->statements.len
    );

    TEST_ASSERT(
        state,
        program->statements.ptr[0]->type == AST_STATEMENT_EXPRESSION,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program.statements[0] is not ast_expression_statement*. got=" STRING_FMT,
        STRING_ARG(ast_statement_type_string(program->statements.ptr[0]->type))
    );

    struct ast_expression_statement* stmt =
        (struct ast_expression_statement*)program->statements.ptr[0];
    TEST_ASSERT(
        state,
        stmt->expression->type == AST_EXPRESSION_IF,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "stmt.expression is not ast_if_expression*. got=" STRING_FMT,
        STRING_ARG(ast_expression_type_string(stmt->expression->type))
    );

    struct ast_if_expression* exp = (struct ast_if_expression*)stmt->expression;
    RUN_SUBTEST(
        state,
        infix_expression,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        exp->condition,
        test_value_string(S("x")),
        S("<"),
        test_value_string(S("y"))
    );

    TEST_ASSERT(
        state,
        exp->consequence->statements.len == 1,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "consequence is not 1 statements. got=%zu",
        exp->consequence->statements.len
    );

    TEST_ASSERT(
        state,
        exp->consequence->statements.ptr[0]->type == AST_STATEMENT_EXPRESSION,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "consequence.statements[0] is not ast_expression_statement*. got=" STRING_FMT,
        STRING_ARG(ast_statement_type_string(exp->consequence->statements.ptr[0]->type))
    );

    struct ast_expression_statement* consequence =
        (struct ast_expression_statement*)exp->consequence->statements.ptr[0];
    RUN_SUBTEST(
        state,
        identifier,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        consequence->expression,
        S("x")
    );

    TEST_ASSERT(
        state,
        exp->alternative->statements.len == 1,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "alternative is not 1 statements. got=%zu",
        exp->alternative->statements.len
    );

    TEST_ASSERT(
        state,
        exp->alternative->statements.ptr[0]->type == AST_STATEMENT_EXPRESSION,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "alternative.statements[0] is not ast_expression_statement*. got=" STRING_FMT,
        STRING_ARG(ast_statement_type_string(exp->alternative->statements.ptr[0]->type))
    );

    struct ast_expression_statement* alternative =
        (struct ast_expression_statement*)exp->alternative->statements.ptr[0];

    RUN_SUBTEST(
        state,
        identifier,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        alternative->expression,
        S("y")
    );

    ast_node_free(&program->node);
    parser_deinit(&p);
    PASS();
}

static TEST_FUNC0(state, function_literal) {
    const struct string input = S("fn(x, y) { x + y; }");

    struct lexer l;
    lexer_init(&l, input);
    struct parser p;
    parser_init(&p, &l);
    struct ast_program* program = parse_program(&p);
    RUN_SUBTEST(
        state,
        check_parser_errors,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        &p
    );

    TEST_ASSERT(
        state,
        program->statements.len == 1,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program has not enough statements. got=%zu",
        program->statements.len
    );

    TEST_ASSERT(
        state,
        program->statements.ptr[0]->type == AST_STATEMENT_EXPRESSION,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program.statements[0] is not ast_expression_statement*. got=" STRING_FMT,
        STRING_ARG(ast_statement_type_string(program->statements.ptr[0]->type))
    );

    struct ast_expression_statement* stmt =
        (struct ast_expression_statement*)program->statements.ptr[0];

    TEST_ASSERT(
        state,
        stmt->expression->type == AST_EXPRESSION_FUNCTION,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "stmt.expression is not ast_function_literal*. got=" STRING_FMT,
        STRING_ARG(ast_expression_type_string(stmt->expression->type))
    );

    struct ast_function_literal* function = (struct ast_function_literal*)stmt->expression;
    TEST_ASSERT(
        state,
        function->parameters.len == 2,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "function literal parameters wrong. want 2, got=%zu",
        function->parameters.len
    );

    RUN_SUBTEST(
        state,
        identifier,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        &function->parameters.ptr[0]->expression,
        S("x")
    );

    RUN_SUBTEST(
        state,
        identifier,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        &function->parameters.ptr[1]->expression,
        S("y")
    );

    TEST_ASSERT(
        state,
        function->body->statements.len == 1,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "function body statements has not 1 statements. got=%zu",
        function->body->statements.len
    );

    struct ast_expression_statement* body_stmt =
        (struct ast_expression_statement*)function->body->statements.ptr[0];

    RUN_SUBTEST(
        state,
        infix_expression,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        body_stmt->expression,
        test_value_string(S("x")),
        S("+"),
        test_value_string(S("y"))
    );

    ast_node_free(&program->node);
    parser_deinit(&p);
    PASS();
}

BUF_T(struct string, string);

static TEST_FUNC(
    state,
    function_parameters,
    struct string input,
    struct string_buf expected_params
) {
    struct lexer l;
    lexer_init(&l, input);
    struct parser p;
    parser_init(&p, &l);
    struct ast_program* program = parse_program(&p);
    RUN_SUBTEST(
        state,
        check_parser_errors,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        &p
    );

    TEST_ASSERT(
        state,
        program->statements.len == 1,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program has not enough statements. got=%zu",
        program->statements.len
    );

    TEST_ASSERT(
        state,
        program->statements.ptr[0]->type == AST_STATEMENT_EXPRESSION,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program.statements[0] is not ast_expression_statement*. got=" STRING_FMT,
        STRING_ARG(ast_statement_type_string(program->statements.ptr[0]->type))
    );

    struct ast_expression_statement* stmt =
        (struct ast_expression_statement*)program->statements.ptr[0];

    TEST_ASSERT(
        state,
        stmt->expression->type == AST_EXPRESSION_FUNCTION,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "stmt.expression is not ast_function_literal*. got=" STRING_FMT,
        STRING_ARG(ast_expression_type_string(stmt->expression->type))
    );

    struct ast_function_literal* function = (struct ast_function_literal*)stmt->expression;
    TEST_ASSERT(
        state,
        function->parameters.len == expected_params.len,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "function literal parameters wrong. want %zu, got=%zu",
        expected_params.len,
        function->parameters.len
    );

    for (size_t i = 0; i < expected_params.len; i++) {
        RUN_SUBTEST(
            state,
            identifier,
            CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
            &function->parameters.ptr[i]->expression,
            expected_params.ptr[i]
        );
    }

    ast_node_free(&program->node);
    parser_deinit(&p);
    PASS();
}

static TEST_FUNC0(state, call_expression) {
    const struct string input = S("add(1, 2 * 3, 4 + 5);");

    struct lexer l;
    lexer_init(&l, input);
    struct parser p;
    parser_init(&p, &l);
    struct ast_program* program = parse_program(&p);
    RUN_SUBTEST(
        state,
        check_parser_errors,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        &p
    );

    TEST_ASSERT(
        state,
        program->statements.len == 1,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program has not enough statements. got=%zu",
        program->statements.len
    );

    TEST_ASSERT(
        state,
        program->statements.ptr[0]->type == AST_STATEMENT_EXPRESSION,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "program.statements[0] is not ast_expression_statement*. got=" STRING_FMT,
        STRING_ARG(ast_statement_type_string(program->statements.ptr[0]->type))
    );

    struct ast_expression_statement* stmt =
        (struct ast_expression_statement*)program->statements.ptr[0];

    TEST_ASSERT(
        state,
        stmt->expression->type == AST_EXPRESSION_CALL,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "stmt.expression is not ast_call_expression*. got=" STRING_FMT,
        STRING_ARG(ast_expression_type_string(stmt->expression->type))
    );

    struct ast_call_expression* exp = (struct ast_call_expression*)stmt->expression;

    RUN_SUBTEST(
        state,
        identifier,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        exp->function,
        S("add")
    );

    TEST_ASSERT(
        state,
        exp->arguments.len == 3,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        "wrong length of arguments. got=%zu",
        exp->arguments.len
    );

    RUN_SUBTEST(
        state,
        integer_literal,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        exp->arguments.ptr[0],
        1
    );
    RUN_SUBTEST(
        state,
        infix_expression,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        exp->arguments.ptr[1],
        test_value_int64(2),
        S("*"),
        test_value_int64(3)
    );
    RUN_SUBTEST(
        state,
        infix_expression,
        CLEANUP(ast_node_free(&program->node); parser_deinit(&p)),
        exp->arguments.ptr[2],
        test_value_int64(4),
        S("+"),
        test_value_int64(5)
    );

    ast_node_free(&program->node);
    parser_deinit(&p);
    PASS();
}

SUITE_FUNC(state, parser) {
    RUN_TEST0(state, let_statements, S("let statements"));
    RUN_TEST0(state, return_statements, S("return statements"));
    RUN_TEST0(state, identifier_expression, S("identifier expression"));
    RUN_TEST0(state, integer_literal_expression, S("integer literal expression"));
    struct {
        struct string input;
        struct string op;
        struct test_value value;
    } prefix_tests[] = {
        {S("!5;"), S("!"), test_value_int64(5)},
        {S("-15;"), S("-"), test_value_int64(15)},
    };
    for (size_t i = 0; i < sizeof(prefix_tests) / sizeof(*prefix_tests); i++) {
        RUN_TEST(
            state,
            prefix_expression,
            string_printf(
                "prefix expression (\"" STRING_FMT "\")",
                STRING_ARG(prefix_tests[i].input)
            ),
            prefix_tests[i].input,
            prefix_tests[i].op,
            prefix_tests[i].value
        );
    }

    struct {
        struct string input;
        struct test_value left_value;
        struct string op;
        struct test_value right_value;
    } infix_tests[] = {
        {S("5 + 5;"), test_value_int64(5), S("+"), test_value_int64(5)},
        {S("5 - 5;"), test_value_int64(5), S("-"), test_value_int64(5)},
        {S("5 * 5;"), test_value_int64(5), S("*"), test_value_int64(5)},
        {S("5 / 5;"), test_value_int64(5), S("/"), test_value_int64(5)},
        {S("5 > 5;"), test_value_int64(5), S(">"), test_value_int64(5)},
        {S("5 < 5;"), test_value_int64(5), S("<"), test_value_int64(5)},
        {S("5 == 5;"), test_value_int64(5), S("=="), test_value_int64(5)},
        {S("5 != 5;"), test_value_int64(5), S("!="), test_value_int64(5)},
        {S("true == true"), test_value_boolean(true), S("=="), test_value_boolean(true)},
        {S("true != false"), test_value_boolean(true), S("!="), test_value_boolean(false)},
        {S("false == false"), test_value_boolean(false), S("=="), test_value_boolean(false)},
    };
    for (size_t i = 0; i < sizeof(infix_tests) / sizeof(*infix_tests); i++) {
        RUN_TEST(
            state,
            infix_expression,
            string_printf(
                "infix expression (\"" STRING_FMT "\")",
                STRING_ARG(infix_tests[i].input)
            ),
            infix_tests[i].input,
            infix_tests[i].left_value,
            infix_tests[i].op,
            infix_tests[i].right_value
        );
    }

    struct {
        struct string input;
        struct string expected;
    } operator_precedence_tests[] = {
        {S("-a * b;"), S("((-a) * b)")},
        {S("!-a;"), S("(!(-a))")},
        {S("a + b + c;"), S("((a + b) + c)")},
        {S("a + b - c;"), S("((a + b) - c)")},
        {S("a * b * c;"), S("((a * b) * c)")},
        {S("a * b / c;"), S("((a * b) / c)")},
        {S("a + b / c;"), S("(a + (b / c))")},
        {S("a + b * c + d / e - f;"), S("(((a + (b * c)) + (d / e)) - f)")},
        {S("3 + 4; -5 * 5;"), S("(3 + 4)((-5) * 5)")},
        {S("5 > 4 == 3 < 4;"), S("((5 > 4) == (3 < 4))")},
        {S("5 < 4 != 3 > 4;"), S("((5 < 4) != (3 > 4))")},
        {S("3 + 4 * 5 == 3 * 1 + 4 * 5;"), S("((3 + (4 * 5)) == ((3 * 1) + (4 * 5)))")},
        {S("true"), S("true")},
        {S("false"), S("false")},
        {S("3 > 5 == false"), S("((3 > 5) == false)")},
        {S("3 < 5 == true"), S("((3 < 5) == true)")},
        {S("1 + (2 + 3) + 4"), S("((1 + (2 + 3)) + 4)")},
        {S("(5 + 5) * 2"), S("((5 + 5) * 2)")},
        {S("2 / (5 + 5)"), S("(2 / (5 + 5))")},
        {S("-(5 + 5)"), S("(-(5 + 5))")},
        {S("!(true == true)"), S("(!(true == true))")},
        {S("a + add(b * c) + d"), S("((a + add((b * c))) + d)")},
        {S("add(a, b, 1, 2 * 3, 4 + 5, add(6, 7 * 8))"),
         S("add(a, b, 1, (2 * 3), (4 + 5), add(6, (7 * 8)))")},
        {S("add(a + b + c * d / f + g)"), S("add((((a + b) + ((c * d) / f)) + g))")},
    };
    for (size_t i = 0; i < sizeof(operator_precedence_tests) / sizeof(*operator_precedence_tests);
         i++) {
        RUN_TEST(
            state,
            operator_precedence,
            string_printf(
                "operator precedence (\"" STRING_FMT "\")",
                STRING_ARG(operator_precedence_tests[i].input)
            ),
            operator_precedence_tests[i].input,
            operator_precedence_tests[i].expected
        );
    }

    struct {
        struct string input;
        bool expected;
    } boolean_tests[] = {
        {S("true;"), true},
        {S("false;"), false},
    };
    for (size_t i = 0; i < sizeof(boolean_tests) / sizeof(*boolean_tests); i++) {
        RUN_TEST(
            state,
            boolean_literal_expression,
            string_printf(
                "boolean literal expression (\"" STRING_FMT "\")",
                STRING_ARG(boolean_tests[i].input)
            ),
            boolean_tests[i].input,
            boolean_tests[i].expected
        );
    }

    RUN_TEST0(state, if_expression, S("if expression"));
    RUN_TEST0(state, if_else_expression, S("if/else expression"));
    RUN_TEST0(state, function_literal, S("function literal"));

    struct {
        struct string input;
        struct string_buf expected_params;
    } function_parameters_tests[] = {
        {S("fn() {}"), BUF_LIT(struct string_buf)},
        {S("fn(x) {}"), BUF_LIT(struct string_buf, S("x"))},
        {S("fn(x, y, z) {}"), BUF_LIT(struct string_buf, S("x"), S("y"), S("z"))},
    };
    for (size_t i = 0; i < sizeof(function_parameters_tests) / sizeof(*function_parameters_tests);
         i++) {
        RUN_TEST(
            state,
            function_parameters,
            string_printf(
                "function parameters (\"" STRING_FMT "\")",
                STRING_ARG(function_parameters_tests[i].input)
            ),
            function_parameters_tests[i].input,
            function_parameters_tests[i].expected_params
        );
        BUF_FREE(function_parameters_tests[i].expected_params);
    }

    RUN_TEST0(state, call_expression, S("call expression"));
}
