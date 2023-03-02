#ifndef MONKEY_AST_H_
#define MONKEY_AST_H_

#include <stdint.h>

#include "monkey/buf.h"
#include "monkey/string.h"
#include "monkey/token.h"

struct ast_node;

typedef struct string ast_node_token_literal_callback_t(const struct ast_node* node);
typedef struct string ast_node_string_callback_t(const struct ast_node* node);
typedef size_t ast_node_decref_callback_t(struct ast_node* node);

enum ast_node_type {
#define X(x) AST_NODE_##x,
#include "monkey/private/ast_node_types.inc"
#undef X
};

struct ast_node {
    enum ast_node_type type;
    ast_node_token_literal_callback_t* token_literal_callback;
    ast_node_string_callback_t* string_callback;
    ast_node_decref_callback_t* decref_callback;
    size_t rc;
};

extern struct ast_node ast_node_init(
    enum ast_node_type type,
    ast_node_token_literal_callback_t* token_literal_callback,
    ast_node_string_callback_t* string_callback,
    ast_node_decref_callback_t* decref_callback
);

extern struct string ast_node_token_literal(const struct ast_node* node);
extern struct string ast_node_string(const struct ast_node* node);
extern size_t ast_node_decref(struct ast_node* node);
extern struct ast_node* ast_node_incref(struct ast_node* node);

enum ast_statement_type {
#define X(x) AST_STATEMENT_##x,
#include "monkey/private/ast_statement_types.inc"
#undef X
};

extern struct string ast_statement_type_string(enum ast_statement_type type);

struct ast_statement {
    struct ast_node node;
    enum ast_statement_type type;
};

extern struct ast_statement ast_statement_init(
    enum ast_statement_type type,
    ast_node_token_literal_callback_t* token_literal_callback,
    ast_node_string_callback_t* string_callback,
    ast_node_decref_callback_t* decref_callback
);
extern struct string ast_statement_token_literal(const struct ast_statement* statement);
extern struct string ast_statement_string(const struct ast_statement* statement);
extern size_t ast_statement_decref(struct ast_statement* statement);

enum ast_expression_type {
#define X(x) AST_EXPRESSION_##x,
#include "monkey/private/ast_expression_types.inc"
#undef X
};

extern struct string ast_expression_type_string(enum ast_expression_type type);

struct ast_expression {
    struct ast_node node;
    enum ast_expression_type type;
};

extern struct ast_expression ast_expression_init(
    enum ast_expression_type type,
    ast_node_token_literal_callback_t* token_literal_callback,
    ast_node_string_callback_t* string_callback,
    ast_node_decref_callback_t* decref_callback
);
extern struct string ast_expression_token_literal(const struct ast_expression* expression);
extern struct string ast_expression_string(const struct ast_expression* expression);
extern size_t ast_expression_decref(struct ast_expression* expression);

BUF_T(struct ast_statement*, ast_statement);

struct ast_program {
    struct ast_node node;
    struct ast_statement_buf statements;
};

extern struct ast_program* ast_program_init(struct ast_statement_buf statements);

struct ast_identifier;

struct ast_let_statement {
    struct ast_statement statement;
    struct token token;
    struct ast_identifier* name;
    struct ast_expression* value;
};

extern struct ast_let_statement* ast_let_statement_init(
    struct token token,
    struct ast_identifier* name,
    struct ast_expression* value
);
static inline struct ast_statement* ast_let_statement_init_base(
    struct token token,
    struct ast_identifier* name,
    struct ast_expression* value
) {
    return &ast_let_statement_init(token, name, value)->statement;
}

struct ast_return_statement {
    struct ast_statement statement;
    struct token token;
    struct ast_expression* return_value;
};

extern struct ast_return_statement*
ast_return_statement_init(struct token token, struct ast_expression* return_value);
static inline struct ast_statement*
ast_return_statement_init_base(struct token token, struct ast_expression* return_value) {
    return &ast_return_statement_init(token, return_value)->statement;
}

struct ast_expression_statement {
    struct ast_statement statement;
    struct token token;
    struct ast_expression* expression;
};

extern struct ast_expression_statement*
ast_expression_statement_init(struct token token, struct ast_expression* expression);
static inline struct ast_statement*
ast_expression_statement_init_base(struct token token, struct ast_expression* expression) {
    return &ast_expression_statement_init(token, expression)->statement;
}

struct ast_block_statement {
    struct ast_statement statement;
    struct token token;
    struct ast_statement_buf statements;
};

extern struct ast_block_statement*
ast_block_statement_init(struct token token, struct ast_statement_buf statements);
static inline struct ast_statement*
ast_block_statement_init_base(struct token token, struct ast_statement_buf statements) {
    return &ast_block_statement_init(token, statements)->statement;
}

struct ast_identifier {
    struct ast_expression expression;
    struct token token;
    struct string value;
};

extern struct ast_identifier* ast_identifier_init(struct token token, struct string value);
static inline struct ast_expression*
ast_identifier_init_base(struct token token, struct string value) {
    return &ast_identifier_init(token, value)->expression;
}

struct ast_integer_literal {
    struct ast_expression expression;
    struct token token;
    int64_t value;
};

extern struct ast_integer_literal* ast_integer_literal_init(struct token token, int64_t value);
static inline struct ast_expression*
ast_integer_literal_init_base(struct token token, int64_t value) {
    return &ast_integer_literal_init(token, value)->expression;
}

struct ast_prefix_expression {
    struct ast_expression expression;
    struct token token;
    struct string op;
    struct ast_expression* right;
};

extern struct ast_prefix_expression*
ast_prefix_expression_init(struct token token, struct string op, struct ast_expression* right);
static inline struct ast_expression* ast_prefix_expression_init_base(
    struct token token,
    struct string op,
    struct ast_expression* right
) {
    return &ast_prefix_expression_init(token, op, right)->expression;
}

struct ast_infix_expression {
    struct ast_expression expression;
    struct token token;
    struct ast_expression* left;
    struct string op;
    struct ast_expression* right;
};

extern struct ast_infix_expression* ast_infix_expression_init(
    struct token token,
    struct ast_expression* left,
    struct string op,
    struct ast_expression* right
);
static inline struct ast_expression* ast_infix_expression_init_base(
    struct token token,
    struct ast_expression* left,
    struct string op,
    struct ast_expression* right
) {
    return &ast_infix_expression_init(token, left, op, right)->expression;
}

struct ast_boolean {
    struct ast_expression expression;
    struct token token;
    bool value;
};

extern struct ast_boolean* ast_boolean_init(struct token token, bool value);
static inline struct ast_expression* ast_boolean_init_base(struct token token, bool value) {
    return &ast_boolean_init(token, value)->expression;
}

struct ast_if_expression {
    struct ast_expression expression;
    struct token token;
    struct ast_expression* condition;
    struct ast_block_statement* consequence;
    struct ast_block_statement* alternative;
};

extern struct ast_if_expression* ast_if_expression_init(
    struct token token,
    struct ast_expression* condition,
    struct ast_block_statement* consequence,
    struct ast_block_statement* alternative
);
static inline struct ast_expression* ast_if_expression_init_base(
    struct token token,
    struct ast_expression* condition,
    struct ast_block_statement* consequence,
    struct ast_block_statement* alternative
) {
    return &ast_if_expression_init(token, condition, consequence, alternative)->expression;
}

BUF_T(struct ast_identifier*, function_parameter);

struct ast_function_literal {
    struct ast_expression expression;
    struct token token;
    struct function_parameter_buf parameters;
    struct ast_block_statement* body;
};

extern struct ast_function_literal* ast_function_literal_init(
    struct token token,
    struct function_parameter_buf parameters,
    struct ast_block_statement* body
);
static inline struct ast_expression* ast_function_literal_init_base(
    struct token token,
    struct function_parameter_buf parameters,
    struct ast_block_statement* body
) {
    return &ast_function_literal_init(token, parameters, body)->expression;
}

BUF_T(struct ast_expression*, ast_call_argument);

struct ast_call_expression {
    struct ast_expression expression;
    struct token token;
    struct ast_expression* function;
    struct ast_call_argument_buf arguments;
};

extern struct ast_call_expression* ast_call_expression_init(
    struct token token,
    struct ast_expression* function,
    struct ast_call_argument_buf arguments
);
static inline struct ast_expression* ast_call_expression_init_base(
    struct token token,
    struct ast_expression* function,
    struct ast_call_argument_buf arguments
) {
    return &ast_call_expression_init(token, function, arguments)->expression;
}

#endif  // MONKEY_AST_H_
