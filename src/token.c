#include "monkey/token.h"

#include <stdlib.h>

struct string token_type_string(enum token_type type) {
    switch (type) {
#define X(x, y) \
    case TOKEN_##x: \
        return STRING_REF(y);
#include "monkey/private/token_types.inc"
    }

    abort();
}

enum token_type lookup_ident(struct string ident) {
    struct {
        struct string text;
        enum token_type type;
    } keywords[] = {
        {STRING_REF_C("fn"), TOKEN_FUNCTION},
        {STRING_REF_C("let"), TOKEN_LET},
        {STRING_REF_C("true"), TOKEN_TRUE},
        {STRING_REF_C("false"), TOKEN_FALSE},
        {STRING_REF_C("if"), TOKEN_IF},
        {STRING_REF_C("else"), TOKEN_ELSE},
        {STRING_REF_C("return"), TOKEN_RETURN},
    };

    for (size_t i = 0; i < sizeof(keywords) / sizeof(*keywords); i++) {
        if (STRING_EQUAL(ident, keywords[i].text)) {
            return keywords[i].type;
        }
    }
    return TOKEN_IDENT;
}

struct token token_dup(struct token token) {
    struct token result = {
        .type = token.type,
        .literal = string_dup(token.literal),
    };
    return result;
}
