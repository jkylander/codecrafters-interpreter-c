#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct string_pair_type {
    ExprType type;
    const char *str;
} known_types[] = {
    {ASSIGN, "ASSIGN"},
    {BINARY, "BINARY"},
    {CALL, "CALL"},
    {GET, "GET"},
    {GROUPING, "GROUPING"},
    {LITERAL, "LITERAL"},
    {LOGICAL, "LOGICAL"},
    {SET, "SET"},
    {SUPER, "SUPER"},
    {THIS, "THIS"},
    {UNARY, "UNARY"},
    {VARIABLE, "VARIABLE"},
};

Expr *create_binary_expr(Token binary_op, Expr *left, Expr *right) {
    Expr *expr = malloc(sizeof(Expr));
    expr->type = BINARY;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    expr->as.binary.operator = binary_op;
    expr->line = left->line;
    return expr;
}

Expr *create_literal_expr(Token token) {
    Expr *expr = malloc(sizeof(Expr));
    expr->type = LITERAL;
    Object *v = malloc(sizeof(Object));

    if (token.type == TOKEN_NUMBER) {
        v->type = VAL_NUMBER;
        v->as.number = strtod(token.start, NULL);
    } else if (token.type == TOKEN_NIL) {
        v->type = VAL_NIL;
        v->as.number = 0;
    } else if (token.type == TOKEN_TRUE || token.type == TOKEN_FALSE) {
        v->type = VAL_BOOL;
        v->as.boolean = token.type == TOKEN_TRUE ? true : false ;
    } else {
        v->type = VAL_STRING;
        int len = token.end - token.start;
        char *str = malloc(len + 1);
        strncpy(str, token.start, len);
        str[len] = '\0';
        v->as.string = str;
    }

    expr->as.literal.value = v;
    expr->line = token.line;
    return expr;
}

Expr *create_grouping_expr(Expr *expression) {
    Expr *expr = malloc(sizeof(Expr));
    expr->type = GROUPING;
    expr->as.grouping.expression = expression;
    expr->line = expression->line;
    return expr;
}

void free_expr(Expr *expr) {
    if (!expr) return;
    switch (expr->type) {
        case BINARY:
            free_expr(expr->as.binary.left);
            free_expr(expr->as.binary.right);
        break;
        case GROUPING:
            free_expr(expr->as.grouping.expression);
        break;
        case UNARY:
            free_expr(expr->as.unary.right);
        break;
        case LITERAL: break;
        default: break;
    }
}

const char *str_from_type(ExprType type) {
    return known_types[type].str;
}
void print_ast(Expr *expr) {
    if (expr != NULL) {
        if (expr->type == LITERAL) {
            if (expr->as.literal.value->type == VAL_STRING) {
                printf("%s", expr->as.literal.value->as.string);
            } else if (expr->as.literal.value->type == VAL_BOOL) {
                printf("%s", expr->as.literal.value->as.boolean == 1 ? "true" : "false");

            } else if (expr->as.literal.value->type == VAL_NIL) {
                printf("nil");

            } else if (expr->as.literal.value->type == VAL_NUMBER) {
                double value = expr->as.literal.value->as.number;
                if (value == (int) value) {
                    printf("%.1f", value);
                } else {
                    printf("%g", value);
                }
            } else {
                printf("%s", expr->as.literal.value->as.string);
            }
        } else if (expr->type == BINARY) {
            printf("(%.*s ", expr->as.binary.operator.length, expr->as.binary.operator.start);
            print_ast(expr->as.binary.left);
            printf(" ");
            print_ast(expr->as.binary.right);
            printf(")");
        } else if (expr->type == GROUPING) {
            printf("(group ");
            print_ast(expr->as.grouping.expression);
            printf(")");
        } else if (expr->type == UNARY) {
            printf("(%.*s ", expr->as.unary.operator.length, expr->as.unary.operator.start);
            print_ast(expr->as.unary.right);
            printf(")");
        }
    }
}
