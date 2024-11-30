#ifndef INTERPRETER_H
#define INTERPRETER_H
#include "ast.h"

Value visit_literal(Expr *expr);
Value visit_grouping(Expr *expr);
Value visit_binary(Expr *expr);
Value visit_unary(Expr *expr);
Value evaluate(Expr *expr);
void print_value(Value *value);

#endif /* INTERPRETER_H */
