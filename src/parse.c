#include "ast.h"
#include "token.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
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

Expr *p_unary(Parser *parser) {
    if (p_match(parser, BANG) || p_match(parser, MINUS)) {
        Token *operator = p_previous(parser);
        Expr *right = p_unary(parser);
        Expr *r = malloc(sizeof(Expr));
        r->type = UNARY;
        r->as.unary.right = right;
        r->as.unary.unary_op = *operator;
        return r;
    }
    return p_primary(parser);
}

Expr *p_comparison(Parser *parser) {
    Expr *expr = p_term(parser);
    while (p_match(parser, GREATER) || p_match(parser, GREATER_EQUAL) ||
            p_match(parser, LESS) || p_match(parser, LESS_EQUAL)) {
        Token *operator = p_previous(parser);
        Expr *right = p_term(parser);
        expr = create_binary_expr(*operator, expr, right);
    }
    return expr;

}

Expr *p_equality(Parser *parser) {
    Expr *expr = p_comparison(parser);
    while (p_match(parser, BANG_EQUAL) || p_match(parser, EQUAL_EQUAL)) {
        Token *operator = p_previous(parser);
        Expr *right = p_comparison(parser);
        expr->as.binary.left = expr;
        expr->as.binary.right = right;
        expr->as.binary.binary_op = *operator;
    }
    return expr;
}

Expr *p_factor(Parser *parser) {
    Expr *expr = p_unary(parser);
    while (p_match(parser, SLASH) || p_match(parser, STAR)) {
        Token *operator = p_previous(parser);
        Expr *right = p_unary(parser);
        expr = create_binary_expr(*operator, expr, right);
    }
    return expr;
}

Expr *p_primary(Parser *parser) {
    Token *current = p_peek(parser);
    if (current == NULL || current->type == EOF_TOKEN) {
        fprintf(stderr, "Error: Unexpected end of input.\n");
        exit(1);
    }

    if (p_match(parser, TRUE) || p_match(parser, FALSE) ||
        p_match(parser, NIL)) {
        return create_literal_expr(*p_previous(parser));
    }
    if (p_match(parser, NUMBER) ||
        p_match(parser, STRING)) {
        return create_literal_expr(*p_previous(parser));
    }

    if (p_match(parser, LEFT_PAREN)) {
        Expr *expr = parse_expression(parser);
        consume(parser, RIGHT_PAREN, "Expect ') after expression");
        return create_grouping_expr(expr);
    }
    fprintf(stderr, "Error: Unexpected token, got token type %s.\n", str_from_token(p_peek(parser)->type));
    exit(1);
}

Expr *parse_expression(Parser *parser) {
    return p_equality(parser);
}

void synchronize(Parser *parser) {
    p_advance(parser);
    while (!p_isAtEnd(parser)) {
        if (p_previous(parser)->type == SEMICOLON) return;
        switch (p_peek(parser)->type) {
            case CLASS:
            case FUN:
            case VAR:
            case FOR:
            case IF:
            case WHILE:
            case PRINT:
            case RETURN:
            return;
            default: break;
        }
        p_advance(parser);
    }
}

Token *consume(Parser *parser, TokenList type, const char *message) {
    if (check(parser, type)) return p_advance(parser);
    Token *t = p_peek(parser);
    if (t->type == EOF_TOKEN) {
        fprintf(stderr, "%zu at end %s\n",t->line, message);
        exit(1);
    } else {
        fprintf(stderr, "%zu at '%s' %s", t->line, t->lexeme, message);
        exit(1);
    }
}

Expr *p_term(Parser *parser) {
    Expr *expr = p_factor(parser);
    while (p_match(parser, MINUS) || p_match(parser, PLUS)) {
        Token *operator = p_previous(parser);
        Expr *right = p_factor(parser);
        expr = create_binary_expr(*operator, expr, right);
    }
    return expr;
}

Expr *parse (Parser *parser) {
    Expr *p = parse_expression(parser);
    return p ? p : NULL;
}
