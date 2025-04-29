#ifndef COMPILER_H
#define COMPILER_H

#include "chunk.h"
#include "object.h"
ObjFunction *compile(const char *source);
ObjFunction *compile_expression(const char *source);

typedef struct Expr Expr;
void print_ast(Expr *expr);
Expr *parse(const char *source);
bool hadError();


#endif /* COMPILER_H */
