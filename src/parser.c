#include "monkey/parser.h"

#include <iso646.h>

#include "monkey/parseint.h"

typedef struct ast_expression* prefix_parse_fn_t(struct parser* parser);
typedef struct ast_expression* infix_parse_fn_t(struct parser* parser, struct ast_expression* left);

enum precedence {
    PREC_LOWEST,
    PREC_EQUALS,
    PREC_LESSGREATER,
    PREC_SUM,
    PREC_PRODUCT,
    PREC_PREFIX,
    PREC_CALL,
};

static enum precedence token_precedence(enum token_type type) {
    switch (type) {
        case TOKEN_EQ:
        case TOKEN_NOT_EQ:
            return PREC_EQUALS;
        case TOKEN_LT:
        case TOKEN_GT:
            return PREC_LESSGREATER;
        case TOKEN_PLUS:
        case TOKEN_MINUS:
            return PREC_SUM;
        case TOKEN_SLASH:
        case TOKEN_ASTERISK:
            return PREC_PRODUCT;
        default:
            return PREC_LOWEST;
    }
}

static inline enum precedence peek_precedence(struct parser* p) {
    return token_precedence(p->peek_token.type);
}

static inline enum precedence cur_precedence(struct parser* p) {
    return token_precedence(p->cur_token.type);
}

static void next_token(struct parser* p) {
    STRING_FREE(p->cur_token.literal);
    p->cur_token = p->peek_token;
    p->peek_token = lexer_next_token(p->l);
}

static struct token take_cur(struct parser* p) {
    struct token result = p->cur_token;
    p->cur_token.literal = STRING_REF("");
    return result;
}

static void peek_error(struct parser* p, enum token_type type) {
    struct string msg = string_printf(
        "expected next token to be " STRING_FMT ", got " STRING_FMT " instead",
        STRING_ARG(token_type_string(type)),
        STRING_ARG(token_type_string(p->peek_token.type))
    );
    BUF_PUSH(&p->errors, msg);
}

static void no_prefix_parse_fn_error(struct parser* p, enum token_type type) {
    struct string msg = string_printf(
        "no prefix parse function for " STRING_FMT " found",
        STRING_ARG(token_type_string(type))
    );
    BUF_PUSH(&p->errors, msg);
}

static bool expect_peek(struct parser* p, enum token_type type) {
    if (p->peek_token.type == type) {
        next_token(p);
        return true;
    } else {
        peek_error(p, type);
        return false;
    }
}

void parser_init(struct parser* p, struct lexer* l) {
    *p = (struct parser){.l = l};
    next_token(p);
    next_token(p);
}

void parser_deinit(struct parser* p) {
    for (size_t i = 0; i < p->errors.len; i++) {
        STRING_FREE(p->errors.ptr[i]);
    }
    BUF_FREE(p->errors);
    STRING_FREE(p->cur_token.literal);
    STRING_FREE(p->peek_token.literal);
}

static struct ast_expression* parse_expression(struct parser* p, enum precedence precedence);

static struct ast_expression* parse_identifier(struct parser* p) {
    struct token token = take_cur(p);
    return ast_identifier_init_base(token, string_dup(token.literal));
}

static struct ast_expression* parse_integer_literal(struct parser* p) {
    struct token token = take_cur(p);

    struct parse_i64_result result = parse_i64(token.literal);
    if (!result.ok) {
        struct string msg = string_printf(
            "could not parse \"" STRING_FMT "\" as integer",
            STRING_ARG(token.literal)
        );
        BUF_PUSH(&p->errors, msg);
        STRING_FREE(token.literal);
        return NULL;
    }

    return ast_integer_literal_init_base(token, result.value);
}

static struct ast_expression* parse_boolean(struct parser* p) {
    struct token token = take_cur(p);
    return ast_boolean_init_base(token, token.type == TOKEN_TRUE);
}

static struct ast_expression* parse_prefix_expression(struct parser* p) {
    struct token token = take_cur(p);
    struct string op = string_dup(token.literal);

    next_token(p);

    struct ast_expression* right = parse_expression(p, PREC_PREFIX);

    return ast_prefix_expression_init_base(token, op, right);
}

static struct ast_expression*
parse_infix_expression(struct parser* p, struct ast_expression* left) {
    struct token token = take_cur(p);
    struct string op = string_dup(token.literal);

    enum precedence precedence = cur_precedence(p);
    next_token(p);
    struct ast_expression* right = parse_expression(p, precedence);

    return ast_infix_expression_init_base(token, left, op, right);
}

static struct ast_expression* parse_grouped_expression(struct parser* p) {
    next_token(p);
    struct ast_expression* exp = parse_expression(p, PREC_LOWEST);
    if (!expect_peek(p, TOKEN_RPAREN)) {
        ast_expression_free(exp);
        return NULL;
    }
    return exp;
}

static prefix_parse_fn_t* prefix_parse_fn(enum token_type type) {
    switch (type) {
        case TOKEN_IDENT:
            return &parse_identifier;
        case TOKEN_INT:
            return &parse_integer_literal;
        case TOKEN_BANG:
        case TOKEN_MINUS:
            return &parse_prefix_expression;
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            return &parse_boolean;
        case TOKEN_LPAREN:
            return &parse_grouped_expression;
        default:
            return NULL;
    }
}

static infix_parse_fn_t* infix_parse_fn(enum token_type type) {
    switch (type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_SLASH:
        case TOKEN_ASTERISK:
        case TOKEN_EQ:
        case TOKEN_NOT_EQ:
        case TOKEN_LT:
        case TOKEN_GT:
            return &parse_infix_expression;
        default:
            return NULL;
    }
}

static struct ast_expression* parse_expression(struct parser* p, enum precedence precedence) {
    prefix_parse_fn_t* prefix = prefix_parse_fn(p->cur_token.type);
    if (prefix == NULL) {
        no_prefix_parse_fn_error(p, p->cur_token.type);
        return NULL;
    }
    struct ast_expression* left_exp = prefix(p);

    while (p->peek_token.type != TOKEN_SEMICOLON and precedence < peek_precedence(p)) {
        infix_parse_fn_t* infix = infix_parse_fn(p->peek_token.type);
        if (infix == NULL) {
            return left_exp;
        }

        next_token(p);
        left_exp = infix(p, left_exp);
    }

    return left_exp;
}

static struct ast_statement* parse_let_statement(struct parser* p) {
    struct token token = take_cur(p);

    if (!expect_peek(p, TOKEN_IDENT)) {
        STRING_FREE(token.literal);
        return NULL;
    }

    struct token name_tok = take_cur(p);
    struct ast_identifier* name = ast_identifier_init(name_tok, string_dup(name_tok.literal));

    if (!expect_peek(p, TOKEN_ASSIGN)) {
        STRING_FREE(token.literal);
        ast_node_free(&name->expression.node);
        free(name);
        return NULL;
    }

    // [TODO] skipping until semicolon
    while (p->cur_token.type != TOKEN_SEMICOLON) {
        next_token(p);
    }

    return ast_let_statement_init_base(token, name, NULL);
}

static struct ast_statement* parse_return_statement(struct parser* p) {
    struct token token = take_cur(p);

    next_token(p);

    // [TODO] skipping until semicolon
    while (p->cur_token.type != TOKEN_SEMICOLON) {
        next_token(p);
    }

    return ast_return_statement_init_base(token, NULL);
}

static struct ast_statement* parse_expression_statement(struct parser* p) {
    struct token token = token_dup(p->cur_token);

    struct ast_expression* expression = parse_expression(p, PREC_LOWEST);

    if (p->peek_token.type == TOKEN_SEMICOLON) {
        next_token(p);
    }

    return ast_expression_statement_init_base(token, expression);
}

static struct ast_statement* parse_statement(struct parser* p) {
    switch (p->cur_token.type) {
        case TOKEN_LET:
            return parse_let_statement(p);
        case TOKEN_RETURN:
            return parse_return_statement(p);
        default:
            return parse_expression_statement(p);
    }
}

struct ast_program* parse_program(struct parser* p) {
    struct ast_statement_buf statements = {0};

    while (p->cur_token.type != TOKEN_EOF) {
        struct ast_statement* stmt = parse_statement(p);
        if (stmt != NULL) {
            BUF_PUSH(&statements, stmt);
        }
        next_token(p);
    }

    return ast_program_init(statements);
}
