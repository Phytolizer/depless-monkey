#include "monkey/ast.h"

#define DOWNCAST(T, node) \
    ({ \
        if ((node) == NULL) return; \
        (T*)(node); \
    })

struct ast_node ast_node_init(
    enum ast_node_type type,
    ast_node_token_literal_callback_t* token_literal_callback,
    ast_node_string_callback_t* string_callback,
    ast_node_free_callback_t* free_callback
) {
    struct ast_node result = {
        .type = type,
        .token_literal_callback = token_literal_callback,
        .string_callback = string_callback,
        .free_callback = free_callback,
    };
    return result;
}

struct string ast_node_token_literal(const struct ast_node* node) {
    return node->token_literal_callback(node);
}

struct string ast_node_string(const struct ast_node* node) {
    return node->string_callback(node);
}

void ast_node_free(struct ast_node* node) {
    return node->free_callback(node);
}

struct string ast_statement_type_string(enum ast_statement_type type) {
    switch (type) {
#define X(x) \
    case AST_STATEMENT_##x: \
        return STRING_REF(#x);
#include "monkey/private/ast_statement_types.inc"
#undef X
    }
    abort();
}

struct ast_statement ast_statement_init(
    enum ast_statement_type type,
    ast_node_token_literal_callback_t* token_literal_callback,
    ast_node_string_callback_t* string_callback,
    ast_node_free_callback_t* free_callback
) {
    struct ast_statement result = {
        .node = ast_node_init(
            AST_NODE_STATEMENT,
            token_literal_callback,
            string_callback,
            free_callback
        ),
        .type = type,
    };
    return result;
}

struct string ast_statement_token_literal(const struct ast_statement* statement) {
    return ast_node_token_literal(&statement->node);
}

struct string ast_statement_string(const struct ast_statement* statement) {
    return ast_node_string(&statement->node);
}

void ast_statement_free(struct ast_statement* statement) {
    if (statement == NULL) return;
    ast_node_free(&statement->node);
}

struct ast_expression ast_expression_init(
    enum ast_expression_type type,
    ast_node_token_literal_callback_t* token_literal_callback,
    ast_node_string_callback_t* string_callback,
    ast_node_free_callback_t* free_callback
) {
    struct ast_expression result = {
        .node = ast_node_init(
            AST_NODE_EXPRESSION,
            token_literal_callback,
            string_callback,
            free_callback
        ),
        .type = type,
    };
    return result;
}

struct string ast_expression_token_literal(const struct ast_expression* expression) {
    return ast_node_token_literal(&expression->node);
}

struct string ast_expression_string(const struct ast_expression* expression) {
    return ast_node_string(&expression->node);
}

void ast_expression_free(struct ast_expression* expression) {
    if (expression == NULL) return;
    ast_node_free(&expression->node);
}

static struct string program_token_literal(const struct ast_node* node) {
    const struct ast_program* self = (const struct ast_program*)node;
    if (self->statements.len > 0) {
        return ast_statement_token_literal(self->statements.ptr[0]);
    } else {
        return STRING_REF("");
    }
}

static struct string program_string(const struct ast_node* node) {
    const struct ast_program* self = (const struct ast_program*)node;
    struct string buf = {0};
    for (size_t i = 0; i < self->statements.len; i++) {
        struct string stmt_str = ast_statement_string(self->statements.ptr[i]);
        string_append(&buf, stmt_str);
        STRING_FREE(stmt_str);
    }
    return buf;
}

static void program_free(struct ast_node* node) {
    struct ast_program* self = DOWNCAST(struct ast_program, node);
    for (size_t i = 0; i < self->statements.len; i++) {
        ast_statement_free(self->statements.ptr[i]);
        free(self->statements.ptr[i]);
    }
    BUF_FREE(self->statements);
    free(self);
}

struct ast_program* ast_program_init(struct ast_statement_buf statements) {
    struct ast_program* self = malloc(sizeof(*self));
    self->node =
        ast_node_init(AST_NODE_PROGRAM, program_token_literal, program_string, program_free);
    self->statements = statements;
    return self;
}

static struct string let_statement_token_literal(const struct ast_node* node) {
    const struct ast_let_statement* self = (const struct ast_let_statement*)node;
    return self->token.literal;
}

static struct string let_statement_string(const struct ast_node* node) {
    const struct ast_let_statement* self = (const struct ast_let_statement*)node;
    struct string buf = string_printf(STRING_FMT " ", STRING_ARG(ast_node_token_literal(node)));
    struct string name_str = ast_node_string(&self->name->expression.node);
    string_append(&buf, name_str);
    STRING_FREE(name_str);
    string_append(&buf, STRING_REF(" = "));
    if (self->value != NULL) {
        struct string value_str = ast_expression_string(self->value);
        string_append(&buf, value_str);
        STRING_FREE(value_str);
    }
    string_append(&buf, STRING_REF(";"));
    return buf;
}

static void let_statement_free(struct ast_node* node) {
    struct ast_let_statement* self = DOWNCAST(struct ast_let_statement, node);
    STRING_FREE(self->token.literal);
    ast_node_free(&self->name->expression.node);
    free(self->name);
    if (self->value != NULL) {
        ast_expression_free(self->value);
        free(self->value);
    }
}

struct ast_let_statement* ast_let_statement_init(
    struct token token,
    struct ast_identifier* name,
    struct ast_expression* value
) {
    struct ast_let_statement* self = malloc(sizeof(*self));
    self->statement = ast_statement_init(
        AST_STATEMENT_LET,
        let_statement_token_literal,
        let_statement_string,
        let_statement_free
    );
    self->token = token;
    self->name = name;
    self->value = value;
    return self;
}

static struct string return_statement_token_literal(const struct ast_node* node) {
    const struct ast_return_statement* self = (const struct ast_return_statement*)node;
    return self->token.literal;
}

static struct string return_statement_string(const struct ast_node* node) {
    const struct ast_return_statement* self = (const struct ast_return_statement*)node;
    struct string buf = string_printf(STRING_FMT " ", STRING_ARG(ast_node_token_literal(node)));
    if (self->return_value != NULL) {
        struct string value_str = ast_expression_string(self->return_value);
        string_append(&buf, value_str);
        STRING_FREE(value_str);
    }
    string_append(&buf, STRING_REF(";"));
    return buf;
}

static void return_statement_free(struct ast_node* node) {
    struct ast_return_statement* self = DOWNCAST(struct ast_return_statement, node);
    STRING_FREE(self->token.literal);
    if (self->return_value) {
        ast_expression_free(self->return_value);
        free(self->return_value);
    }
}

struct ast_return_statement*
ast_return_statement_init(struct token token, struct ast_expression* return_value) {
    struct ast_return_statement* self = malloc(sizeof(*self));
    self->statement = ast_statement_init(
        AST_STATEMENT_RETURN,
        return_statement_token_literal,
        return_statement_string,
        return_statement_free
    );
    self->token = token;
    self->return_value = return_value;
    return self;
}

struct string ast_expression_type_string(enum ast_expression_type type) {
    switch (type) {
#define X(x) \
    case AST_EXPRESSION_##x: \
        return STRING_REF(#x);
#include "monkey/private/ast_expression_types.inc"
#undef X
    }
    abort();
}

static struct string expression_statement_token_literal(const struct ast_node* node) {
    const struct ast_expression_statement* self = (const struct ast_expression_statement*)node;
    return self->token.literal;
}

static struct string expression_statement_string(const struct ast_node* node) {
    const struct ast_expression_statement* self = (const struct ast_expression_statement*)node;
    if (self->expression != NULL) {
        return ast_expression_string(self->expression);
    } else {
        return STRING_REF("");
    }
}

static void expression_statement_free(struct ast_node* node) {
    struct ast_expression_statement* self = DOWNCAST(struct ast_expression_statement, node);
    STRING_FREE(self->token.literal);
    if (self->expression) {
        ast_expression_free(self->expression);
        free(self->expression);
    }
}

struct ast_expression_statement*
ast_expression_statement_init(struct token token, struct ast_expression* expression) {
    struct ast_expression_statement* self = malloc(sizeof(*self));
    self->statement = ast_statement_init(
        AST_STATEMENT_EXPRESSION,
        expression_statement_token_literal,
        expression_statement_string,
        expression_statement_free
    );
    self->token = token;
    self->expression = expression;
    return self;
}

static struct string identifier_token_literal(const struct ast_node* node) {
    const struct ast_identifier* self = (const struct ast_identifier*)node;
    return self->token.literal;
}

static struct string identifier_string(const struct ast_node* node) {
    const struct ast_identifier* self = (const struct ast_identifier*)node;
    return string_dup(self->value);
}

static void identifier_free(struct ast_node* node) {
    struct ast_identifier* self = DOWNCAST(struct ast_identifier, node);
    STRING_FREE(self->token.literal);
    STRING_FREE(self->value);
}

struct ast_identifier* ast_identifier_init(struct token token, struct string value) {
    struct ast_identifier* self = malloc(sizeof(*self));
    self->expression = ast_expression_init(
        AST_EXPRESSION_IDENTIFIER,
        identifier_token_literal,
        identifier_string,
        identifier_free
    );
    self->token = token;
    self->value = value;
    return self;
}

static struct string integer_literal_token_literal(const struct ast_node* node) {
    const struct ast_integer_literal* self = (const struct ast_integer_literal*)node;
    return self->token.literal;
}

static struct string integer_literal_string(const struct ast_node* node) {
    const struct ast_integer_literal* self = (const struct ast_integer_literal*)node;
    return string_dup(self->token.literal);
}

static void integer_literal_free(struct ast_node* node) {
    struct ast_integer_literal* self = DOWNCAST(struct ast_integer_literal, node);
    STRING_FREE(self->token.literal);
}

struct ast_integer_literal* ast_integer_literal_init(struct token token, int64_t value) {
    struct ast_integer_literal* self = malloc(sizeof(*self));
    self->expression = ast_expression_init(
        AST_EXPRESSION_INTEGER_LITERAL,
        integer_literal_token_literal,
        integer_literal_string,
        integer_literal_free
    );
    self->token = token;
    self->value = value;
    return self;
}

static struct string prefix_expression_token_literal(const struct ast_node* node) {
    const struct ast_prefix_expression* self = (const struct ast_prefix_expression*)node;
    return self->token.literal;
}

static struct string prefix_expression_string(const struct ast_node* node) {
    const struct ast_prefix_expression* self = (const struct ast_prefix_expression*)node;
    struct string buf = string_printf("(" STRING_FMT, STRING_ARG(self->op));
    struct string right_str = ast_expression_string(self->right);
    string_append(&buf, right_str);
    STRING_FREE(right_str);
    string_append(&buf, STRING_REF(")"));
    return buf;
}

static void prefix_expression_free(struct ast_node* node) {
    struct ast_prefix_expression* self = DOWNCAST(struct ast_prefix_expression, node);
    STRING_FREE(self->token.literal);
    STRING_FREE(self->op);
    ast_expression_free(self->right);
    free(self->right);
}

struct ast_prefix_expression*
ast_prefix_expression_init(struct token token, struct string op, struct ast_expression* right) {
    struct ast_prefix_expression* self = malloc(sizeof(*self));
    self->expression = ast_expression_init(
        AST_EXPRESSION_PREFIX,
        prefix_expression_token_literal,
        prefix_expression_string,
        prefix_expression_free
    );
    self->token = token;
    self->op = op;
    self->right = right;
    return self;
}

static struct string infix_expression_token_literal(const struct ast_node* node) {
    const struct ast_infix_expression* self = (const struct ast_infix_expression*)node;
    return self->token.literal;
}

static struct string infix_expression_string(const struct ast_node* node) {
    const struct ast_infix_expression* self = (const struct ast_infix_expression*)node;
    struct string buf = string_dup(STRING_REF("("));
    struct string left_str = ast_expression_string(self->left);
    string_append(&buf, left_str);
    STRING_FREE(left_str);
    string_append_printf(&buf, " " STRING_FMT " ", STRING_ARG(self->op));
    struct string right_str = ast_expression_string(self->right);
    string_append(&buf, right_str);
    STRING_FREE(right_str);
    string_append(&buf, STRING_REF(")"));
    return buf;
}

static void infix_expression_free(struct ast_node* node) {
    struct ast_infix_expression* self = DOWNCAST(struct ast_infix_expression, node);
    STRING_FREE(self->token.literal);
    STRING_FREE(self->op);
    ast_expression_free(self->left);
    free(self->left);
    ast_expression_free(self->right);
    free(self->right);
}

struct ast_infix_expression* ast_infix_expression_init(
    struct token token,
    struct ast_expression* left,
    struct string op,
    struct ast_expression* right
) {
    struct ast_infix_expression* self = malloc(sizeof(*self));
    self->expression = ast_expression_init(
        AST_EXPRESSION_INFIX,
        infix_expression_token_literal,
        infix_expression_string,
        infix_expression_free
    );
    self->token = token;
    self->left = left;
    self->op = op;
    self->right = right;
    return self;
}

static struct string boolean_token_literal(const struct ast_node* node) {
    const struct ast_boolean* self = (const struct ast_boolean*)node;
    return self->token.literal;
}

static struct string boolean_string(const struct ast_node* node) {
    const struct ast_boolean* self = (const struct ast_boolean*)node;
    return string_dup(self->token.literal);
}

static void boolean_free(struct ast_node* node) {
    struct ast_boolean* self = DOWNCAST(struct ast_boolean, node);
    STRING_FREE(self->token.literal);
}

struct ast_boolean* ast_boolean_init(struct token token, bool value) {
    struct ast_boolean* self = malloc(sizeof(*self));
    self->expression = ast_expression_init(
        AST_EXPRESSION_BOOLEAN,
        boolean_token_literal,
        boolean_string,
        boolean_free
    );
    self->token = token;
    self->value = value;
    return self;
}
