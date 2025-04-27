#include <stdlib.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
    (void) oldSize;
    if (newSize == 0) {
        free(pointer);
        return nullptr;
    }
    void *result = realloc(pointer, newSize);
    if (result == nullptr)
        exit(1);
    return result;
}

void freeObject(Obj *object) {
    switch (object->type) {
        case OBJ_STRING: {
            ObjString *string = (ObjString*) object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }
    }
}

void freeObjects() {
    Obj *object = vm.objects;
    while (object != nullptr) {
        Obj *next = object->next;
        freeObject(object);
        object = next;
    }
}
