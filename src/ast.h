#ifndef AST_H
#define AST_H
/*
expression     → literal
               | unary
               | binary
               | grouping ;

literal        → NUMBER | STRING | "true" | "false" | "nil" ;
grouping       → "(" expression ")" ;
unary          → ( "-" | "!" ) expression ;
binary         → expression operator expression ;
operator       → "==" | "!=" | "<" | "<=" | ">" | ">="
               | "+"  | "-"  | "*" | "/" ;

*/
#include "scanner.h"
typedef enum {
    LITERAL,
    UNARY,
    BINARY,
    GROUPING,

    EXPR_COUNT
} ExprType;

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_STRING,
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        char *string;
    } as;
} Value;


typedef struct Expr Expr;
struct Expr {
    ExprType type;
    int line;
    union {
        struct {
            Token binary_op;
            Expr *left;
            Expr *right;
        } binary;
        struct {
            Token unary_op;
            Expr *right;
        } unary;
        struct {
            Value *value;
        } literal;
        struct {
            Expr *expression;
        } grouping;
    } as;
};

Expr *create_literal_expr(Token value);
Expr *create_grouping_expr(Expr *expression);
Expr *create_binary_expr(Token binary_op, Expr *left, Expr *right);
const char *str_from_type(ExprType type);
void print_ast(Expr *expr);
#endif /* AST_H */
