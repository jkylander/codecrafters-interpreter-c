#ifndef OBJECT_H
#define OBJECT_H

#include "chunk.h"
#include "common.h"
#include "table.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_BOUND_METHOD(value) isObjType((value), OBJ_BOUND_METHOD)
#define IS_CLASS(value) isObjType((value), OBJ_CLASS)
#define IS_CLOSURE(value) isObjType((value), OBJ_CLOSURE)
#define IS_FUNCTION(value) isObjType((value), OBJ_FUNCTION)
#define IS_INSTANCE(value) isObjType((value), OBJ_INSTANCE)
#define IS_LIST(value) isObjType((value), OBJ_LIST)
#define IS_MAP(value) isObjType((value), OBJ_MAP)
#define IS_NATIVE(value) isObjType((value), OBJ_NATIVE)
#define IS_STRING(value) isObjType((value), OBJ_STRING)

#define AS_BOUND_METHOD(value) ((ObjBoundMethod *) AS_OBJ(value))
#define AS_CLASS(value) ((ObjClass *) AS_OBJ(value))
#define AS_CLOSURE(value) ((ObjClosure *) AS_OBJ(value))
#define AS_FUNCTION(value) ((ObjFunction *) AS_OBJ(value))
#define AS_LIST(value) ((ObjList *) AS_OBJ(value))
#define AS_INSTANCE(value) ((ObjInstance *) AS_OBJ(value))
#define AS_MAP(value) ((ObjMap *) AS_OBJ(value))
#define AS_NATIVE(value) (((ObjNative *) AS_OBJ(value))->function)
#define AS_STRING(value) ((ObjString *) AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *) AS_OBJ(value))->chars)

#define OBJ_TYPE_ENUM                                                          \
    X(OBJ_BOUND_METHOD)                                                        \
    X(OBJ_CLASS)                                                               \
    X(OBJ_CLOSURE)                                                             \
    X(OBJ_UPVALUE)                                                             \
    X(OBJ_FUNCTION)                                                            \
    X(OBJ_INSTANCE)                                                            \
    X(OBJ_LIST)                                                                \
    X(OBJ_MAP)                                                                 \
    X(OBJ_NATIVE)                                                              \
    X(OBJ_STRING)

typedef enum {
#define X(e) e,
    OBJ_TYPE_ENUM
#undef X
} ObjType;

struct Obj {
    ObjType type;
    bool isMarked;
    struct Obj *next;
};

typedef struct {
    Obj obj;
    int arity;
    int upvalueCount;
    Chunk chunk;
    ObjString *name;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, const Value *args);

typedef struct {
    Obj obj;
    NativeFn function;
} ObjNative;

struct ObjString {
    Obj obj;
    int length;
    char *chars;
    uint32_t hash;
};

typedef struct ObjUpvalue {
    Obj obj;
    Value *location;
    Value closed;
    struct ObjUpvalue *next;
} ObjUpvalue;

typedef struct {
    Obj obj;
    ObjFunction *function;
    ObjUpvalue **upvalues;
    int upvalueCount;
} ObjClosure;

typedef struct {
    Obj obj;
    ObjString *name;
    Table methods;
} ObjClass;

typedef struct {
    Obj obj;
    ObjClass *class;
    Table fields;
} ObjInstance;

typedef struct {
    Obj obj;
    Value receiver;
    ObjClosure *method;
} ObjBoundMethod;

typedef struct {
    Obj obj;
    ValueArray elements;
} ObjList;

typedef struct {
    Obj obj;
    Table table;
} ObjMap;

ObjList *newList();
ObjMap *newMap();
ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method);
ObjInstance *newInstance(ObjClass *class);
ObjClass *newClass(ObjString *name);
ObjClosure *newClosure(ObjFunction *function);
ObjFunction *newFunction();
ObjNative *newNative(NativeFn function);
ObjString *copyString(const char *chars, int length);
ObjUpvalue *newUpvalue(Value *slot);
ObjString *takeString(char *chars, int length);
void printObject(FILE *fout, Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

extern const char *const ObjType_String[];

#endif /* OBJECT_H */
