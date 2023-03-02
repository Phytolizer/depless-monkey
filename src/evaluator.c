#include "monkey/evaluator.h"

#include <iso646.h>

#include "monkey/buf.h"
#include "monkey/private/stdc.h"

BUF_T(struct object*, function_argument);
BUF_T(struct environment*, environment);

struct evaluator {
    struct environment_buf envs;
};

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

static bool is_error(struct object* obj) {
    return obj != NULL and obj->type == OBJECT_ERROR;
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

static struct object*
eval_statement(struct evaluator* ev, struct ast_statement* statement, struct environment* env);

static struct object* eval_block_statement(
    struct evaluator* ev,
    struct ast_block_statement* block,
    struct environment* env
) {
    struct object* result = NULL;
    for (size_t i = 0; i < block->statements.len; i++) {
        object_free(result);
        result = eval_statement(ev, block->statements.ptr[i], env);
        if (result != NULL and
            (result->type == OBJECT_RETURN_VALUE or result->type == OBJECT_ERROR)) {
            return result;
        }
    }
    return result;
}

static struct object*
eval_expression(struct evaluator* ev, struct ast_expression* expression, struct environment* env);

static struct object* eval_if_expression(
    struct evaluator* ev,
    struct ast_if_expression* expression,
    struct environment* env
) {
    struct object* condition = eval_expression(ev, expression->condition, env);
    if (is_error(condition)) return condition;

    if (is_truthy(condition)) {
        object_free(condition);
        return eval_block_statement(ev, expression->consequence, env);
    } else {
        object_free(condition);
        if (expression->alternative != NULL) {
            return eval_block_statement(ev, expression->alternative, env);
        } else {
            return object_null_init_base();
        }
    }
}

static struct object* eval_identifier(struct ast_identifier* identifier, struct environment* env) {
    struct object* val = environment_get(env, identifier->value);
    if (val == NULL) {
        return object_error_init_base(
            string_printf("identifier not found: " STRING_FMT, STRING_ARG(identifier->value))
        );
    } else {
        return object_dup(val);
    }
}

static struct function_argument_buf
eval_expressions(struct evaluator* ev, struct ast_call_argument_buf exps, struct environment* env) {
    struct function_argument_buf result = {0};
    for (size_t i = 0; i < exps.len; i++) {
        struct object* evaluated = eval_expression(ev, exps.ptr[i], env);
        if (is_error(evaluated)) {
            for (size_t j = 0; j < i; j++) {
                object_free(result.ptr[j]);
            }
            BUF_FREE(result);
            result = (struct function_argument_buf){0};
            BUF_PUSH(&result, evaluated);
            return result;
        }
        BUF_PUSH(&result, evaluated);
    }
    return result;
}

static struct environment*
extend_function_env(struct object_function* fn, struct function_argument_buf args) {
    struct environment* env = malloc(sizeof(*env));
    environment_init_enclosed(env, fn->env);

    for (size_t i = 0; i < fn->parameters.len; i++) {
        struct string param_name = string_dup(fn->parameters.ptr[i]->value);
        struct object* arg = object_dup(args.ptr[i]);
        environment_set(env, param_name, arg);
    }

    return env;
}

static struct object* unwrap_return_value(struct object* obj) {
    if (obj != NULL and obj->type == OBJECT_RETURN_VALUE) {
        struct object* inner = ((struct object_return_value*)obj)->value;
        ((struct object_return_value*)obj)->value = NULL;
        object_free(obj);
        return inner;
    } else {
        return obj;
    }
}

static struct object*
apply_function(struct evaluator* ev, struct object* fn, struct function_argument_buf args) {
    if (fn->type != OBJECT_FUNCTION) {
        return object_error_init_base(
            string_printf("not a function: " STRING_FMT, STRING_ARG(object_type_string(fn->type)))
        );
    }

    auto function = (struct object_function*)fn;
    struct environment* extended_env = extend_function_env(function, args);
    struct object* evaluated = eval_statement(ev, &function->body->statement, extended_env);
    if (extended_env->outer->rc == 1) {
        environment_free(*extended_env);
        free(extended_env);
    } else {
        BUF_PUSH(&ev->envs, extended_env);
    }
    return unwrap_return_value(evaluated);
}

static struct object*
eval_expression(struct evaluator* ev, struct ast_expression* expression, struct environment* env) {
    switch (expression->type) {
        case AST_EXPRESSION_INTEGER_LITERAL:
            return object_int64_init_base(((struct ast_integer_literal*)expression)->value);
        case AST_EXPRESSION_BOOLEAN:
            return object_boolean_init_base(((struct ast_boolean*)expression)->value);
        case AST_EXPRESSION_PREFIX: {
            auto exp = (struct ast_prefix_expression*)expression;
            struct object* right = eval_expression(ev, exp->right, env);
            if (is_error(right)) return right;

            return eval_prefix_expression(exp->op, right);
        }
        case AST_EXPRESSION_INFIX: {
            auto exp = (struct ast_infix_expression*)expression;
            struct object* left = eval_expression(ev, exp->left, env);
            if (is_error(left)) return left;
            struct object* right = eval_expression(ev, exp->right, env);
            if (is_error(right)) {
                object_free(left);
                return right;
            }

            return eval_infix_expression(exp->op, left, right);
        }
        case AST_EXPRESSION_IF:
            return eval_if_expression(ev, (struct ast_if_expression*)expression, env);
        case AST_EXPRESSION_IDENTIFIER:
            return eval_identifier((struct ast_identifier*)expression, env);
        case AST_EXPRESSION_FUNCTION: {
            auto func = (struct ast_function_literal*)expression;
            struct function_parameter_buf params = {0};
            BUF_RESERVE(&params, func->parameters.len);
            for (size_t i = 0; i < func->parameters.len; i++) {
                BUF_PUSH(
                    &params,
                    (struct ast_identifier*)
                        ast_node_incref(&func->parameters.ptr[i]->expression.node)
                );
            }
            auto body = (struct ast_block_statement*)ast_node_incref(&func->body->statement.node);
            return object_function_init_base(params, body, env);
        }
        case AST_EXPRESSION_CALL: {
            auto call = (struct ast_call_expression*)expression;
            struct object* function = eval_expression(ev, call->function, env);
            if (is_error(function)) return function;
            struct function_argument_buf args = eval_expressions(ev, call->arguments, env);
            if (args.len == 1 and is_error(args.ptr[0])) {
                object_free(function);
                return args.ptr[0];
            }

            struct object* result = apply_function(ev, function, args);
            object_free(function);
            for (size_t i = 0; i < args.len; i++) {
                object_free(args.ptr[i]);
            }
            BUF_FREE(args);
            return result;
        }
        case AST_EXPRESSION_STRING:
            return object_string_init_base(
                string_dup(((struct ast_string_literal*)expression)->value)
            );
        default:
            // [TODO] eval_expression
            return object_null_init_base();
    }
}

static struct object*
eval_statement(struct evaluator* ev, struct ast_statement* statement, struct environment* env) {
    switch (statement->type) {
        case AST_STATEMENT_EXPRESSION:
            return eval_expression(
                ev,
                ((struct ast_expression_statement*)statement)->expression,
                env
            );
        case AST_STATEMENT_BLOCK:
            return eval_block_statement(ev, (struct ast_block_statement*)statement, env);
        case AST_STATEMENT_RETURN: {
            struct object* val =
                eval_expression(ev, ((struct ast_return_statement*)statement)->return_value, env);
            if (is_error(val)) return val;

            return object_return_value_init_base(val);
        }
        case AST_STATEMENT_LET: {
            struct ast_let_statement* let = (struct ast_let_statement*)statement;
            struct object* val = eval_expression(ev, let->value, env);
            if (is_error(val)) return val;
            environment_set(env, string_dup(let->name->value), val);
            return object_null_init_base();
        }
        default:
            // [TODO] eval_statement
            return object_null_init_base();
    }
}

static struct object*
eval_program(struct evaluator* ev, struct ast_program* program, struct environment* env) {
    struct object* result = NULL;

    for (size_t i = 0; i < program->statements.len; i++) {
        object_free(result);
        result = eval_statement(ev, program->statements.ptr[i], env);
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

struct object* eval(struct ast_node* node, struct environment* env) {
    struct evaluator ev = {0};
    struct object* result;
    switch (node->type) {
        case AST_NODE_EXPRESSION:
            result = eval_expression(&ev, (struct ast_expression*)node, env);
            break;
        case AST_NODE_STATEMENT:
            result = eval_statement(&ev, (struct ast_statement*)node, env);
            break;
        case AST_NODE_PROGRAM:
            result = eval_program(&ev, (struct ast_program*)node, env);
            break;
    }
    for (size_t i = 0; i < ev.envs.len; i++) {
        environment_free(*ev.envs.ptr[i]);
        free(ev.envs.ptr[i]);
    }
    BUF_FREE(ev.envs);
    return result;
}
