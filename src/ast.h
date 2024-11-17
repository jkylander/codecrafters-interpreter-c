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
#include "token.h"
typedef enum {
    LITERAL,
    UNARY,
    BINARY,
    GROUPING,

    EXPR_COUNT
} ExprType ;

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
            Token value;
        } literal;
    } as;
};

#endif /* AST_H */
