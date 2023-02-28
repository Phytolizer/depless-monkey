#include "monkey/evaluator.h"

#include <iso646.h>

#include "monkey/private/stdc.h"

static bool objects_equal(struct object* left, struct object* right) {
    bool result;
    if (left == NULL and right == NULL) result = true;
    if (left == NULL or right == NULL) result = false;
    if (left->type != right->type) result = false;
    switch (left->type) {
        case OBJECT_INTEGER:
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
    if (right->type != OBJECT_INTEGER) {
        struct string right_type = object_type_string(right->type);
        object_free(right);
        return object_error_init_base(
            string_printf("unknown operator: -" STRING_FMT, STRING_ARG(right_type))
        );
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
        return object_error_init_base(string_printf(
            "unknown operator: " STRING_FMT STRING_FMT,
            STRING_ARG(op),
            STRING_ARG(object_type_string(right->type))
        ));
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
        return object_error_init_base(string_printf(
            "unknown operator: " STRING_FMT " " STRING_FMT " " STRING_FMT,
            STRING_ARG(object_type_string(OBJECT_INTEGER)),
            STRING_ARG(op),
            STRING_ARG(object_type_string(OBJECT_INTEGER))
        ));
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
    } else if (left->type == OBJECT_INTEGER and right->type == OBJECT_INTEGER) {
        return eval_integer_infix_expression(
            op,
            (struct object_int64*)left,
            (struct object_int64*)right
        );
    } else if (left->type != right->type) {
        struct string left_type = object_type_string(left->type);
        object_free(left);
        struct string right_type = object_type_string(right->type);
        object_free(right);
        return object_error_init_base(string_printf(
            "type mismatch: " STRING_FMT " " STRING_FMT " " STRING_FMT,
            STRING_ARG(left_type),
            STRING_ARG(op),
            STRING_ARG(right_type)
        ));
    } else {
        struct string left_type = object_type_string(left->type);
        object_free(left);
        struct string right_type = object_type_string(right->type);
        object_free(right);
        return object_error_init_base(string_printf(
            "unknown operator: " STRING_FMT " " STRING_FMT " " STRING_FMT,
            STRING_ARG(left_type),
            STRING_ARG(op),
            STRING_ARG(right_type)
        ));
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

static struct object* eval_statement(struct ast_statement* statement);

static struct object* eval_block_statement(struct ast_block_statement* block) {
    struct object* result = NULL;
    for (size_t i = 0; i < block->statements.len; i++) {
        object_free(result);
        result = eval_statement(block->statements.ptr[i]);
        if (result != NULL and
            (result->type == OBJECT_RETURN_VALUE or result->type == OBJECT_ERROR)) {
            return result;
        }
    }
    return result;
}

static struct object* eval_expression(struct ast_expression* expression);

static struct object* eval_if_expression(struct ast_if_expression* expression) {
    struct object* condition = eval_expression(expression->condition);
    if (condition == NULL) return object_null_init_base();
    if (is_truthy(condition)) {
        object_free(condition);
        return eval_block_statement(expression->consequence);
    } else {
        object_free(condition);
        if (expression->alternative != NULL) {
            return eval_block_statement(expression->alternative);
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
            return eval_block_statement((struct ast_block_statement*)statement);
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

static struct object* eval_program(struct ast_program* program) {
    struct object* result = NULL;

    for (size_t i = 0; i < program->statements.len; i++) {
        object_free(result);
        result = eval_statement(program->statements.ptr[i]);
        if (result) {
            switch (result->type) {
                case OBJECT_RETURN_VALUE:
                    return object_return_value_unwrap(result);
                case OBJECT_ERROR:
                    return result;
                default:
                    break;
            }
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
            return eval_program((struct ast_program*)node);
    }
}
