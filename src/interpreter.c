#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL(value) ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)

Value visit_literal(Expr *expr);
Value visit_grouping(Expr *expr);
Value visit_binary(Expr *expr);
Value visit_unary(Expr *expr);
Value evaluate(Expr *expr);
void print_value(Value *value);

Value visit_literal(Expr *expr) {
    return *expr->as.literal.value;
}
Value visit_grouping(Expr *expr) {
    return evaluate(expr->as.grouping.expression);
}

Value visit_binary(Expr *expr) {
    TokenType operatorType = expr->as.binary.binary_op.type;

}

Value visit_unary(Expr *expr) {
    Value right = evaluate(expr->as.unary.right);
    switch(expr->as.unary.unary_op.type) {
        case TOKEN_MINUS:
            /**(double *)right.literal = -*(double *)right.literal;*/
            right.as.number = -right.as.number;
            break;
        case TOKEN_BANG:
            right.as.number = !right.as.number;
            break;
        default: 
            /*fprintf(stderr, "[line %d] Error: Unexpected type %s. Token %s\n", expr->line, str_from_type(expr->type), str_from_token(expr->as.unary.unary_op.type));*/
            break;
    }
    return right;

}

Value evaluate(Expr *expr) {
    switch (expr->type) {
        case LITERAL:
            return visit_literal(expr);
        case BINARY:
            return visit_binary(expr);
        case UNARY:
            return visit_unary(expr);
        case GROUPING:
            return visit_grouping(expr);
        default:
            /*fprintf(stderr, "[line %d] Error: Unexpected type %s.\n", expr->line, str_from_type(expr->type));*/
            exit(65);
            break;
    }
}

void print_value(Value *value) {
    switch(value->type) {
        case VAL_BOOL:
            printf("%s\n", value->as.boolean == 1 ? "true" : "false");
            break;
        case VAL_NIL:
            printf("nil\n");
            break;
        case VAL_STRING:
            printf("%s\n", value->as.string);
            break;
        case VAL_NUMBER:
            if (value->as.number == (int)value->as.number)
                printf("%d\n", (int)value->as.number);
            else printf("%g\n", value->as.number);
        default: break;
    }
}

