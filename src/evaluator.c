#include "monkey/evaluator.h"

#include "monkey/private/stdc.h"

static struct object* eval_expression(struct ast_expression* expression) {
    switch (expression->type) {
        case AST_EXPRESSION_INTEGER_LITERAL:
            return object_int64_init_base(((struct ast_integer_literal*)expression)->value);
        default:
            // [TODO] eval_expression
            return NULL;
    }
}

static struct object* eval_statement(struct ast_statement* statement) {
    switch (statement->type) {
        case AST_STATEMENT_EXPRESSION:
            return eval_expression(((struct ast_expression_statement*)statement)->expression);
        default:
            // [TODO] eval_statement
            return NULL;
    }
}

static struct object* eval_statements(struct ast_statement_buf statements) {
    struct object* result = NULL;

    for (size_t i = 0; i < statements.len; i++) {
        result = eval_statement(statements.ptr[i]);
    }

    return result;
}

struct object* eval(MONKEY_UNUSED struct ast_node* node) {
    switch (node->type) {
        case AST_NODE_EXPRESSION:
            return eval_expression((struct ast_expression*)node);
        case AST_NODE_STATEMENT:
            return eval_statement((struct ast_statement*)node);
        case AST_NODE_PROGRAM:
            return eval_statements(((struct ast_program*)node)->statements);
    }
}
