#include "monkey/evaluator.h"

#include "monkey/private/stdc.h"

static struct object* eval_bang_operator_expression(struct object* right) {
    if (right == NULL) return NULL;
    bool result;
    if (right->type == OBJECT_BOOLEAN) {
        result = !((struct object_boolean*)right)->value;
    } else if (right->type == OBJECT_NULL) {
        result = true;
    } else {
        result = false;
    }
    object_free(right);
    free(right);
    return object_boolean_init_base(result);
}

static struct object* eval_minus_prefix_operator_expression(struct object* right) {
    if (right == NULL) return NULL;
    if (right->type != OBJECT_INT64) {
        object_free(right);
        free(right);
        return NULL;
    }

    int64_t value = ((struct object_int64*)right)->value;
    object_free(right);
    free(right);
    return object_int64_init_base(-value);
}

static struct object* eval_prefix_expression(struct string op, struct object* right) {
    if (right == NULL) return NULL;
    if (STRING_EQUAL(op, STRING_REF("!"))) {
        return eval_bang_operator_expression(right);
    } else if (STRING_EQUAL(op, STRING_REF("-"))) {
        return eval_minus_prefix_operator_expression(right);
    } else {
        object_free(right);
        free(right);
        return NULL;
    }
}

static struct object* eval_expression(struct ast_expression* expression) {
    switch (expression->type) {
        case AST_EXPRESSION_INTEGER_LITERAL:
            return object_int64_init_base(((struct ast_integer_literal*)expression)->value);
        case AST_EXPRESSION_BOOLEAN:
            return object_boolean_init_base(((struct ast_boolean*)expression)->value);
        case AST_EXPRESSION_PREFIX: {
            struct ast_prefix_expression* exp = (struct ast_prefix_expression*)expression;
            struct object* right = eval_expression(exp->right);
            return eval_prefix_expression(exp->op, right);
        }
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
