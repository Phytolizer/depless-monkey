#ifndef MONKEY_TOKEN_H_
#define MONKEY_TOKEN_H_

#include "monkey/string.h"

enum token_type {
#define X(x, _y) TOKEN_##x,
#include "monkey/private/token_types.inc"
#undef X
};

struct token {
    enum token_type type;
    struct string literal;
};

extern struct string token_type_string(enum token_type type);
extern enum token_type lookup_ident(struct string ident);
extern struct token token_dup(struct token token);

#endif  // MONKEY_TOKEN_H_
