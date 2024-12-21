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
    ASSIGN,
    BINARY,
    CALL,
    GET,
    GROUPING,
    LITERAL,
    LOGICAL,
    SET,
    SUPER,
    THIS,
    UNARY,
    VARIABLE,

    EXPR_COUNT
} ExprType;

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_STRING,
} ObjectType;

typedef struct {
    ObjectType type;
    union {
        bool boolean;
        double number;
        char *string;
    } as;
} Object;


typedef struct Expr Expr;
struct Expr {
    ExprType type;
    int line;
    union {
        struct {
            Token name;
            Expr *value;
        } assign;
        struct {
            Token operator;
            Expr *left;
            Expr *right;
        } binary;
        struct {
            Expr *callee;
            Token paren;
            Expr **arguments;
        } call;
        struct {
            Expr *object;
            Token name;
        } get;
        struct {
            Expr *expression;
        } grouping;
        struct {
            Object *value;
        } literal;
        struct {
            Expr *left;
            Token operator;
            Expr *right;
        } logical;
        struct {
            Expr *object;
            Token name;
            Expr *value;
        } set;
        struct {
            Token keyword;
            Token method;
        } super;
        struct {
            Token keyword;
        } this;
        struct {
            Token operator;
            Expr *right;
        } unary;
        struct {
            Token name;
        } variable;
    } as;
};

Expr *create_literal_expr(Token value);
Expr *create_grouping_expr(Expr *expression);
Expr *create_binary_expr(Token binary_op, Expr *left, Expr *right);
const char *str_from_type(ExprType type);
void print_ast(Expr *expr);
#endif /* AST_H */
