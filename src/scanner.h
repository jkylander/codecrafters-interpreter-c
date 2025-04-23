#ifndef SCANNER_H
#define SCANNER_H

#define TOKEN_KINDS                                                            \
    TOKEN_KIND(TOKEN_LEFT_PAREN, "LEFT_PAREN")                                 \
    TOKEN_KIND(TOKEN_RIGHT_PAREN, "RIGHT_PAREN")                               \
    TOKEN_KIND(TOKEN_LEFT_BRACE, "LEFT_BRACE")                                 \
    TOKEN_KIND(TOKEN_RIGHT_BRACE, "RIGHT_BRACE")                               \
    TOKEN_KIND(TOKEN_COMMA, "COMMA")                                           \
    TOKEN_KIND(TOKEN_DOT, "DOT")                                               \
    TOKEN_KIND(TOKEN_MINUS, "MINUS")                                           \
    TOKEN_KIND(TOKEN_PLUS, "PLUS")                                             \
    TOKEN_KIND(TOKEN_SEMICOLON, "SEMICOLON")                                   \
    TOKEN_KIND(TOKEN_SLASH, "SLASH")                                           \
    TOKEN_KIND(TOKEN_STAR, "STAR")                                             \
    TOKEN_KIND(TOKEN_BANG, "BANG")                                             \
    TOKEN_KIND(TOKEN_BANG_EQUAL, "BANG_EQUAL")                                 \
    TOKEN_KIND(TOKEN_EQUAL, "EQUAL")                                           \
    TOKEN_KIND(TOKEN_EQUAL_EQUAL, "EQUAL_EQUAL")                               \
    TOKEN_KIND(TOKEN_GREATER, "GREATER")                                       \
    TOKEN_KIND(TOKEN_GREATER_EQUAL, "GREATER_EQUAL")                           \
    TOKEN_KIND(TOKEN_LESS, "LESS")                                             \
    TOKEN_KIND(TOKEN_LESS_EQUAL, "LESS_EQUAL")                                 \
    TOKEN_KIND(TOKEN_IDENTIFIER, "IDENTIFIER")                                 \
    TOKEN_KIND(TOKEN_STRING, "STRING")                                         \
    TOKEN_KIND(TOKEN_NUMBER, "NUMBER")                                         \
    TOKEN_KIND(TOKEN_AND, "AND")                                               \
    TOKEN_KIND(TOKEN_CLASS, "CLASS")                                           \
    TOKEN_KIND(TOKEN_ELSE, "ELSE")                                             \
    TOKEN_KIND(TOKEN_FALSE, "FALSE")                                           \
    TOKEN_KIND(TOKEN_FOR, "FOR")                                               \
    TOKEN_KIND(TOKEN_FUN, "FUN")                                               \
    TOKEN_KIND(TOKEN_IF, "IF")                                                 \
    TOKEN_KIND(TOKEN_NIL, "NIL")                                               \
    TOKEN_KIND(TOKEN_OR, "OR")                                                 \
    TOKEN_KIND(TOKEN_PRINT, "PRINT")                                           \
    TOKEN_KIND(TOKEN_RETURN, "RETURN")                                         \
    TOKEN_KIND(TOKEN_SUPER, "SUPER")                                           \
    TOKEN_KIND(TOKEN_THIS, "THIS")                                             \
    TOKEN_KIND(TOKEN_TRUE, "TRUE")                                             \
    TOKEN_KIND(TOKEN_VAR, "VAR")                                               \
    TOKEN_KIND(TOKEN_WHILE, "WHILE")                                           \
    TOKEN_KIND(TOKEN_ERROR, "ERROR")                                           \
    TOKEN_KIND(TOKEN_EOF, "EOF")                                               \
    TOKEN_KIND(TOKEN_COUNT, "")

typedef enum {
#define TOKEN_KIND(e, s) e,
    TOKEN_KINDS
#undef TOKEN_KIND
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    int length;
    int line;
} Token;

void initScanner(const char *source);
Token scanToken();
bool lex(const char *source);

#endif /* SCANNER_H */
