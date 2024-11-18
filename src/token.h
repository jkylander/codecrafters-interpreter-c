// vim: ft=c
#ifndef TOKEN_H
#define TOKEN_H
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sysexits.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS,
    SEMICOLON, SLASH, STAR,

    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,

    IDENTIFIER, STRING, NUMBER,

    AND, CLASS, ELSE, FALSE, FUN,
    FOR, IF, NIL, OR, PRINT, RETURN,
    SUPER, THIS, TRUE, VAR, WHILE,

    EOF_TOKEN,

    TOKEN_COUNT,
} TokenList;

typedef struct {
    char *contents;
    size_t length;
} FileData;

typedef struct {
    TokenList type;
    char *lexeme;
    void *literal;
    size_t line;
    size_t length;
    bool error;
    bool comment;
} Token;

typedef struct {
    Token *tokens;
    size_t count;
    size_t capacity;
} TokenArray;

TokenArray create_token_array(size_t initial_capacity);
void free_token_array(TokenArray *array);
void token_array_add(TokenArray *array, Token token);
void print_token_array(TokenArray token_array);
const char *str_from_token(TokenList type);

TokenArray tokenize(char *file, bool *error);

#endif /* TOKEN_H */
