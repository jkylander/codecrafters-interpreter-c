#include "ast.h"
#include "token.h"
#include "parse.h"
#include <stdio.h>

Parser create_parser(TokenArray *array) {
    Parser parser;
    parser.array= array;
    parser.current = 0;
    return parser;
}
Token *p_peek(Parser *parser) {
    if (parser->current < parser->array->count) {
        return &parser->array->tokens[parser->current];
    }
    return NULL;
}

Token *p_previous(Parser *parser) {
    if (parser->current > 0) {
        return &parser->array->tokens[parser->current - 1];
    }
    return NULL;
}

bool p_isAtEnd(Parser *parser) {
    Token *current = p_peek(parser);
    return current == NULL || current->type == EOF_TOKEN;
}

Token *p_advance(Parser *parser) {
    if (!p_isAtEnd(parser)) {
        parser->current++;
        return p_previous(parser);
    }
    return NULL;
}

bool p_match(Parser *parser, TokenList type) {
    Token *current = p_peek(parser);
    if ((current != NULL) && current->type == type) {
        p_advance(parser);
        return true;
    }
    return false;
}


bool check(Parser *parser, TokenList type) {
    if (p_isAtEnd(parser)) return false;
    return p_peek(parser)->type == type;
}

Expr parse_primary(Parser *parser) {
    Token *current = p_peek(parser);
    if (current == NULL || current->type == EOF_TOKEN) {
        fprintf(stderr, "Error: Unexpected end of input.\n");
        exit(1);
    }

    if (p_match(parser, TRUE) || p_match(parser, FALSE) ||
        p_match(parser, NIL) || p_match(parser, NUMBER) ||
        p_match(parser, STRING)) {
        return create_literal_expr(*p_previous(parser));
    }
    fprintf(stderr, "Error: Unexpected token, got token type %s.\n", str_from_token(p_peek(parser)->type));
    exit(1);
}

Expr parse_expression(Parser *parser) {
    TokenArray *tokens = parser->array;
    while (!p_isAtEnd(parser)) {
        Expr expr = parse_primary(parser);
        print_ast(&expr);
    }
    printf("\n");
    return create_literal_expr(*p_previous(parser));
}

Expr parse_term(Parser *parser) {
    Expr expr = parse_primary(parser);

    Token *current;
    while ((current = p_peek(parser)) != NULL && !p_isAtEnd(parser)) {
        if (current->type != PLUS && current->type != MINUS) {
            break;
        }
        Token operator = *current;
        Expr right = parse_primary(parser);
        expr = create_binary_expr(operator, &expr, &right);
    }
    return expr;
}
