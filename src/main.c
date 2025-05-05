#include "compiler.h"
#include "scanner.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *read_file_contents(const char *filename);

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
        if (hadError()) {
            return 65;
        }
        print_ast(ast);
        printf("\n");
        free_expr(ast);
        free(source);

    } else if (strcmp(command, "evaluate") == 0) {
        char *source = read_file_contents(argv[2]);
        InterpretResult result = evaluate(source);
        free(source);
        if (result == INTERPRET_COMPILE_ERROR)
            exit(65);
        if (result == INTERPRET_RUNTIME_ERROR)
            exit(70);

    } else if (strcmp(command, "run") == 0) {
        char *source = read_file_contents(argv[2]);
        InterpretResult result = interpret(source);
        free(source);
        if (result == INTERPRET_COMPILE_ERROR)
            exit(65);
        if (result == INTERPRET_RUNTIME_ERROR)
            exit(70);
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    freeVM();
    return 0;
}

char *read_file_contents(const char *filename) {
    FILE *f = fopen(filename, "rb");

    if (f == nullptr) {
        fprintf(stderr, "Could not open file: %s\n", filename);
        exit(74);
    }
    if (fseek(f, 0, SEEK_END) < 0)
        return nullptr;
    long fsize = ftell(f);
    if (fsize < 0)
        return nullptr;
    rewind(f);
    char *s = malloc(fsize + 1);
    if (s == nullptr)
        return nullptr;
    unsigned long read_size = fread(s, 1, fsize, f);
    if (read_size != (unsigned long) fsize)
        return nullptr;
    s[fsize] = '\0';

    if (f != stdin) {
        int fc = fclose(f);
        if (fc != 0) {
            perror("File close failed\n.");
            return nullptr;
        }
    }
    return s;
}
