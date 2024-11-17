#include "ast.h"
#include <stdlib.h>

Expr *create_binary_expr(Token binary_op, Expr *left, Expr *right) {
    Expr *expr = malloc(sizeof(Expr));
    expr->type = BINARY;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    expr->as.binary.binary_op = binary_op;
    return expr;
}
