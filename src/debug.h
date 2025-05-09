#ifndef DEBUG_H
#define DEBUG_H
#include "chunk.h"

void disassembleChunk(FILE *ferr, Chunk *chunk, const char *name);
int disassembleInstruction(FILE *ferr, Chunk *chunk, int offset);

#endif /* DEBUG_H */
