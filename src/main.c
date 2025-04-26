#define _POSIX_C_SOURCE 200809L
#include "compiler.h"
#include "scanner.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *read_file_contents(const char *filename);

static void runFile(const char *path) {
    char *source = read_file_contents(path);
    InterpretResult result = interpret(source);
    free(source);
    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, char *argv[]) {
    initVM();
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (argc < 3) {
        fprintf(stderr, "Usage: ./your_program tokenize <filename>\n");
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "tokenize") == 0) {
        char *source = read_file_contents(argv[2]);
        bool hadError = lex(source);
        free(source);
        return hadError ? 65 : 0;
    } else if (strcmp(command, "parse") == 0) {
        char *source = read_file_contents(argv[2]);
        Expr *ast = parse(source);
        print_ast(ast);
        printf("\n");

    } else if (strcmp(command, "evaluate") == 0) {
        runFile(argv[2]);

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
            exit(74);
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
    fclose(out);
    buf[buflen - 1] = '\0';

    return buf;
}
