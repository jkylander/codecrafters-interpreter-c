#define _POSIX_C_SOURCE 200809L
#include "chunk.h"
#include "debug.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *read_file_contents(const char *filename);

void testChunk() {
    Chunk chunk;
    initChunk(&chunk);
    int constant = addConstant(&chunk, 1.2);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);
    constant = addConstant(&chunk, 3.4);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);
    writeChunk(&chunk, OP_ADD, 123);
    constant = addConstant(&chunk, 5.6);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, constant, 123);
    writeChunk(&chunk, OP_DIVIDE, 123);
    writeChunk(&chunk, OP_NEGATE, 123);
    writeChunk(&chunk, OP_RETURN, 123);
    interpret(&chunk);
    freeChunk(&chunk);
}

int main(int argc, char *argv[]) {
    initVM();
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    testChunk();

    if (argc < 3) {
        fprintf(stderr, "Usage: ./your_program tokenize <filename>\n");
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "tokenize") == 0) {
        char *file_contents = read_file_contents(argv[2]);

        // Uncomment this block to pass the first stage
        if (strlen(file_contents) > 0) {
            fprintf(stderr, "Scanner not implemented\n");
            exit(1);
        }

        free(file_contents);
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    freeVM();
    return 0;
}

char *read_file_contents(const char *filename) {
    FILE *file;
    if (strcmp(filename, "-") == 0) {
        file = stdin;
    } else {
        file = fopen(filename, "rb");
        if (file == nullptr) {
            fprintf(stderr, "Could not open file: %s\n", filename);
            exit(EXIT_FAILURE);
        }
    }
    char *buf;
    size_t buflen;
    FILE *out = open_memstream(&buf, &buflen);
    for (;;) {
        char buf2[4096];
        size_t n = fread(buf2, 1, sizeof(buf2), file);
        if (n == 0) {
            break;
        }
        fwrite(buf2, 1, n, out);
    }

    if (file != stdin) {
        fclose(file);
    }
    fflush(out);
    fputc('\0', out);
    fclose(out);

    return buf;
}
