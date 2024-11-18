#include "ast.h"
#include "token.h"

Expr create_binary_expr(Token binary_op, Expr *left, Expr *right) {
    Expr expr;
    expr.type = BINARY;
    expr.as.binary.left = left;
    expr.as.binary.right = right;
    expr.as.binary.binary_op = binary_op;
    return expr;
}

Expr create_literal_expr(Token value) {
    Expr expr;
    expr.type = LITERAL;
    expr.as.literal.value = value;
    return expr;
}

Expr create_grouping_expr(Expr *expression) {
    Expr expr;
    expr.type = GROUPING;
    expr.as.grouping.expression = expression;
    return expr;
}

void print_ast(Expr *expr) {
    if (expr->type == LITERAL) {
        if (expr->as.literal.value.literal != NULL) {
            if (expr->as.literal.value.type == STRING) {
                printf("%s ", (char *)expr->as.literal.value.literal);
            } else if (expr->as.literal.value.type == NUMBER) {
                double value = *(double *)expr->as.literal.value.literal;
                if (value == (int) value) {
                    printf(" %.1f", value);
                } else {
                    printf(" %g", value);
                }
            }
        } else {
            printf("%s ", expr->as.literal.value.lexeme);
        }
    } else if (expr->type == BINARY) {
        printf("%s", expr->as.binary.binary_op.lexeme);
        #if 0
        for (int i = 0; i < expr->as.binary.binary_op.length; ++i) {
            printf("%c", expr->as.binary.binary_op.lexeme[i]);
        }
        #endif
        print_ast(expr->as.binary.left);
        printf(" ");
        print_ast(expr->as.binary.right);
    }
}
