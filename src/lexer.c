#include "monkey/lexer.h"

#include <iso646.h>

static char peek_char(const struct lexer* l) {
    if (l->read_position >= l->input.length) {
        return '\0';
    } else {
        return l->input.data[l->read_position];
    }
}

static void read_char(struct lexer* l) {
    l->ch = peek_char(l);
    l->position = l->read_position;
    l->read_position += 1;
}

void lexer_init(struct lexer* l, struct string input) {
    l->input = input;
    l->position = 0;
    l->read_position = 0;
    read_char(l);
}

static struct token new_token(enum token_type type, char ch) {
    struct token result = {
        .type = type,
        .literal = string_printf("%c", ch),
    };
    return result;
}

static bool is_letter(char ch) {
    return ('a' <= ch and ch <= 'z') or ('A' <= ch and ch <= 'Z') or ch == '_';
}

static bool is_digit(char ch) {
    return '0' <= ch and ch <= '9';
}

static struct string read_identifier(struct lexer* l) {
    size_t position = l->position;
    while (is_letter(l->ch)) {
        read_char(l);
    }
    return STRING_REF_DATA(l->input.data + position, l->position - position);
}

static struct string read_number(struct lexer* l) {
    size_t position = l->position;
    while (is_digit(l->ch)) {
        read_char(l);
    }
    return STRING_REF_DATA(l->input.data + position, l->position - position);
}

static struct string read_string(struct lexer* l) {
    size_t position = l->position + 1;
    while (true) {
        read_char(l);
        if (l->ch == '"' or l->ch == '\0') {
            break;
        }
    }
    return STRING_REF_DATA(l->input.data + position, l->position - position);
}

static void skip_whitespace(struct lexer* l) {
    while (l->ch == ' ' or l->ch == '\t' or l->ch == '\r' or l->ch == '\n') {
        read_char(l);
    }
}

struct token lexer_next_token(struct lexer* l) {
    struct token tok = {0};

    skip_whitespace(l);

    switch (l->ch) {
        case '=':
            if (peek_char(l) == '=') {
                char ch = l->ch;
                read_char(l);
                struct string literal = string_printf("%c%c", ch, l->ch);
                tok = (struct token){.type = TOKEN_EQ, .literal = literal};
            } else {
                tok = new_token(TOKEN_ASSIGN, l->ch);
            }
            break;
        case '+':
            tok = new_token(TOKEN_PLUS, l->ch);
            break;
        case '-':
            tok = new_token(TOKEN_MINUS, l->ch);
            break;
        case '!':
            if (peek_char(l) == '=') {
                char ch = l->ch;
                read_char(l);
                struct string literal = string_printf("%c%c", ch, l->ch);
                tok = (struct token){.type = TOKEN_NOT_EQ, .literal = literal};
            } else {
                tok = new_token(TOKEN_BANG, l->ch);
            }
            break;
        case '/':
            tok = new_token(TOKEN_SLASH, l->ch);
            break;
        case '*':
            tok = new_token(TOKEN_ASTERISK, l->ch);
            break;
        case '<':
            tok = new_token(TOKEN_LT, l->ch);
            break;
        case '>':
            tok = new_token(TOKEN_GT, l->ch);
            break;
        case ';':
            tok = new_token(TOKEN_SEMICOLON, l->ch);
            break;
        case '(':
            tok = new_token(TOKEN_LPAREN, l->ch);
            break;
        case ')':
            tok = new_token(TOKEN_RPAREN, l->ch);
            break;
        case ',':
            tok = new_token(TOKEN_COMMA, l->ch);
            break;
        case '{':
            tok = new_token(TOKEN_LBRACE, l->ch);
            break;
        case '}':
            tok = new_token(TOKEN_RBRACE, l->ch);
            break;
        case '"':
            tok.type = TOKEN_STRING;
            tok.literal = read_string(l);
            break;
        case '\0':
            tok.type = TOKEN_EOF;
            tok.literal = STRING_REF("");
            break;
        default:
            if (is_letter(l->ch)) {
                tok.literal = read_identifier(l);
                tok.type = lookup_ident(tok.literal);
                return tok;
            } else if (is_digit(l->ch)) {
                tok.literal = read_number(l);
                tok.type = TOKEN_INT;
                return tok;
            } else {
                tok = new_token(TOKEN_ILLEGAL, l->ch);
            }
            break;
    }

    read_char(l);
    return tok;
}
