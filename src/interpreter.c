#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scanner.h"

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
    Value right = evaluate(expr->as.binary.right);
    Value left = evaluate(expr->as.binary.left);

    // Number operations
    if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
        Value result = {.type = VAL_NUMBER};
        switch(operatorType) {
            case TOKEN_PLUS:
                result.as.number = left.as.number + right.as.number;
                return result;

            case TOKEN_MINUS:
                result.as.number = left.as.number - right.as.number;
                return result;

            case TOKEN_STAR:
                result.as.number = left.as.number * right.as.number;
                return result;

            case TOKEN_SLASH:
                if (right.as.number == 0) {
                    fprintf(stderr, "Division by zero");
                    exit(65);
                }
                result.as.number = left.as.number / right.as.number;
                return result;

            default: break;
        }
    }
    // Comparison operations
    if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
        Value result = {.type = VAL_BOOL };
        switch(operatorType) {
            case TOKEN_GREATER:
                result.as.boolean= left.as.number > right.as.number;
                return result;

            case TOKEN_GREATER_EQUAL:
                result.as.boolean = left.as.number >= right.as.number;
                return result;

            case TOKEN_LESS:
                result.as.boolean = left.as.number < right.as.number;
                return result;

            case TOKEN_LESS_EQUAL:
                result.as.boolean = left.as.number <= right.as.number;
                return result;

            case TOKEN_EQUAL_EQUAL:
                result.as.boolean = left.as.number == right.as.number;
                return result;

            case TOKEN_BANG_EQUAL:
                result.as.boolean = left.as.number != right.as.number;
                return result;

            default: break;
        }
    }

    // String concatenation
    if (left.type == VAL_STRING && right.type == VAL_STRING) {
        Value result = {.type = VAL_STRING};
        switch(operatorType) {
            case TOKEN_PLUS: {
                size_t left_len = strlen(left.as.string);
                size_t right_len = strlen(right.as.string);
                result.as.string = malloc(left_len + right_len + 1);
                strcpy(result.as.string, left.as.string);
                strcat(result.as.string, right.as.string);
                return result;
            }
            default: break;
        }
    }
    return (Value) {.type = VAL_NIL};
}

bool is_truthy(Value value) {
    switch(value.type) {
        case VAL_NIL:
            return false;
        case VAL_BOOL:
            return value.as.boolean;
        case VAL_NUMBER:
            if (value.as.number == 0) return false;
            return true;
        case VAL_STRING:
            return true;
        default: return false;
    }
}

Value visit_unary(Expr *expr) {
    Value operand = evaluate(expr->as.unary.right);
    switch(expr->as.unary.unary_op.type) {
        case TOKEN_MINUS:
            if (operand.type == VAL_NUMBER) {
                Value result = {.type = VAL_NUMBER, .as.number = -operand.as.number};
                return result;
            }
            break;
        case TOKEN_BANG:
            Value result = {.type = VAL_BOOL, .as.boolean = !is_truthy(operand)};
            return result;
        default: 
            break;
    }
    return (Value){.type = VAL_NIL};
}

Value evaluate(Expr *expr) {
    if (!expr) {
        Value nil_value = {.type = VAL_NIL };
        return nil_value;
    }
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

