#ifndef INTERPRETER_H
#define INTERPRETER_H
#include "ast.h"

Object visit_literal(Expr *expr);
Object visit_grouping(Expr *expr);
Object visit_binary(Expr *expr);
Object visit_unary(Expr *expr);
Object evaluate(Expr *expr);
void print_value(Object *value);

#endif /* INTERPRETER_H */
