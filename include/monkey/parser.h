#ifndef MONKEY_PARSER_H_
#define MONKEY_PARSER_H_

#include "monkey/ast.h"
#include "monkey/buf.h"
#include "monkey/lexer.h"
#include "monkey/string.h"

BUF_T(struct string, parser_error);

struct parser;

struct parser {
    struct lexer* l;

    struct token cur_token;
    struct token peek_token;

    struct parser_error_buf errors;
};

extern void parser_init(struct parser* parser, struct lexer* l);
extern void parser_deinit(struct parser* parser);
extern struct ast_program* parse_program(struct parser* p);

#endif  // MONKEY_PARSER_H_
