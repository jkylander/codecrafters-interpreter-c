#include <stdlib.h>
#include "ast.h"
#include "scanner.h"

Token visit_literal(Expr *expr);
Token visit_grouping(Expr *expr);
Token visit_binary(Expr *expr);
Token visit_unary(Expr *expr);
Token evaluate(Expr *expr);

Token visit_literal(Expr *expr) {
    return expr->as.literal.value;
}
Token visit_grouping(Expr *expr) {
    return evaluate(expr->as.grouping.expression);
}

Token visit_binary(Expr *expr) {

}

Token visit_unary(Expr *expr) {
    Token right = evaluate(expr->as.unary.right);
    switch(expr->as.unary.unary_op.type) {
        case TOKEN_MINUS:
            /**(double *)right.literal = -*(double *)right.literal;*/
            return right;
            break;
        case TOKEN_BANG:
            break;
        default: 
            /*fprintf(stderr, "[line %d] Error: Unexpected type %s. Token %s\n", expr->line, str_from_type(expr->type), str_from_token(expr->as.unary.unary_op.type));*/
            break;
    }

}

Token evaluate(Expr *expr) {
    switch (expr->type) {
        case LITERAL:
            return visit_literal(expr);
        case BINARY:
            return visit_binary(expr);
        case UNARY:
            return visit_unary(expr);
        case GROUPING:
            return evaluate(expr);
        default:
            /*fprintf(stderr, "[line %d] Error: Unexpected type %s.\n", expr->line, str_from_type(expr->type));*/
            exit(65);
            break;
    }
}

