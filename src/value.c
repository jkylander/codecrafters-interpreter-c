#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"

void initValueArray(ValueArray *array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

static void ensureNewSpace(ValueArray *array) {
    if (array->capacity < array->count + 1) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->values =
            GROW_ARRAY(Value, array->values, oldCapacity, array->capacity);
    }
}

void writeValueArray(ValueArray *array, Value value) {
    ensureNewSpace(array);

    array->values[array->count] = value;
    array->count++;
}

void insertValueArray(ValueArray *array, int pos, Value value) {
    assert(pos >= 0 && pos < array->count);
    ensureNewSpace(array);
    memmove(array->values + pos + 1, array->values + pos,
            (array->count - pos) * sizeof(Value));
    array->values[pos] = value;
    array->count++;
}

Value removeValueArray(ValueArray *array, int pos) {
    assert(pos >= 0 && pos < array->count);
    Value value = array->values[pos];
    memmove(array->values + pos, array->values + pos + 1,
            (array->count - pos - 1) * sizeof(Value));
    array->count--;
    return value;
}

void printValueC(FILE *fout, Value value) {
    if (IS_LIST(value)) {
        fprintf(fout, "<list %u>", AS_LIST(value)->elements.count);
    } else if (IS_MAP(value)) {
        fprintf(fout, "<map>");
    } else {
        printValue(fout, value);
    }
}

void printValue(FILE *fout, Value value) {
#ifdef NAN_BOXING
    if (IS_BOOL(value)) {
        fprintf(fout, AS_BOOL(value) ? "true" : "false");
    } else if (IS_NIL(value)) {
        fprintf(fout, "nil");
    } else if (IS_NUMBER(value)) {
        fprintf(fout, "%g", AS_NUMBER(value));
    } else if (IS_OBJ(value)) {
        printObject(fout, value);
    }

#else
    switch (value.type) {
        case VAL_BOOL: fprintf(fout, AS_BOOL(value) ? "true" : "false"); break;
        case VAL_NIL: fprintf(fout, "nil"); break;
        case VAL_NUMBER: {
            double num = AS_NUMBER(value);
            if (num == (int) num) {
                fprintf(fout, "%d", (int) num);
            } else {
                fprintf(fout, "%g", num);
            }
            break;
        }
        case VAL_OBJ: printObject(value); break;
    }
#endif
}

void freeValueArray(ValueArray *array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

bool valuesEqual(Value a, Value b) {
    #ifdef NAN_BOXING

    // IEEE 754: NaN != NaN
#if 0
    if (IS_NUMBER(a) && IS_NUMBER(b)) {
        return AS_NUMBER(a) == AS_NUMBER(b);
    }
#endif
    return a == b;

    #else
    if (a.type != b.type)
        return false;
    switch (a.type) {
        case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL: return true;
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ: return AS_OBJ(a) == AS_OBJ(b);
        default: return false;
    }
    #endif
}
