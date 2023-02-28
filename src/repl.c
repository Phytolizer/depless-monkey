#include "monkey/repl.h"

#include "monkey/evaluator.h"
#include "monkey/lexer.h"
#include "monkey/parser.h"
#include "monkey/string.h"

const struct string PROMPT = STRING_REF_C(">> ");

void repl_start(FILE* in, FILE* out) {
    struct string line = {0};
    while (true) {
        fprintf(out, STRING_FMT, STRING_ARG(PROMPT));
        fflush(out);
        if (!string_getline(&line, in)) {
            fprintf(out, "\n");
            fflush(out);
            break;
        }

        struct lexer lexer;
        lexer_init(&lexer, line);
        struct parser parser;
        parser_init(&parser, &lexer);
        struct ast_program* program = parse_program(&parser);
        if (parser.errors.len > 0) {
            fprintf(out, "parser errors:\n");
            for (size_t i = 0; i < parser.errors.len; i++) {
                fprintf(out, "\t" STRING_FMT "\n", STRING_ARG(parser.errors.ptr[i]));
            }
            ast_node_free(&program->node);
            parser_deinit(&parser);
            continue;
        }

        struct object* result = eval(&program->node);
        if (result != NULL) {
            struct string result_str = object_inspect(result);
            fprintf(out, STRING_FMT "\n", STRING_ARG(result_str));
            STRING_FREE(result_str);
        }
        object_free(result);
        free(result);
        ast_node_free(&program->node);
        parser_deinit(&parser);
    }

    STRING_FREE(line);
}
