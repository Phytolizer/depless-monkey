#include "monkey/evaluator.h"

#include <iso646.h>

#include "monkey/buf.h"
#include "monkey/private/stdc.h"

BUF_T(struct environment*, environment);

struct evaluator {
    struct environment_buf envs;
};

static struct object* builtin_len(struct object_buf args) {
    if (args.len != 1) {
        return object_error_init_base(
            string_printf("wrong number of arguments. got=%zu, want=1", args.len)
        );
    }

    struct object* arg = args.ptr[0];
    switch (arg->type) {
        case OBJECT_STRING:
            return object_int64_init_base(((struct object_string*)arg)->value.length);
        case OBJECT_ARRAY:
            return object_int64_init_base(((struct object_array*)arg)->elements.len);
        default:
            return object_error_init_base(string_printf(
                "argument to `len` not supported, got " STRING_FMT,
                STRING_ARG(object_type_string(arg->type))
            ));
    }
}

static struct object* builtin_first(struct object_buf args) {
    if (args.len != 1) {
        return object_error_init_base(
            string_printf("wrong number of arguments. got=%zu, want=1", args.len)
        );
    }
    if (args.ptr[0]->type != OBJECT_ARRAY) {
        return object_error_init_base(string_printf(
            "argument to `first` must be ARRAY, got " STRING_FMT,
            STRING_ARG(object_type_string(args.ptr[0]->type))
        ));
    }

    struct object_array* arr = (struct object_array*)args.ptr[0];
    if (arr->elements.len > 0) {
        return object_dup(arr->elements.ptr[0]);
    } else {
        return object_null_init_base();
    }
}

static struct object* builtin_last(struct object_buf args) {
    if (args.len != 1) {
        return object_error_init_base(
            string_printf("wrong number of arguments. got=%zu, want=1", args.len)
        );
    }
    if (args.ptr[0]->type != OBJECT_ARRAY) {
        return object_error_init_base(string_printf(
            "argument to `last` must be ARRAY, got " STRING_FMT,
            STRING_ARG(object_type_string(args.ptr[0]->type))
        ));
    }

    struct object_array* arr = (struct object_array*)args.ptr[0];
    if (arr->elements.len > 0) {
        return object_dup(arr->elements.ptr[arr->elements.len - 1]);
    } else {
        return object_null_init_base();
    }
}

static struct object* builtin_rest(struct object_buf args) {
    if (args.len != 1) {
        return object_error_init_base(
            string_printf("wrong number of arguments. got=%zu, want=1", args.len)
        );
    }
    if (args.ptr[0]->type != OBJECT_ARRAY) {
        return object_error_init_base(string_printf(
            "argument to `rest` must be ARRAY, got " STRING_FMT,
            STRING_ARG(object_type_string(args.ptr[0]->type))
        ));
    }

    struct object_array* arr = (struct object_array*)args.ptr[0];
    if (arr->elements.len > 0) {
        struct object_buf elements = {0};
        BUF_RESERVE(&elements, arr->elements.len - 1);
        for (size_t i = 1; i < arr->elements.len; i += 1) {
            BUF_PUSH(&elements, object_dup(arr->elements.ptr[i]));
        }
        return object_array_init_base(elements);
    } else {
        return object_null_init_base();
    }
}

static struct object* builtin_push(struct object_buf args) {
    if (args.len != 2) {
        return object_error_init_base(
            string_printf("wrong number of arguments. got=%zu, want=2", args.len)
        );
    }
    if (args.ptr[0]->type != OBJECT_ARRAY) {
        return object_error_init_base(string_printf(
            "argument to `push` must be ARRAY, got " STRING_FMT,
            STRING_ARG(object_type_string(args.ptr[0]->type))
        ));
    }

    struct object_array* arr = (struct object_array*)args.ptr[0];
    size_t len = arr->elements.len;
    struct object_buf elements = {0};
    BUF_RESERVE(&elements, len + 1);
    for (size_t i = 0; i < len; i += 1) {
        BUF_PUSH(&elements, object_dup(arr->elements.ptr[i]));
    }
    BUF_PUSH(&elements, object_dup(args.ptr[1]));
    return object_array_init_base(elements);
}

static struct evaluator evaluator_new(struct environment* env) {
    struct evaluator evaluator = {
        .envs = {0},
    };
    environment_set(env, STRING_REF("len"), object_builtin_init_base(&builtin_len));
    environment_set(env, STRING_REF("first"), object_builtin_init_base(&builtin_first));
    environment_set(env, STRING_REF("last"), object_builtin_init_base(&builtin_last));
    environment_set(env, STRING_REF("rest"), object_builtin_init_base(&builtin_rest));
    environment_set(env, STRING_REF("push"), object_builtin_init_base(&builtin_push));
    return evaluator;
}

static void evaluator_free(struct evaluator evaluator) {
    for (size_t i = 0; i < evaluator.envs.len; i += 1) {
        environment_free(*evaluator.envs.ptr[i]);
        free(evaluator.envs.ptr[i]);
    }
    BUF_FREE(evaluator.envs);
}

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

static struct object* eval_string_infix_expression(
    struct string op,
    struct object_string* left,
    struct object_string* right
) {
    if (STRING_EQUAL(op, STRING_REF("+"))) {
        struct string result =
            string_printf(STRING_FMT STRING_FMT, STRING_ARG(left->value), STRING_ARG(right->value));
        object_free(&left->object);
        object_free(&right->object);
        return object_string_init_base(result);
    } else {
        object_free(&left->object);
        object_free(&right->object);
        return object_error_init_base(string_printf(
            "unknown operator: " STRING_FMT " " STRING_FMT " " STRING_FMT,
            STRING_ARG(object_type_string(OBJECT_STRING)),
            STRING_ARG(op),
            STRING_ARG(object_type_string(OBJECT_STRING))
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
    } else if (left->type == OBJECT_STRING and right->type == OBJECT_STRING) {
        return eval_string_infix_expression(
            op,
            (struct object_string*)left,
            (struct object_string*)right
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
    if (val != NULL) {
        return object_dup(val);
    } else {
        return object_error_init_base(
            string_printf("identifier not found: " STRING_FMT, STRING_ARG(identifier->value))
        );
    }
}

static struct object_buf
eval_expressions(struct evaluator* ev, struct ast_expression_buf exps, struct environment* env) {
    struct object_buf result = {0};
    for (size_t i = 0; i < exps.len; i++) {
        struct object* evaluated = eval_expression(ev, exps.ptr[i], env);
        if (is_error(evaluated)) {
            for (size_t j = 0; j < i; j++) {
                object_free(result.ptr[j]);
            }
            BUF_FREE(result);
            result = (struct object_buf){0};
            BUF_PUSH(&result, evaluated);
            return result;
        }
        BUF_PUSH(&result, evaluated);
    }
    return result;
}

static struct environment* extend_function_env(struct object_function* fn, struct object_buf args) {
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
apply_function(struct evaluator* ev, struct object* fn, struct object_buf args) {
    switch (fn->type) {
        case OBJECT_FUNCTION: {
            auto function = (struct object_function*)fn;
            if (function->parameters.len != args.len) {
                return object_error_init_base(string_printf(
                    "wrong number of arguments: expected %zu, got %zu",
                    function->parameters.len,
                    args.len
                ));
            }
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
        case OBJECT_BUILTIN: {
            auto builtin = (struct object_builtin*)fn;
            return builtin->fn(args);
        }
        default:
            return object_error_init_base(string_printf(
                "not a function: " STRING_FMT,
                STRING_ARG(object_type_string(fn->type))
            ));
    }
}

static struct object*
eval_array_index_expression(struct object_array* array, struct object_int64* index) {
    int64_t i = index->value;
    size_t max = array->elements.len - 1;
    object_free(&index->object);
    if (i < 0 or (size_t) i > max) {
        object_free(&array->object);
        return object_null_init_base();
    } else {
        struct object* result = object_dup(array->elements.ptr[i]);
        object_free(&array->object);
        return result;
    }
}

static struct object* eval_index_expression(struct object* left, struct object* index) {
    if (left->type == OBJECT_ARRAY and index->type == OBJECT_INTEGER) {
        return eval_array_index_expression((struct object_array*)left, (struct object_int64*)index);
    } else {
        return object_error_init_base(string_printf(
            "index operator not supported: " STRING_FMT,
            STRING_ARG(object_type_string(left->type))
        ));
    }
}

static struct object*
eval_hash_literal(struct evaluator* ev, struct ast_hash_literal* hash, struct environment* env) {
    struct object_hash_table table;
    object_hash_table_init(&table);

    for (const struct ast_expression_hash_bucket* bucket = ast_expression_hash_first(&hash->pairs);
         bucket != NULL;
         bucket = ast_expression_hash_next(&hash->pairs, bucket)) {
        struct object* key = eval_expression(ev, bucket->key, env);
        if (is_error(key)) {
            object_hash_table_free(&table);
            return key;
        }
        if (!object_is_hashable(key)) {
            object_hash_table_free(&table);
            object_free(key);
            return object_error_init_base(string_printf(
                "unusable as hash key: " STRING_FMT,
                STRING_ARG(object_type_string(key->type))
            ));
        }

        struct object_hash_key hash_key = object_hash_key(key);

        struct object* value = eval_expression(ev, bucket->value, env);
        if (is_error(value)) {
            object_hash_table_free(&table);
            object_free(key);
            return value;
        }

        object_hash_table_insert(&table, hash_key, key, value);
    }

    return object_hash_init_base(table);
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
            struct object_buf args = eval_expressions(ev, call->arguments, env);
            if (args.len == 1 and is_error(args.ptr[0])) {
                object_free(function);
                struct object* err = args.ptr[0];
                BUF_FREE(args);
                return err;
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
        case AST_EXPRESSION_ARRAY: {
            auto array = (struct ast_array_literal*)expression;
            struct object_buf elements = eval_expressions(ev, array->elements, env);
            if (elements.len == 1 and is_error(elements.ptr[0])) {
                struct object* err = elements.ptr[0];
                BUF_FREE(elements);
                return err;
            }
            return object_array_init_base(elements);
        }
        case AST_EXPRESSION_INDEX: {
            auto exp = (struct ast_index_expression*)expression;
            struct object* left = eval_expression(ev, exp->left, env);
            if (is_error(left)) return left;
            struct object* index = eval_expression(ev, exp->index, env);
            if (is_error(index)) {
                object_free(left);
                return index;
            }

            return eval_index_expression(left, index);
        }
        case AST_EXPRESSION_HASH:
            return eval_hash_literal(ev, (struct ast_hash_literal*)expression, env);
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
    struct evaluator ev = evaluator_new(env);
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
    evaluator_free(ev);
    return result;
}
