#ifndef PARSE_H
#define PARSE_H
#include "ast.h"
#include "stmt.h"
typedef struct Parser Parser;
struct Parser {
    TokenArray *array;
    int current;
    bool hadError;
};

Expr *parse_term(Parser *parser);
Parser create_parser(TokenArray *array);
Expr *parse_expression(Parser *parser);
Expr *p_term(Parser *parser);
Expr *p_primary(Parser *parser);
StmtArray parse(Parser *parser);
Token *consume(Parser *parser, TokenType type, const char *message);
void errorAtCurrent(Parser *parser, const char *message);
#endif /* PARSE_H */
