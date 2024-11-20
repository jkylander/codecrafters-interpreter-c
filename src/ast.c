#include "ast.h"
#include "token.h"

Expr *create_binary_expr(Token binary_op, Expr *left, Expr *right) {
    Expr *expr = malloc(sizeof(Expr));
    expr->type = BINARY;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    expr->as.binary.binary_op = binary_op;
    return expr;
}

Expr *create_literal_expr(Token value) {
    Expr *expr = malloc(sizeof(Expr));
    expr->type = LITERAL;
    expr->as.literal.value = value;
    return expr;
}

Expr *create_grouping_expr(Expr *expression) {
    Expr *expr = malloc(sizeof(Expr));
    expr->type = GROUPING;
    expr->as.grouping.expression = expression;
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

void print_ast(Expr *expr) {
    if (expr->type == LITERAL) {
        if (expr->as.literal.value.literal != NULL) {
            if (expr->as.literal.value.type == STRING) {
                printf("%s", (char *)expr->as.literal.value.literal);
            } else if (expr->as.literal.value.type == NUMBER) {
                double value = *(double *)expr->as.literal.value.literal;
                if (value == (int) value) {
                    printf("%.1f", value);
                } else {
                    printf("%g", value);
                }
            }
        } else {
            printf("%s", expr->as.literal.value.lexeme);
        }
    } else if (expr->type == BINARY) {
        printf("%s", expr->as.binary.binary_op.lexeme);
        print_ast(expr->as.binary.left);
        printf(" ");
        print_ast(expr->as.binary.right);
    } else if (expr->type == GROUPING) {
        printf("(group ");
        print_ast(expr->as.grouping.expression);
        printf(")");
    }
}
