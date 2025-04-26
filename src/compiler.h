#ifndef COMPILER_H
#define COMPILER_H

#include "chunk.h"
bool compile(const char *source, Chunk *chunk);

typedef struct Expr Expr;
void print_ast(Expr *expr);
Expr *parse(const char *source);
bool hadError();

#endif /* COMPILER_H */
