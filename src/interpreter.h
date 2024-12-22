#ifndef INTERPRETER_H
#define INTERPRETER_H
#include "ast.h"
#include "stmt.h"

Object visit_literal(Expr *expr);
Object visit_grouping(Expr *expr);
Object visit_binary(Expr *expr);
Object visit_unary(Expr *expr);
Object evaluate(Expr *expr);
void print_object(Object *value);
void print_statements(StmtArray *array);

#endif /* INTERPRETER_H */
