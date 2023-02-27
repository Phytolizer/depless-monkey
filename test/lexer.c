#include "monkey/test/lexer.h"

#include <monkey/lexer.h>
#include <monkey/token.h>

#include "monkey/test/framework.h"

static TEST_FUNC0(state, next_token) {
    const struct string input = STRING_REF_C(
        "let five = 5;\n"
        "let ten = 10;\n"
        "\n"
        "let add = fn(x, y) {\n"
        "  x + y;\n"
        "};\n"
        "\n"
        "let result = add(five, ten);\n"
        "!-/*5;\n"
        "5 < 10 > 5;\n"
        "\n"
        "if (5 < 10) {\n"
        "  return true;\n"
        "} else {\n"
        "  return false;\n"
        "}\n"
        "\n"
        "10 == 10;\n"
        "10 != 9;\n"
    );

#define S(x) STRING_REF_C(x)
    // clang-format off
    struct {
        enum token_type expected_type;
        struct string expected_literal;
    } tests[] = {
        {TOKEN_LET, S("let")},
        {TOKEN_IDENT, S("five")},
        {TOKEN_ASSIGN, S("=")},
        {TOKEN_INT, S("5")},
        {TOKEN_SEMICOLON, S(";")},
        {TOKEN_LET, S("let")},
        {TOKEN_IDENT, S("ten")},
        {TOKEN_ASSIGN, S("=")},
        {TOKEN_INT, S("10")},
        {TOKEN_SEMICOLON, S(";")},
        {TOKEN_LET, S("let")},
        {TOKEN_IDENT, S("add")},
        {TOKEN_ASSIGN, S("=")},
        {TOKEN_FUNCTION, S("fn")},
        {TOKEN_LPAREN, S("(")},
        {TOKEN_IDENT, S("x")},
        {TOKEN_COMMA, S(",")},
        {TOKEN_IDENT, S("y")},
        {TOKEN_RPAREN, S(")")},
        {TOKEN_LBRACE, S("{")},
        {TOKEN_IDENT, S("x")},
        {TOKEN_PLUS, S("+")},
        {TOKEN_IDENT, S("y")},
        {TOKEN_SEMICOLON, S(";")},
        {TOKEN_RBRACE, S("}")},
        {TOKEN_SEMICOLON, S(";")},
        {TOKEN_LET, S("let")},
        {TOKEN_IDENT, S("result")},
        {TOKEN_ASSIGN, S("=")},
        {TOKEN_IDENT, S("add")},
        {TOKEN_LPAREN, S("(")},
        {TOKEN_IDENT, S("five")},
        {TOKEN_COMMA, S(",")},
        {TOKEN_IDENT, S("ten")},
        {TOKEN_RPAREN, S(")")},
        {TOKEN_SEMICOLON, S(";")},
        {TOKEN_BANG, S("!")},
        {TOKEN_MINUS, S("-")},
        {TOKEN_SLASH, S("/")},
        {TOKEN_ASTERISK, S("*")},
        {TOKEN_INT, S("5")},
        {TOKEN_SEMICOLON, S(";")},
        {TOKEN_INT, S("5")},
        {TOKEN_LT, S("<")},
        {TOKEN_INT, S("10")},
        {TOKEN_GT, S(">")},
        {TOKEN_INT, S("5")},
        {TOKEN_SEMICOLON, S(";")},
        {TOKEN_IF, S("if")},
        {TOKEN_LPAREN, S("(")},
        {TOKEN_INT, S("5")},
        {TOKEN_LT, S("<")},
        {TOKEN_INT, S("10")},
        {TOKEN_RPAREN, S(")")},
        {TOKEN_LBRACE, S("{")},
        {TOKEN_RETURN, S("return")},
        {TOKEN_TRUE, S("true")},
        {TOKEN_SEMICOLON, S(";")},
        {TOKEN_RBRACE, S("}")},
        {TOKEN_ELSE, S("else")},
        {TOKEN_LBRACE, S("{")},
        {TOKEN_RETURN, S("return")},
        {TOKEN_FALSE, S("false")},
        {TOKEN_SEMICOLON, S(";")},
        {TOKEN_RBRACE, S("}")},
        {TOKEN_INT, S("10")},
        {TOKEN_EQ, S("==")},
        {TOKEN_INT, S("10")},
        {TOKEN_SEMICOLON, S(";")},
        {TOKEN_INT, S("10")},
        {TOKEN_NOT_EQ, S("!=")},
        {TOKEN_INT, S("9")},
        {TOKEN_SEMICOLON, S(";")},
        {TOKEN_EOF, S("")},
    };
    // clang-format on
#undef S

    struct lexer lexer;
    lexer_init(&lexer, input);

    for (size_t i = 0; i < sizeof(tests) / sizeof(*tests); i++) {
        struct token tok = lexer_next_token(&lexer);
        TEST_ASSERT(
            state,
            tok.type == tests[i].expected_type,
            CLEANUP(STRING_FREE(tok.literal)),
            "tests[%d] - tokentype wrong. expected=\"" STRING_FMT "\", got=\"" STRING_FMT "\"",
            i,
            STRING_ARG(token_type_string(tests[i].expected_type)),
            STRING_ARG(token_type_string(tok.type))
        );
        TEST_ASSERT(
            state,
            STRING_EQUAL(tok.literal, tests[i].expected_literal),
            CLEANUP(STRING_FREE(tok.literal)),
            "tests[%d] - literal wrong. expected=\"" STRING_FMT "\", got=\"" STRING_FMT "\"",
            i,
            STRING_ARG(tests[i].expected_literal),
            STRING_ARG(tok.literal)
        );
        STRING_FREE(tok.literal);
    }
    PASS();
}

SUITE_FUNC(state, lexer) {
    RUN_TEST0(state, next_token, STRING_REF("next token"));
}
