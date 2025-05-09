#ifndef VM_H
#define VM_H
#include "chunk.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include <stdint.h>
#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
    ObjClosure *closure;
    ObjFunction *function;
    uint8_t *ip;
    Value *slots;
} CallFrame;

typedef struct {
    FILE *fout;
    FILE *ferr;
    CallFrame frames[FRAMES_MAX];
    int frameCount;
    Value stack[STACK_MAX];
    Value *stackTop;
    Chunk *chunk;
    uint8_t *ip;
    Table strings;
    ObjString *initString;
    Table globals;
    ObjUpvalue *openUpvalues;

    ObjClass *listClass;
    ObjClass *mapClass;
    size_t bytesAllocated;
    size_t nextGC;
    Obj *objects;
    int grayCount;
    int grayCapacity;
    Obj **grayStack;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

void initVM(FILE *fout, FILE *ferr);
void freeVM();
InterpretResult interpret(const char *source);
InterpretResult evaluate(const char *source);
void push(Value value);
Value pop();

#endif /* VM_H */
