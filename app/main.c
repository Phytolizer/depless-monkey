#include <monkey/evaluator.h>
#include <monkey/lexer.h>
#include <monkey/parser.h>
#include <monkey/repl.h>

#include "./slurp.h"

static bool eval_source(struct string program, struct environment* env) {
    struct lexer lexer;
    lexer_init(&lexer, program);
    struct parser parser;
    parser_init(&parser, &lexer);
    struct ast_program* ast = parse_program(&parser);
    if (parser.errors.len > 0) {
        fprintf(stderr, "parser errors:\n");
        for (size_t i = 0; i < parser.errors.len; i++) {
            fprintf(stderr, "\t" STRING_FMT "\n", STRING_ARG(parser.errors.ptr[i]));
        }
        ast_node_decref(&ast->node);
        parser_deinit(&parser);
        return false;
    }
    eval(&ast->node, env);
    ast_node_decref(&ast->node);
    parser_deinit(&parser);
    return true;
}

int main(int argc, char** argv) {
    struct environment env;
    environment_init(&env);
    if (argc > 2) {
        fprintf(stderr, "Usage: monkey [script]\n");
        exit(1);
    } else if (argc == 2) {
        struct string initial_program = slurp_file(STRING_REF_FROM_C(argv[1]));
        if (!eval_source(initial_program, &env)) {
            exit(1);
        }
        repl_start(stdin, stdout, &env);
    } else {
        printf(
            "Hello! This is the Monkey programming language!\n"
            "Feel free to type in commands\n"
        );
        repl_start(stdin, stdout, &env);
    }
    environment_free(env);
}
