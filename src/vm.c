#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"
#include "vm.h"

VM vm;

static void nativeError(const char *format, ...);
static void defineNativeMethod(ObjClass *class, const char *name, NativeFn fn);
static void runtimeError(const char *format, ...);

static Value printLoxValue(int argCount, const Value *args) {
    for (int i = 0; i < argCount; i++) {
        printValue(vm.fout, args[i]);
    }
    fprintf(vm.fout, "\n");
    return BOOL_VAL(true);
}

static Value printErrNative(int argCount, const Value *args) {
    if (argCount != 1) {
        nativeError("Expected 1 argument but got %d", argCount);
    }
    if (!IS_STRING(args[0])) {
        nativeError("Expected string argument.");
    }
    fprintf(stderr, "%s\n", AS_CSTRING(args[0]));
    return BOOL_VAL(true);
}

static Value clockNative(int argCount, const Value *args) {
    (void) argCount, (void) args;
    return NUMBER_VAL((double) clock() / CLOCKS_PER_SEC);
}

static Value timeNative(int argCount, const Value *args) {
    (void) argCount, (void) args;
    return NUMBER_VAL((double) time(nullptr));
}
static void resetStack() {
    vm.stackTop = vm.stack;
    vm.frameCount = 0;
    vm.openUpvalues = nullptr;
}

static bool checkIndexBounds(const char *type, int bounds, Value indexValue) {
    if (!IS_NUMBER(indexValue)) {
        runtimeError("%s must be a number.", type);
        return false;
    }
    double indexNum = AS_NUMBER(indexValue);
    if (indexNum < 0 || indexNum >= (double) bounds) {
        runtimeError("%s (%g) out of bounds (%d)", type, indexNum, bounds);
        return false;
    }
    if ((double) (int) indexNum != indexNum) {
        runtimeError("%s (%g) must be a whole number.", type, indexNum);
        return false;
    }
    return true;
}

static bool checkListIndex(Value listValue, Value indexValue) {
    ObjList *list = AS_LIST(listValue);
    return checkIndexBounds("List index", list->elements.count, indexValue);
}

static Value mapCount(int argCount, const Value *args) {
    if (argCount != 0) {
        nativeError("Expected 0 arguments, got %d", argCount);
    }
    ObjMap *map = AS_MAP(args[-1]);
    int count = 0;
    for (int i = 0; i < map->table.capacity; i++) {
        count += !!(map->table.entries[i].key);
    }
    return NUMBER_VAL((double) count);
}

static Value mapHas(int argCount, const Value *args) {
    if (argCount != 1) {
        nativeError("Expected 1 argument, got %d", argCount);
    }
    if (!IS_STRING(args[0])) {
        nativeError("Maps can only be indexed by string.");
    }

    ObjMap *map = AS_MAP(args[-1]);
    ObjString *key = AS_STRING(args[0]);
    Value value;
    return BOOL_VAL(tableGet(&map->table, key, &value));
}

static Value mapRemove(int argCount, const Value *args) {
    if (argCount != 1) {
        nativeError("Expected 1 arguments, got %d", argCount);
    }
    if (!IS_STRING(args[0])) {
        nativeError("Maps can only be indexed by string.");
    }

    ObjMap *map = AS_MAP(args[-1]);
    ObjString *key = AS_STRING(args[0]);

    return BOOL_VAL(tableDelete(&map->table, key));
}

static void initMapClass() {
    const char mapStr[] = "(Map)";
    ObjString *mapClassName = copyString(mapStr, sizeof(mapStr) - 1);
    push(OBJ_VAL(mapClassName));
    vm.mapClass = newClass(mapClassName);
    pop();

    defineNativeMethod(vm.mapClass, "count", mapCount);
    defineNativeMethod(vm.mapClass, "has", mapHas);
    defineNativeMethod(vm.mapClass, "remove", mapRemove);
}

static Value listPop(int argCount, const Value *args) {
    if (argCount != 0) {
        nativeError("Expected 0 arguments, got %d", argCount);
    }
    ObjList *list = AS_LIST(args[-1]);
    if (list->elements.count == 0) {
        runtimeError("Can't pop form empty list.");
        return BOOL_VAL(false);
    }
    return removeValueArray(&list->elements, list->elements.count - 1);
}

static Value listPush(int argCount, const Value *args) {
    if (argCount != 1) {
        nativeError("Expected 1 arguments, got %d", argCount);
    }
    ObjList *list = AS_LIST(args[-1]);
    writeValueArray(&list->elements, args[0]);
    return NIL_VAL;
}

static Value listInsert(int argCount, const Value *args) {
    if (argCount != 2) {
        nativeError("expected 2 arguments, got %d", argCount);
    }
    if (!checkListIndex(args[-1], args[0])) {
        nativeError("Invalid list.");
    }

    ObjList *list = AS_LIST(args[-1]);
    int pos = (int) AS_NUMBER(args[0]);
    insertValueArray(&list->elements, pos, args[1]);

    return NIL_VAL;
}

static Value listSize(int argCount, const Value *args) {
    if (argCount != 0) {
        nativeError("Expected 0 arguments, got %d", argCount);
    }
    ObjList *list = AS_LIST(args[-1]);
    return NUMBER_VAL((double) list->elements.count);
}

static Value listRemove(int argCount, const Value *args) {
    if (argCount != 1) {
        nativeError("Expected 1 arguments, got %d", argCount);
    }
    if (!checkListIndex(args[-1], args[0])) {
        nativeError("Invalid list.");
    }
    ObjList *list = AS_LIST(args[-1]);
    int pos = (int) AS_NUMBER(args[0]);
    return removeValueArray(&list->elements, pos);
}

static void initListClass() {
    const char listStr[] = "(List)";
    ObjString *listClassName = copyString(listStr, sizeof(listStr) - 1);
    push(OBJ_VAL(listClassName));
    vm.listClass = newClass(listClassName);
    pop();

    defineNativeMethod(vm.listClass, "insert", listInsert);
    defineNativeMethod(vm.listClass, "push", listPush);
    defineNativeMethod(vm.listClass, "pop", listPop);
    defineNativeMethod(vm.listClass, "size", listSize);
    defineNativeMethod(vm.listClass, "remove", listRemove);
}

static void nativeError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(vm.ferr, format, args);
    va_end(args);
    fputs("\n", vm.ferr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(vm.ferr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == nullptr) {
            fprintf(vm.ferr, "script\n");
        } else {
            fprintf(vm.ferr, "%s()\n", function->name->chars);
        }
    }

    freeVM();
    exit(70);
}

static void runtimeError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(vm.ferr, format, args);
    va_end(args);
    fputs("\n", vm.ferr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        ObjFunction *function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(vm.ferr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name == nullptr) {
            fprintf(vm.ferr, "script\n");
        } else {
            fprintf(vm.ferr, "%s()\n", function->name->chars);
        }
    }

    resetStack();
}

static void defineNative(const char *name, NativeFn function) {
    push(OBJ_VAL(copyString(name, (int) strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

static void defineNativeMethod(ObjClass *class, const char *name, NativeFn fn) {
    ObjString *str = copyString(name, (int) strlen(name));
    push(OBJ_VAL(str));
    ObjNative *native = newNative(fn);
    push(OBJ_VAL(native));
    tableSet(&class->methods, str, OBJ_VAL(native));
    pop();
    pop();
}

void initVM(FILE *fout, FILE *ferr) {
    resetStack();
    vm.fout = fout;
    vm.ferr = ferr;
    vm.objects = nullptr;
    vm.bytesAllocated = 0;
    vm.nextGC = 1024 * 1024;
    vm.grayCount = 0;
    vm.grayCapacity = 0;
    vm.grayStack = nullptr;
    vm.listClass = nullptr;
    vm.mapClass = nullptr;

    initTable(&vm.globals);
    initTable(&vm.strings);
    vm.initString = nullptr;
    vm.initString = copyString("init", 4);
    initListClass();
    initMapClass();
    defineNative("clock", timeNative);
    defineNative("wallClock", clockNative);
    defineNative("error", printErrNative);
    defineNative("printf", printLoxValue);
}
void freeVM() {
    freeTable(&vm.strings);
    freeTable(&vm.globals);
    vm.initString = nullptr;
    freeObjects();
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static Value peek(int distance) { return vm.stackTop[-1 - distance]; }

static bool call(Obj *callable, int argCount) {
    ObjClosure *closure;
    ObjFunction *function;

    if (callable->type == OBJ_CLOSURE) {
        closure = (ObjClosure *) callable;
        function = closure->function;
    } else {
        assert(callable->type == OBJ_FUNCTION);
        closure = nullptr;
        function = (ObjFunction *) callable;
    }
    if (argCount != function->arity) {
        runtimeError("Expected %d arguments but got %d.", function->arity,
                     argCount);
        return false;
    }
    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }
    CallFrame *frame = &vm.frames[vm.frameCount++];
    frame->closure = closure;
    frame->function = function;
    frame->ip = function->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

static bool callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod *bound = AS_BOUND_METHOD(callee);
                vm.stackTop[-argCount - 1] = bound->receiver;
                return callValue(OBJ_VAL(bound->method), argCount);
            }
            case OBJ_CLASS: {
                ObjClass *class = AS_CLASS(callee);
                vm.stackTop[-argCount - 1] = OBJ_VAL(newInstance(class));
                Value initializer;
                if (tableGet(&class->methods, vm.initString, &initializer)) {
                    return callValue(initializer, argCount);
                } else if (argCount != 0) {
                    runtimeError("Expected 0 arguments but got %d", argCount);
                    return false;
                }
                return true;
            }
            case OBJ_CLOSURE: return call(AS_OBJ(callee), argCount);
            case OBJ_NATIVE: {
                NativeFn native = AS_NATIVE(callee);
                Value result = native(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            }
            default: break;
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

static bool invokeFromClass(ObjClass *class, ObjString *name, int argCount) {
    Value method;
    if (!tableGet(&class->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }
    return callValue(method, argCount);
}

static bool invoke(ObjString *name, int argCount) {
    Value receiver = peek(argCount);
    ObjClass *class;

    if (IS_LIST(receiver)) {
        class = vm.listClass;
    } else if (IS_MAP(receiver)) {
        class = vm.mapClass;
    } else if (IS_INSTANCE(receiver)) {
        ObjInstance *instance = AS_INSTANCE(receiver);
        Value value;
        if (tableGet(&instance->fields, name, &value)) {
            vm.stackTop[-argCount - 1] = value;
            return callValue(value, argCount);
        }
        class = instance->class;
    } else {
        runtimeError("Only lists, maps, and instances have methods.");
        return false;
    }
    return invokeFromClass(class, name, argCount);
}

static bool bindMethod(ObjClass *class, ObjString *name) {
    Value method;
    if (!tableGet(&class->methods, name, &method)) {
        runtimeError("Undefined property '%s'.", name->chars);
        return false;
    }
    ObjBoundMethod *bound = newBoundMethod(peek(0), AS_CLOSURE(method));
    pop();
    push(OBJ_VAL(bound));
    return true;
}

static ObjUpvalue *captureUpvalue(Value *local) {
    ObjUpvalue *prevUpvalue = nullptr;
    ObjUpvalue *upvalue = vm.openUpvalues;
    while (upvalue != nullptr && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    // TODO: pointer-pointer instead of if-conditions?
    if (upvalue != nullptr && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue *createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;
    if (prevUpvalue == nullptr) {
        vm.openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }
    return createdUpvalue;
}

static void closeUpvalues(const Value *last) {
    while (vm.openUpvalues != nullptr && vm.openUpvalues->location >= last) {
        ObjUpvalue *upvalue = vm.openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.openUpvalues = upvalue->next;
    }
}

static void defineMethod(ObjString *name) {
    Value method = peek(0);
    ObjClass *class = AS_CLASS(peek(1));
    tableSet(&class->methods, name, method);
    pop();
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate() {
    ObjString *b = AS_STRING(peek(0));
    ObjString *a = AS_STRING(peek(1));

    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString *result = takeString(chars, length);

    pop();
    pop();
    push(OBJ_VAL(result));
}

static InterpretResult run() {
    CallFrame *frame = &vm.frames[vm.frameCount - 1];
#define READ_BYTE() (*frame->ip++)
#define READ_CONSTANT()                                                        \
    (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_SHORT()                                                           \
    (frame->ip += 2, (uint16_t) ((frame->ip[-2] << 8) | frame->ip[-1]))
#define BINARY_OP(valueType, op)                                               \
    do {                                                                       \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                      \
            runtimeError("Operands must be numbers.");                         \
            return INTERPRET_RUNTIME_ERROR;                                    \
        }                                                                      \
        double b = AS_NUMBER(pop());                                           \
        double a = AS_NUMBER(pop());                                           \
        push(valueType(a op b));                                               \
    } while (0)

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        fprintf(ferr, "         ");
        for (Value *slot = vm.stack; slot < vm.stackTop; slot++) {
            fprintf(ferr, "[ ");
            printValue(*slot);
            printf(" ]");
        }
        fprintf(ferr, "\n");
        disassembleInstruction(
            &frame->closure->function->chunk,
            (int) (frame->ip - frame->closure->function->chunk.code));
#endif
        OpCode instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_POP: pop(); break;
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString *name = READ_STRING();
                Value value;
                if (!tableGet(&vm.globals, name, &value)) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString *name = READ_STRING();
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString *name = READ_STRING();
                if (tableSet(&vm.globals, name, peek(0))) {
                    tableDelete(&vm.globals, name);
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            }
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(0))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance *instance = AS_INSTANCE(peek(0));
                ObjString *name = READ_STRING();

                Value value;
                if (tableGet(&instance->fields, name, &value)) {
                    pop(); // instance
                    push(value);
                    break;
                }
                if (!bindMethod(instance->class, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SET_PROPERTY: {
                if (!IS_INSTANCE(peek(1))) {
                    runtimeError("Only instances have properties.");
                    return INTERPRET_RUNTIME_ERROR;
                }

                ObjInstance *instance = AS_INSTANCE(peek(1));
                tableSet(&instance->fields, READ_STRING(), peek(0));
                Value value = pop();
                pop();
                push(value);
                break;
            }
            case OP_GET_INDEX: {
                if (IS_LIST(peek(1))) {
                    if (!checkListIndex(peek(1), peek(0))) {
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    int index = (int) AS_NUMBER(pop());
                    ObjList *list = AS_LIST(pop());
                    push(list->elements.values[index]);
                    break;
                } else if (IS_MAP(peek(1))) {
                    if (!IS_STRING(peek(0))) {
                        runtimeError("Maps can only be indexed be string.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    ObjString *key = AS_STRING(peek(0));
                    ObjMap *map = AS_MAP(peek(1));
                    Value value;
                    if (tableGet(&map->table, key, &value)) {
                        pop(); // key
                        pop(); // map
                        push(value);
                        break;
                    }
                    runtimeError("Undefined key '%s'.", key->chars);
                } else {
                    runtimeError("Can only index lists or maps.");
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_SET_INDEX: {
                if (IS_LIST(peek(2))) {
                    if (!checkListIndex(peek(2), peek(1))) {
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    Value value = pop();
                    int index = (int) AS_NUMBER(pop());
                    ObjList *list = AS_LIST(pop());
                    list->elements.values[index] = value;
                    push(value);
                    break;
                } else if (IS_MAP(peek(2))) {
                    if (!IS_STRING(peek(1))) {
                        runtimeError("Maps can only be indexed be string.");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    ObjString *key = AS_STRING(peek(1));
                    ObjMap *map = AS_MAP(peek(2));
                    tableSet(&map->table, key, peek(0));
                    Value value = pop();
                    pop(); // key
                    pop(); // map
                    push(value);
                    break;
                } else {
                    runtimeError("Can only set index of lists or maps.");
                }
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_LIST_INIT: {
                push(OBJ_VAL(newList()));
                break;
            }
            case OP_LIST_DATA: {
                if (!IS_LIST(peek(1))) {
                    runtimeError("List data can only be added to a list.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjList *list = AS_LIST(peek(1));
                writeValueArray(&list->elements, peek(0));
                pop();
                break;
            }
            case OP_MAP_INIT: {
                push(OBJ_VAL(newMap()));
                break;
            }
            case OP_MAP_DATA: {
                if (!IS_MAP(peek(2))) {
                    runtimeError("Map data can only be added to a map.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                if (!IS_STRING(peek(1))) {
                    runtimeError("Map key must be a string.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjMap *map = AS_MAP(peek(2));
                ObjString *key = AS_STRING(peek(1));
                tableSet(&map->table, key, peek(0));
                pop(); // Value
                pop(); // Key
                break;
            }
            case OP_GET_SUPER: {
                ObjString *name = READ_STRING();
                ObjClass *superclass = AS_CLASS(pop());

                if (!bindMethod(superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_EQUAL: {
                Value a = pop();
                Value b = pop();
                push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER: BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS: BINARY_OP(BOOL_VAL, <); break;
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    double b = AS_NUMBER(pop());
                    double a = AS_NUMBER(pop());
                    push(NUMBER_VAL(a + b));
                } else {
                    runtimeError(
                        "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
            case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
            case OP_DIVIDE: BINARY_OP(NUMBER_VAL, /); break;
            case OP_NOT: push(BOOL_VAL(isFalsey(pop()))); break;
            case OP_NEGATE: {
                if (!IS_NUMBER(peek(0))) {
                    runtimeError("Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            }
            case OP_PRINT: {
                printValue(vm.fout, pop());
                fprintf(vm.fout, "\n");
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (isFalsey(peek(0)))
                    frame->ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OP_CALL: {
                int argCount = READ_BYTE();
                if (!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_INVOKE: {
                ObjString *method = READ_STRING();
                int argCount = READ_BYTE();
                if (!invoke(method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_SUPER_INVOKE: {
                ObjString *method = READ_STRING();
                int argCount = READ_BYTE();
                ObjClass *superclass = AS_CLASS(pop());
                if (!invokeFromClass(superclass, method, argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_CLOSURE: {
                ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
                ObjClosure *closure = newClosure(function);
                push(OBJ_VAL(closure));
                for (int i = 0; i < closure->upvalueCount; i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues[i] =
                            captureUpvalue(frame->slots + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OP_CLOSE_UPVALUE: {
                closeUpvalues(vm.stackTop - 1);
                pop();
                break;
            }
            case OP_CLASS: {
                push(OBJ_VAL(newClass(READ_STRING())));
                break;
            }
            case OP_INHERIT: {
                Value superclass = peek(1);
                if (!IS_CLASS(superclass)) {
                    runtimeError("Superclass must be a class.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                ObjClass *subclass = AS_CLASS(peek(0));
                tableAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);
                pop();
                break;
            }
            case OP_METHOD: {
                defineMethod(READ_STRING());
                break;
            }
            case OP_RETURN: {
                Value result = pop();
                closeUpvalues(frame->slots);
                vm.frameCount--;
                if (vm.frameCount == 0) {
                    pop();
                    return INTERPRET_OK;
                }
                vm.stackTop = frame->slots;
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef READ_SHORT
#undef BINARY_OP
}

InterpretResult evaluate(const char *source) {
    ObjFunction *function = compile_expression(source);
    if (function == nullptr)
        return INTERPRET_COMPILE_ERROR;
    push(OBJ_VAL(function));
    ObjClosure *closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    call((Obj *) closure, 0);

    return run();
}

InterpretResult interpret(const char *source) {
    ObjFunction *function = compile(source);
    if (function == nullptr)
        return INTERPRET_COMPILE_ERROR;
    push(OBJ_VAL(function));
    ObjClosure *closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    call((Obj *) closure, 0);

    return run();
}
