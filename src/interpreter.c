#include "interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
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

void runtime_error(const char *message, int line) {
    fprintf(stderr, "Runtime error: %s\n[line %d]\n", message, line);
    exit(70);
}

Value visit_binary(Expr *expr) {
    TokenType operatorType = expr->as.binary.binary_op.type;
    Value right = evaluate(expr->as.binary.right);
    Value left = evaluate(expr->as.binary.left);

    // Arithmetic
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
                    runtime_error("Division by zero.", expr->line);
                }
                result.as.number = left.as.number / right.as.number;
                return result;
            default: break;
        }
    }

    // Comparison
    if (operatorType == TOKEN_EQUAL_EQUAL || operatorType == TOKEN_BANG_EQUAL) {
        bool is_equal;
        if (left.type != right.type) is_equal = false;
        else {
            switch(left.type) {
                case VAL_NUMBER:
                    is_equal = left.as.number == right.as.number;
                    break;
                case VAL_BOOL:
                    is_equal = left.as.boolean == right.as.boolean;
                    break;
                case VAL_STRING:
                    is_equal = strcmp(left.as.string, right.as.string) == 0;
                    break;
                case VAL_NIL:
                    is_equal = true; // nil == nil
                    break;
                default:
                    is_equal = false;
                    break;
            }
        }
        Value result = {.type = VAL_BOOL};
        result.as.boolean = operatorType == TOKEN_EQUAL_EQUAL ? is_equal : !is_equal;
        return result;
    }

    // Number Comparison
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

            default: break;
        }
    }

    // String concatenation
    if (left.type == VAL_STRING && right.type == VAL_STRING) {
        switch(operatorType) {
            case TOKEN_PLUS: {
                Value result = {.type = VAL_STRING};
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

    // String/number comparisons
    if (left.type == VAL_STRING && right.type == VAL_NUMBER ||
        left.type == VAL_NUMBER && right.type == VAL_STRING ) {
        runtime_error("Operands must be numbers.", expr->line);

    }

    runtime_error("Operands must be two numbers or two strings.", expr->line);
    return (Value){.type = VAL_NIL};

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
            } else {
                runtime_error("Operand must be a number.", expr->line);
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

