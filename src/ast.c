#include "monkey/ast.h"

#include "monkey/private/stdc.h"

struct ast_node ast_node_init(
    enum ast_node_type type,
    ast_node_token_literal_callback_t* token_literal_callback,
    ast_node_string_callback_t* string_callback,
    ast_node_decref_callback_t* decref_callback
) {
    struct ast_node result = {
        .type = type,
        .token_literal_callback = token_literal_callback,
        .string_callback = string_callback,
        .decref_callback = decref_callback,
        .rc = 1,
    };
    return result;
}

struct string ast_node_token_literal(const struct ast_node* node) {
    return node->token_literal_callback(node);
}

struct string ast_node_string(const struct ast_node* node) {
    return node->string_callback(node);
}

size_t ast_node_decref(struct ast_node* node) {
    if (node == NULL) return 0;
    return node->decref_callback(node);
}

struct ast_node* ast_node_incref(struct ast_node* node) {
    node->rc++;
    return node;
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
    ast_node_decref_callback_t* decref_callback
) {
    struct ast_statement result = {
        .node = ast_node_init(
            AST_NODE_STATEMENT,
            token_literal_callback,
            string_callback,
            decref_callback
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

size_t ast_statement_decref(struct ast_statement* statement) {
    if (statement == NULL) return 0;
    return ast_node_decref(&statement->node);
}

struct ast_expression ast_expression_init(
    enum ast_expression_type type,
    ast_node_token_literal_callback_t* token_literal_callback,
    ast_node_string_callback_t* string_callback,
    ast_node_decref_callback_t* decref_callback
) {
    struct ast_expression result = {
        .node = ast_node_init(
            AST_NODE_EXPRESSION,
            token_literal_callback,
            string_callback,
            decref_callback
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

size_t ast_expression_decref(struct ast_expression* expression) {
    if (expression == NULL) return 0;
    return ast_node_decref(&expression->node);
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

static size_t program_decref(struct ast_node* node) {
    node->rc--;
    if (node->rc > 0) return node->rc;
    auto self = (struct ast_program*)node;
    for (size_t i = 0; i < self->statements.len; i++) {
        if (ast_statement_decref(self->statements.ptr[i]) == 0) {
            free(self->statements.ptr[i]);
        }
    }
    BUF_FREE(self->statements);
    free(self);
    return 0;
}

struct ast_program* ast_program_init(struct ast_statement_buf statements) {
    struct ast_program* self = malloc(sizeof(*self));
    self->node =
        ast_node_init(AST_NODE_PROGRAM, program_token_literal, program_string, program_decref);
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

static size_t let_statement_decref(struct ast_node* node) {
    node->rc--;
    if (node->rc > 0) return node->rc;
    auto self = (struct ast_let_statement*)node;
    STRING_FREE(self->token.literal);
    ast_node_decref(&self->name->expression.node);
    free(self->name);
    if (self->value != NULL) {
        if (ast_expression_decref(self->value) == 0) {
            free(self->value);
        }
    }
    return 0;
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
        let_statement_decref
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

static size_t return_statement_decref(struct ast_node* node) {
    node->rc--;
    if (node->rc > 0) return node->rc;
    auto self = (struct ast_return_statement*)node;
    STRING_FREE(self->token.literal);
    if (self->return_value) {
        if (ast_expression_decref(self->return_value) == 0) {
            free(self->return_value);
        }
    }
    return 0;
}

struct ast_return_statement*
ast_return_statement_init(struct token token, struct ast_expression* return_value) {
    struct ast_return_statement* self = malloc(sizeof(*self));
    self->statement = ast_statement_init(
        AST_STATEMENT_RETURN,
        return_statement_token_literal,
        return_statement_string,
        return_statement_decref
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

static size_t expression_statement_decref(struct ast_node* node) {
    node->rc--;
    if (node->rc > 0) return node->rc;
    auto self = (struct ast_expression_statement*)node;
    STRING_FREE(self->token.literal);
    if (self->expression) {
        if (ast_expression_decref(self->expression) == 0) {
            free(self->expression);
        }
    }
    return 0;
}

struct ast_expression_statement*
ast_expression_statement_init(struct token token, struct ast_expression* expression) {
    struct ast_expression_statement* self = malloc(sizeof(*self));
    self->statement = ast_statement_init(
        AST_STATEMENT_EXPRESSION,
        expression_statement_token_literal,
        expression_statement_string,
        expression_statement_decref
    );
    self->token = token;
    self->expression = expression;
    return self;
}

static struct string block_statement_token_literal(const struct ast_node* node) {
    const struct ast_block_statement* self = (const struct ast_block_statement*)node;
    return self->token.literal;
}

static struct string block_statement_string(const struct ast_node* node) {
    const struct ast_block_statement* self = (const struct ast_block_statement*)node;
    struct string buf = {0};
    for (size_t i = 0; i < self->statements.len; i++) {
        struct string statement_str = ast_statement_string(self->statements.ptr[i]);
        string_append(&buf, statement_str);
        STRING_FREE(statement_str);
    }
    return buf;
}

static size_t block_statement_decref(struct ast_node* node) {
    node->rc--;
    if (node->rc > 0) return node->rc;
    auto self = (struct ast_block_statement*)node;
    STRING_FREE(self->token.literal);
    for (size_t i = 0; i < self->statements.len; i++) {
        if (ast_statement_decref(self->statements.ptr[i]) == 0) {
            free(self->statements.ptr[i]);
        }
    }
    BUF_FREE(self->statements);
    return 0;
}

struct ast_block_statement*
ast_block_statement_init(struct token token, struct ast_statement_buf statements) {
    struct ast_block_statement* self = malloc(sizeof(*self));
    self->statement = ast_statement_init(
        AST_STATEMENT_BLOCK,
        block_statement_token_literal,
        block_statement_string,
        block_statement_decref
    );
    self->token = token;
    self->statements = statements;
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

static size_t identifier_decref(struct ast_node* node) {
    node->rc--;
    if (node->rc > 0) return node->rc;
    auto self = (struct ast_identifier*)node;
    STRING_FREE(self->token.literal);
    STRING_FREE(self->value);
    return 0;
}

struct ast_identifier* ast_identifier_init(struct token token, struct string value) {
    struct ast_identifier* self = malloc(sizeof(*self));
    self->expression = ast_expression_init(
        AST_EXPRESSION_IDENTIFIER,
        identifier_token_literal,
        identifier_string,
        identifier_decref
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

static size_t integer_literal_decref(struct ast_node* node) {
    node->rc--;
    if (node->rc > 0) return node->rc;
    auto self = (struct ast_integer_literal*)node;
    STRING_FREE(self->token.literal);
    return 0;
}

struct ast_integer_literal* ast_integer_literal_init(struct token token, int64_t value) {
    struct ast_integer_literal* self = malloc(sizeof(*self));
    self->expression = ast_expression_init(
        AST_EXPRESSION_INTEGER_LITERAL,
        integer_literal_token_literal,
        integer_literal_string,
        integer_literal_decref
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

static size_t prefix_expression_decref(struct ast_node* node) {
    node->rc--;
    if (node->rc > 0) return node->rc;
    auto self = (struct ast_prefix_expression*)node;
    STRING_FREE(self->token.literal);
    STRING_FREE(self->op);
    if (ast_expression_decref(self->right) == 0) {
        free(self->right);
    }
    return 0;
}

struct ast_prefix_expression*
ast_prefix_expression_init(struct token token, struct string op, struct ast_expression* right) {
    struct ast_prefix_expression* self = malloc(sizeof(*self));
    self->expression = ast_expression_init(
        AST_EXPRESSION_PREFIX,
        prefix_expression_token_literal,
        prefix_expression_string,
        prefix_expression_decref
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

static size_t infix_expression_decref(struct ast_node* node) {
    node->rc--;
    if (node->rc > 0) return node->rc;
    auto self = (struct ast_infix_expression*)node;
    STRING_FREE(self->token.literal);
    STRING_FREE(self->op);
    if (ast_expression_decref(self->left) == 0) {
        free(self->left);
    }
    if (ast_expression_decref(self->right) == 0) {
        free(self->right);
    }
    return 0;
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
        infix_expression_decref
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

static size_t boolean_decref(struct ast_node* node) {
    node->rc--;
    if (node->rc > 0) return node->rc;
    auto self = (struct ast_boolean*)node;
    STRING_FREE(self->token.literal);
    return 0;
}

static struct ast_node* boolean_dup(const struct ast_node* node) {
    const struct ast_boolean* self = (const struct ast_boolean*)node;
    struct token token = token_dup(self->token);
    return (struct ast_node*)ast_boolean_init(token, self->value);
}

struct ast_boolean* ast_boolean_init(struct token token, bool value) {
    struct ast_boolean* self = malloc(sizeof(*self));
    self->expression = ast_expression_init(
        AST_EXPRESSION_BOOLEAN,
        boolean_token_literal,
        boolean_string,
        boolean_decref
    );
    self->token = token;
    self->value = value;
    return self;
}

static struct string if_expression_token_literal(const struct ast_node* node) {
    const struct ast_if_expression* self = (const struct ast_if_expression*)node;
    return self->token.literal;
}

static struct string if_expression_string(const struct ast_node* node) {
    const struct ast_if_expression* self = (const struct ast_if_expression*)node;
    struct string buf = string_dup(STRING_REF("if"));
    struct string condition_str = ast_expression_string(self->condition);
    string_append(&buf, condition_str);
    STRING_FREE(condition_str);
    string_append(&buf, STRING_REF(" "));
    struct string consequence_str = ast_statement_string(&self->consequence->statement);
    string_append(&buf, consequence_str);
    STRING_FREE(consequence_str);
    if (self->alternative) {
        string_append(&buf, STRING_REF(" else "));
        struct string alternative_str = ast_statement_string(&self->alternative->statement);
        string_append(&buf, alternative_str);
        STRING_FREE(alternative_str);
    }
    return buf;
}

static size_t if_expression_decref(struct ast_node* node) {
    node->rc--;
    if (node->rc > 0) return node->rc;
    auto self = (struct ast_if_expression*)node;
    STRING_FREE(self->token.literal);
    if (ast_expression_decref(self->condition) == 0) {
        free(self->condition);
    }
    if (ast_statement_decref(&self->consequence->statement) == 0) {
        free(self->consequence);
    }
    if (self->alternative) {
        if (ast_statement_decref(&self->alternative->statement) == 0) {
            free(self->alternative);
        }
    }
    return 0;
}

struct ast_if_expression* ast_if_expression_init(
    struct token token,
    struct ast_expression* condition,
    struct ast_block_statement* consequence,
    struct ast_block_statement* alternative
) {
    struct ast_if_expression* self = malloc(sizeof(*self));
    self->expression = ast_expression_init(
        AST_EXPRESSION_IF,
        if_expression_token_literal,
        if_expression_string,
        if_expression_decref
    );
    self->token = token;
    self->condition = condition;
    self->consequence = consequence;
    self->alternative = alternative;
    return self;
}

static struct string function_literal_token_literal(const struct ast_node* node) {
    const struct ast_function_literal* self = (const struct ast_function_literal*)node;
    return self->token.literal;
}

static struct string function_literal_string(const struct ast_node* node) {
    const struct ast_function_literal* self = (const struct ast_function_literal*)node;
    struct string buf = string_dup(ast_node_token_literal(node));
    string_append(&buf, STRING_REF("("));
    for (size_t i = 0; i < self->parameters.len; i++) {
        if (i > 0) {
            string_append(&buf, STRING_REF(", "));
        }
        string_append(&buf, self->parameters.ptr[i]->token.literal);
    }
    string_append(&buf, STRING_REF(") "));
    struct string body_str = ast_statement_string(&self->body->statement);
    string_append(&buf, body_str);
    STRING_FREE(body_str);
    return buf;
}

static size_t function_literal_decref(struct ast_node* node) {
    node->rc--;
    if (node->rc > 0) return node->rc;
    auto self = (struct ast_function_literal*)node;
    STRING_FREE(self->token.literal);
    for (size_t i = 0; i < self->parameters.len; i++) {
        if (ast_expression_decref(&self->parameters.ptr[i]->expression) == 0) {
            free(self->parameters.ptr[i]);
        }
    }
    BUF_FREE(self->parameters);
    if (ast_statement_decref(&self->body->statement) == 0) {
        free(self->body);
    }
    return 0;
}

struct ast_function_literal* ast_function_literal_init(
    struct token token,
    struct function_parameter_buf parameters,
    struct ast_block_statement* body
) {
    struct ast_function_literal* self = malloc(sizeof(*self));
    self->expression = ast_expression_init(
        AST_EXPRESSION_FUNCTION,
        function_literal_token_literal,
        function_literal_string,
        function_literal_decref
    );
    self->token = token;
    self->parameters = parameters;
    self->body = body;
    return self;
}

static struct string call_expression_token_literal(const struct ast_node* node) {
    const struct ast_call_expression* self = (const struct ast_call_expression*)node;
    return self->token.literal;
}

static struct string call_expression_string(const struct ast_node* node) {
    const struct ast_call_expression* self = (const struct ast_call_expression*)node;
    struct string buf = ast_expression_string(self->function);
    string_append(&buf, STRING_REF("("));
    for (size_t i = 0; i < self->arguments.len; i++) {
        if (i > 0) {
            string_append(&buf, STRING_REF(", "));
        }
        struct string argument_str = ast_expression_string(self->arguments.ptr[i]);
        string_append(&buf, argument_str);
        STRING_FREE(argument_str);
    }
    string_append(&buf, STRING_REF(")"));
    return buf;
}

static size_t call_expression_decref(struct ast_node* node) {
    node->rc--;
    if (node->rc > 0) return node->rc;
    auto self = (struct ast_call_expression*)node;
    STRING_FREE(self->token.literal);
    if (ast_expression_decref(self->function) == 0) {
        free(self->function);
    }
    for (size_t i = 0; i < self->arguments.len; i++) {
        if (ast_expression_decref(self->arguments.ptr[i]) == 0) {
            free(self->arguments.ptr[i]);
        }
    }
    BUF_FREE(self->arguments);
    return 0;
}

struct ast_call_expression* ast_call_expression_init(
    struct token token,
    struct ast_expression* function,
    struct ast_call_argument_buf arguments
) {
    struct ast_call_expression* self = malloc(sizeof(*self));
    self->expression = ast_expression_init(
        AST_EXPRESSION_CALL,
        call_expression_token_literal,
        call_expression_string,
        call_expression_decref
    );
    self->token = token;
    self->function = function;
    self->arguments = arguments;
    return self;
}

static struct string string_literal_token_literal(const struct ast_node* node) {
    auto self = (const struct ast_string_literal*)node;
    return self->token.literal;
}

static struct string string_literal_string(const struct ast_node* node) {
    auto self = (const struct ast_string_literal*)node;
    return self->token.literal;
}

static size_t string_literal_decref(struct ast_node* node) {
    node->rc--;
    if (node->rc > 0) return node->rc;
    auto self = (struct ast_string_literal*)node;
    STRING_FREE(self->token.literal);
    STRING_FREE(self->value);
    return 0;
}

struct ast_string_literal* ast_string_literal_init(struct token token, struct string value) {
    struct ast_string_literal* self = malloc(sizeof(*self));
    self->expression = ast_expression_init(
        AST_EXPRESSION_STRING,
        string_literal_token_literal,
        string_literal_string,
        string_literal_decref
    );
    self->token = token;
    self->value = value;
    return self;
}
