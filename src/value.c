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

void printValueC(Value value) {
    if (IS_LIST(value)) {
        printf("<list %u>", AS_LIST(value)->elements.count);
    } else if (IS_MAP(value)) {
        printf("<map>");
    } else {
        printValue(value);
    }
}

void printValue(Value value) {
    switch (value.type) {
        case VAL_BOOL: printf(AS_BOOL(value) ? "true" : "false"); break;
        case VAL_NIL: printf("nil"); break;
        case VAL_NUMBER: {
            double num = AS_NUMBER(value);
            if (num == (int) num) {
                printf("%d", (int) num);
            } else {
                printf("%g", num);
            }
            break;
        }
        case VAL_OBJ: printObject(value); break;
    }
}

void freeValueArray(ValueArray *array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type)
        return false;
    switch (a.type) {
        case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL: return true;
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ: return AS_OBJ(a) == AS_OBJ(b);
        default: return false;
    }
}
