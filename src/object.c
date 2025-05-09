#include <stdio.h>
#include <string.h>

#include "chunk.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

const char *const ObjType_String[] = {
#define X(e) #e,
    OBJ_TYPE_ENUM
#undef X
};

#define ALLOCATE_OBJ(type, objectType)                                         \
    (type *) allocateObject(sizeof(type), objectType)

static Obj *allocateObject(size_t size, ObjType type) {
    Obj *object = (Obj *) reallocate(nullptr, 0, size);
    object->type = type;
    object->isMarked = false;

    object->next = vm.objects;
    vm.objects = object;

#ifdef DEBUG_LOG_GC
    fprintf(fout, "%p allocate %zu for %s\n", (void *) object, size,
           ObjType_String[type]);
#endif

    return object;
}

ObjBoundMethod *newBoundMethod(Value receiver, ObjClosure *method) {
    ObjBoundMethod *bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

ObjClass *newClass(ObjString *name) {
    ObjClass *class = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    class->name = name;
    initTable(&class->methods);
    return class;
}

ObjClosure *newClosure(ObjFunction *function) {
    ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue *, function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = nullptr;
    }
    ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

static ObjString *allocateString(char *chars, int length, uint32_t hash) {
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    push(OBJ_VAL(string));
    tableSet(&vm.strings, string, NIL_VAL);
    pop();

    return string;
}

static uint32_t hashString(const char *key, int length) {
    // FNV-1a
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t) key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString *copyString(const char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != nullptr)
        return interned;
    char *heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

ObjUpvalue *newUpvalue(Value *slot) {
    ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = nullptr;
    return upvalue;
}

static void printFunction(FILE *fout, ObjFunction *function) {
    if (function->name == nullptr) {
        fprintf(fout, "<script>");
        return;
    }
    fprintf(fout, "<fn %s>", function->name->chars);
}

void printObject(FILE *fout, Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_BOUND_METHOD:
            printFunction(fout, AS_BOUND_METHOD(value)->method->function);
            break;
        case OBJ_CLASS: fprintf(fout, "%s", AS_CLASS(value)->name->chars); break;
        case OBJ_CLOSURE: printFunction(fout, AS_CLOSURE(value)->function); break;
        case OBJ_UPVALUE: fprintf(fout, "upvalue"); break;
        case OBJ_FUNCTION: printFunction(fout, AS_FUNCTION(value)); break;
        case OBJ_INSTANCE:
            fprintf(fout, "%s instance", AS_INSTANCE(value)->class->name->chars);
            break;
        case OBJ_LIST: {
            ObjList *list = AS_LIST(value);
            fputc('[', fout);
            if (list->elements.count > 0) {
                printValueC(fout, list->elements.values[0]);
            }
            for (int i = 1; i < list->elements.count; i++) {
                fprintf(fout, ", ");
                printValueC(fout, list->elements.values[i]);
            }
            fputc(']', fout);
        }
        case OBJ_MAP: {
            ObjMap *map = AS_MAP(value);
            fputc('{', fout);
            bool first = true;
            for (int i = 0; i < map->table.capacity; ++i) {
                Entry *entry = &map->table.entries[i];
                if (entry->key == nullptr) {
                    continue;
                }
                if (first) {
                    first = false;
                } else {
                    fprintf(fout, ", ");
                }
                fprintf(fout, "%.*s: ", entry->key->length, entry->key->chars);
                printValueC(fout, entry->value);
            }
            fputc('}', fout);
            break;
        }
        case OBJ_NATIVE: fprintf(fout, "<native fn>"); break;
        case OBJ_STRING: fprintf(fout, "%s", AS_CSTRING(value)); break;
    }
}

ObjMap *newMap() {
    ObjMap *map = ALLOCATE_OBJ(ObjMap, OBJ_MAP);
    initTable(&map->table);
    return map;
}

ObjList *newList() {
    ObjList *list = ALLOCATE_OBJ(ObjList, OBJ_LIST);
    initValueArray(&list->elements);
    return list;
}

ObjFunction *newFunction() {
    ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = nullptr;
    initChunk(&function->chunk);
    return function;
}

ObjInstance *newInstance(ObjClass *class) {
    ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->class = class;
    initTable(&instance->fields);
    return instance;
}

ObjNative *newNative(NativeFn function) {
    ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

ObjString *takeString(char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.strings, chars, length, hash);
    if (interned != nullptr) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    return allocateString(chars, length, hash);
}
