#include "ast.h"
#include "scanner.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>

Parser create_parser(TokenArray *array) {
    Parser parser;
    parser.array= array;
    parser.current = 0;
    parser.hadError = false;
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
    return current == NULL || current->type == TOKEN_EOF;
}

Token *p_advance(Parser *parser) {
    if (!p_isAtEnd(parser)) {
        parser->current++;
        return p_previous(parser);
    }
    return NULL;
}

bool p_match(Parser *parser, TokenType type) {
    Token *current = p_peek(parser);
    if ((current != NULL) && current->type == type) {
        p_advance(parser);
        return true;
    }
    return false;
}


bool check(Parser *parser, TokenType type) {
    if (p_isAtEnd(parser)) return false;
    return p_peek(parser)->type == type;
}

Expr *p_unary(Parser *parser) {
    if (p_match(parser, TOKEN_BANG) || p_match(parser, TOKEN_MINUS)) {
        Token *operator = p_previous(parser);
        Expr *right = p_unary(parser);
        Expr *r = malloc(sizeof(Expr));
        r->type = UNARY;
        r->as.unary.right = right;
        r->as.unary.operator = *operator;
        r->line = right->line;
        return r;
    }
    return p_primary(parser);
}

Expr *p_comparison(Parser *parser) {
    Expr *expr = p_term(parser);
    while (p_match(parser, TOKEN_GREATER) || p_match(parser, TOKEN_GREATER_EQUAL) ||
            p_match(parser, TOKEN_LESS) || p_match(parser, TOKEN_LESS_EQUAL)) {
        Token *operator = p_previous(parser);
        Expr *right = p_term(parser);
        expr = create_binary_expr(*operator, expr, right);
    }
    return expr;

}

Expr *p_equality(Parser *parser) {
    Expr *expr = p_comparison(parser);
    while (p_match(parser, TOKEN_BANG_EQUAL) || p_match(parser, TOKEN_EQUAL_EQUAL)) {
        Token *operator = p_previous(parser);
        Expr *right = p_comparison(parser);
        expr = create_binary_expr(*operator, expr, right);
    }
    return expr;
}

Expr *p_factor(Parser *parser) {
    Expr *expr = p_unary(parser);
    while (p_match(parser, TOKEN_SLASH) || p_match(parser, TOKEN_STAR)) {
        Token *operator = p_previous(parser);
        Expr *right = p_unary(parser);
        expr = create_binary_expr(*operator, expr, right);
    }
    return expr;
}

Expr *p_primary(Parser *parser) {
    Token *current = p_peek(parser);
    if (current == NULL || current->type == TOKEN_EOF) {
        fprintf(stderr, "Error: Unexpected end of input.\n");
        exit(65);
    }

    if (p_match(parser, TOKEN_TRUE) || p_match(parser, TOKEN_FALSE) ||
        p_match(parser, TOKEN_NIL)) {
        return create_literal_expr(*p_previous(parser));
    }
    if (p_match(parser, TOKEN_NUMBER) ||
        p_match(parser, TOKEN_STRING)) {
        return create_literal_expr(*p_previous(parser));
    }

    if (p_match(parser, TOKEN_LEFT_PAREN)) {
        Expr *expr = parse_expression(parser);
        consume(parser, TOKEN_RIGHT_PAREN, "Expect ') after expression\n");
        return create_grouping_expr(expr);
    }
    errorAtCurrent(parser, "Expect expression.\n");
    return NULL;

    /*fprintf(stderr, "[line %d] Error at '%.*s': Expect expression.\n", current->line, current->length, current->start);*/
    /*exit(65);*/
}

void errorAt(Parser *parser, const char *message) {
    Token *token = p_peek(parser);
    fprintf(stderr, "[line %d] Error", token->line);
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // nothing
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, "; %s\n", message);
    exit(65);
    /*parser->hadError = true;*/
}

void errorAtCurrent(Parser *parser, const char *message) {
    errorAt(parser, message);
}

Expr *parse_expression(Parser *parser) {
    return p_equality(parser);
}

void synchronize(Parser *parser) {
    p_advance(parser);
    while (!p_isAtEnd(parser)) {
        if (p_previous(parser)->type == TOKEN_SEMICOLON) return;
        switch (p_peek(parser)->type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
            return;
            default: break;
        }
        p_advance(parser);
    }
}

Token *consume(Parser *parser, TokenType type, const char *message) {
    if (check(parser, type)) return p_advance(parser);
    errorAtCurrent(parser, message);
    return NULL;
}

Expr *p_term(Parser *parser) {
    Expr *expr = p_factor(parser);
    while (p_match(parser, TOKEN_MINUS) || p_match(parser, TOKEN_PLUS)) {
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
