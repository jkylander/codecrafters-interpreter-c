#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *read_file_contents(const char *filename);

int main(int argc, char *argv[]) {
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (argc < 3) {
        fprintf(stderr, "Usage: ./your_program tokenize <filename>\n");
        return 1;
    }

    const char *command = argv[1];

    if (strcmp(command, "tokenize") == 0) {
        // You can use print statements as follows for debugging, they'll be visible when running tests.
        fprintf(stderr, "Logs from your program will appear here!\n");

        char *file_contents = read_file_contents(argv[2]);

        // Uncomment this block to pass the first stage
        if (strlen(file_contents) > 0) {
            fprintf(stderr, "Scanner not implemented\n");
            exit(1);
        }
        printf("EOF  null\n"); // Placeholder, remove this line when implementing the scanner

        free(file_contents);
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

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
