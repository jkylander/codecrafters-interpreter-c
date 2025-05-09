#ifndef COMPILER_H
#define COMPILER_H

#include "chunk.h"
#include "object.h"
ObjFunction *compile(const char *source);
ObjFunction *compile_expression(const char *source);
void markCompilerRoots();

typedef struct Expr Expr;
void print_ast(FILE *fout, Expr *expr);
void free_expr(Expr *expr);
Expr *parse(const char *source);
bool hadError();


#endif /* COMPILER_H */
