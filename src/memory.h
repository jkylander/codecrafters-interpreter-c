#ifndef MEMORY_H
#define MEMORY_H
#include "common.h"
#include "value.h"

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_ARRAY(type, pointer, oldCount, newCount)                          \
    (type *) reallocate((pointer), sizeof(type) * (oldCount),                  \
                        sizeof(type) * (newCount))
#define FREE_ARRAY(type, pointer, oldCount)                                    \
    reallocate(pointer, sizeof(type) * oldCount, 0)
#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)
#define ALLOCATE(type, count) (type*) reallocate(nullptr, 0, sizeof(type) * (count))

void *reallocate(void *pointer, size_t oldSize, size_t newSize);
void markObject(Obj* object);
void markValue(Value value);
void collectGarbage();
void freeObjects();

typedef struct {
    char *buf;
    size_t size;
    FILE *fp;
} FileStream;

void initFileStream(FileStream *fs);
void freeFileStream(FileStream *fs);

#endif /* MEMORY_H */
