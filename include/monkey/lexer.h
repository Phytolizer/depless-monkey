#ifndef MONKEY_LEXER_H_
#define MONKEY_LEXER_H_

#include "monkey/token.h"

struct lexer {
    struct string input;
    size_t position;
    size_t read_position;
    char ch;
};

extern void lexer_init(struct lexer* lexer, struct string input);
extern struct token lexer_next_token(struct lexer* lexer);

#endif // MONKEY_LEXER_H_
