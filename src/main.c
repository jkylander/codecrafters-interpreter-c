#include <stdlib.h>
#include <sysexits.h>
#include "token.h"

int main(int argc, char *argv[]) {
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (argc < 3) {
        fprintf(stderr, "Usage: ./your_program tokenize <filename>\n");
        return EX_USAGE;
    }

    const char *command = argv[1];
    bool error = false;

    if (strcmp(command, "tokenize") == 0) {
        TokenArray tokens = tokenize(argv[2], &error);
        print_token_array(tokens);
        free_token_array(&tokens);

    } else if(strcmp(command, "parse") == 0) {

    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return EX_USAGE;
    }


    if (error) return EX_DATAERR;
    return EX_OK;
}


