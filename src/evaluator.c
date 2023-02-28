#include "monkey/evaluator.h"

#include <iso646.h>

#include "monkey/private/stdc.h"

static bool objects_equal(struct object* left, struct object* right) {
    bool result;
    if (left == NULL and right == NULL) result = true;
    if (left == NULL or right == NULL) result = false;
    if (left->type != right->type) result = false;
    switch (left->type) {
        case OBJECT_INT64:
            result = ((struct object_int64*)left)->value == ((struct object_int64*)right)->value;
            break;
        case OBJECT_BOOLEAN:
            result =
                ((struct object_boolean*)left)->value == ((struct object_boolean*)right)->value;
            break;
        case OBJECT_NULL:
            result = true;
            break;
        default:
            abort();
    }
    object_free(left);
    object_free(right);
    return result;
}

static struct object* eval_bang_operator_expression(struct object* right) {
    if (right == NULL) return object_null_init_base();
    bool result;
    if (right->type == OBJECT_BOOLEAN) {
        result = !((struct object_boolean*)right)->value;
    } else if (right->type == OBJECT_NULL) {
        result = true;
    } else {
        result = false;
    }
    object_free(right);
    return object_boolean_init_base(result);
}

static struct object* eval_minus_prefix_operator_expression(struct object* right) {
    if (right == NULL) return object_null_init_base();
    if (right->type != OBJECT_INT64) {
        object_free(right);
        return object_null_init_base();
    }

    int64_t value = ((struct object_int64*)right)->value;
    object_free(right);
    return object_int64_init_base(-value);
}

static struct object* eval_prefix_expression(struct string op, struct object* right) {
    if (right == NULL) return object_null_init_base();
    if (STRING_EQUAL(op, STRING_REF("!"))) {
        return eval_bang_operator_expression(right);
    } else if (STRING_EQUAL(op, STRING_REF("-"))) {
        return eval_minus_prefix_operator_expression(right);
    } else {
        object_free(right);
        return object_null_init_base();
    }
}

static struct object* eval_integer_infix_expression(
    struct string op,
    struct object_int64* left,
    struct object_int64* right
) {
    int64_t left_val = left->value;
    int64_t right_val = right->value;
    object_free(&left->object);
    object_free(&right->object);

    if (STRING_EQUAL(op, STRING_REF("+"))) {
        return object_int64_init_base(left_val + right_val);
    } else if (STRING_EQUAL(op, STRING_REF("-"))) {
        return object_int64_init_base(left_val - right_val);
    } else if (STRING_EQUAL(op, STRING_REF("*"))) {
        return object_int64_init_base(left_val * right_val);
    } else if (STRING_EQUAL(op, STRING_REF("/"))) {
        return object_int64_init_base(left_val / right_val);
    } else if (STRING_EQUAL(op, STRING_REF("<"))) {
        return object_boolean_init_base(left_val < right_val);
    } else if (STRING_EQUAL(op, STRING_REF(">"))) {
        return object_boolean_init_base(left_val > right_val);
    } else {
        return object_null_init_base();
    }
}

static struct object*
eval_infix_expression(struct string op, struct object* left, struct object* right) {
    if (left == NULL || right == NULL) {
        object_free(left);
        object_free(right);
        return object_null_init_base();
    }
    if (STRING_EQUAL(op, STRING_REF("=="))) {
        return object_boolean_init_base(objects_equal(left, right));
    } else if (STRING_EQUAL(op, STRING_REF("!="))) {
        return object_boolean_init_base(!objects_equal(left, right));
    } else if (left->type == OBJECT_INT64 and right->type == OBJECT_INT64) {
        return eval_integer_infix_expression(
            op,
            (struct object_int64*)left,
            (struct object_int64*)right
        );
    } else {
        object_free(left);
        object_free(right);
        return object_null_init_base();
    }
}

static bool is_truthy(struct object* obj) {
    if (obj == NULL) return false;
    if (obj->type == OBJECT_BOOLEAN) {
        return ((struct object_boolean*)obj)->value;
    } else if (obj->type == OBJECT_NULL) {
        return false;
    } else {
        return true;
    }
}

static struct object* eval_expression(struct ast_expression* expression);
static struct object* eval_statements(struct ast_statement_buf statements);

static struct object* eval_if_expression(struct ast_if_expression* expression) {
    struct object* condition = eval_expression(expression->condition);
    if (condition == NULL) return object_null_init_base();
    if (is_truthy(condition)) {
        object_free(condition);
        return eval_statements(expression->consequence->statements);
    } else {
        object_free(condition);
        if (expression->alternative != NULL) {
            return eval_statements(expression->alternative->statements);
        } else {
            return object_null_init_base();
        }
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
        case AST_EXPRESSION_INFIX: {
            struct ast_infix_expression* exp = (struct ast_infix_expression*)expression;
            struct object* left = eval_expression(exp->left);
            struct object* right = eval_expression(exp->right);
            return eval_infix_expression(exp->op, left, right);
        }
        case AST_EXPRESSION_IF:
            return eval_if_expression((struct ast_if_expression*)expression);
        default:
            // [TODO] eval_expression
            return object_null_init_base();
    }
}

static struct object* eval_statement(struct ast_statement* statement) {
    switch (statement->type) {
        case AST_STATEMENT_EXPRESSION:
            return eval_expression(((struct ast_expression_statement*)statement)->expression);
        case AST_STATEMENT_BLOCK:
            return eval_statements(((struct ast_block_statement*)statement)->statements);
        case AST_STATEMENT_RETURN: {
            struct object* val =
                eval_expression(((struct ast_return_statement*)statement)->return_value);
            return object_return_value_init_base(val);
        }
        default:
            // [TODO] eval_statement
            return object_null_init_base();
    }
}

static struct object* eval_statements(struct ast_statement_buf statements) {
    struct object* result = NULL;

    for (size_t i = 0; i < statements.len; i++) {
        object_free(result);
        result = eval_statement(statements.ptr[i]);
        if (result and result->type == OBJECT_RETURN_VALUE) {
            return object_return_value_unwrap(result);
        }
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
