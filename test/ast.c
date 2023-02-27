#include "monkey/test/ast.h"

#include "monkey/ast.h"
#include "monkey/buf.h"

static TEST_FUNC0(state, string) {
    struct ast_program* program = ast_program_init(BUF_LIT(
        struct ast_statement_buf,
        ast_let_statement_init_base(
            (struct token){TOKEN_LET, STRING_REF_C("let")},
            ast_identifier_init(
                (struct token){TOKEN_IDENT, STRING_REF_C("myVar")},
                STRING_REF("myVar")
            ),
            ast_identifier_init_base(
                (struct token){TOKEN_IDENT, STRING_REF_C("anotherVar")},
                STRING_REF("anotherVar")
            )
        )
    ));

    struct string actual = ast_node_string(&program->node);
    TEST_ASSERT(
        state,
        STRING_EQUAL(actual, STRING_REF("let myVar = anotherVar;")),
        CLEANUP(STRING_FREE(actual); ast_node_free(&program->node)),
        "program.string() wrong. got=\"" STRING_FMT "\"",
        STRING_ARG(actual)
    );
    STRING_FREE(actual);
    ast_node_free(&program->node);
    PASS();
}

SUITE_FUNC(state, ast) {
    RUN_TEST0(state, string, STRING_REF("string()"));
}
