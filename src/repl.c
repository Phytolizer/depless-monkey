#include "monkey/repl.h"

#include "monkey/lexer.h"
#include "monkey/string.h"

const struct string PROMPT = STRING_REF_C(">> ");

extern void repl_start(FILE* in, FILE* out) {
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
        for (struct token tok = lexer_next_token(&lexer); tok.type != TOKEN_EOF;
             tok = lexer_next_token(&lexer)) {
            fprintf(
                out,
                "{Type:" STRING_FMT " Literal:" STRING_FMT "}\n",
                STRING_ARG(token_type_string(tok.type)),
                STRING_ARG(tok.literal)
            );
            STRING_FREE(tok.literal);
        }
    }

    STRING_FREE(line);
}
