#ifndef PARSE_H
#define PARSE_H
#include "ast.h"
#include "token.h"
typedef struct Parser Parser;
struct Parser {
    TokenArray *array;
    size_t current;
};

Expr *parse_term(Parser *parser);
Parser create_parser(TokenArray *array);
Expr *parse_expression(Parser *parser);
Expr *p_term(Parser *parser);
Expr *p_primary(Parser *parser);
Expr *parse(Parser *parser);
Token *consume(Parser *parser, TokenList type, const char *message);
#endif /* PARSE_H */
