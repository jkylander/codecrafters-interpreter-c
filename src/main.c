#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <stdio.h>
#include "ast.h"
#include "parse.h"
#include "scanner.h"
#include "interpreter.h"
#include "stmt.h"

const char *read_file(const char *filename);
int main(int argc, char *argv[]) {
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (argc < 3) {
        fprintf(stderr, "Usage: ./your_program tokenize|parse|evaluate|run <filename>\n");
        return EX_USAGE;
    }

    const char *command = argv[1];
    bool error = false;
    const char *data = read_file(argv[2]);
    TokenArray tokens = scan(data);
    if (strcmp(command, "tokenize") == 0) {
        if (tokens.hadError == true) error = true;
        print_token_array(tokens);
        free_token_array(&tokens);

    } else if(strcmp(command, "parse") == 0) {
        if (!error) {
            Parser parser = create_parser(&tokens);
            if (parser.hadError) error = true;
            Expr ast = *parse_expression(&parser);
            print_ast(&ast);
            printf("\n");
            free_token_array(&tokens);
        }
    } else if (strcmp(command, "evaluate") == 0) {
        if (tokens.hadError == true) error = true;
        Parser parser = create_parser(&tokens);
        if (parser.hadError) error = true;
        Expr ast = *parse_expression(&parser);
        Object object = evaluate(&ast);
        print_object(&object);

    } else if (strcmp(command, "run") == 0) {
        if (tokens.hadError == true) error = true;
        Parser parser = create_parser(&tokens);
        if (parser.hadError) error = true;
        StmtArray statements = parse(&parser);
        print_statements(&statements);

    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return EX_USAGE;
    }

    if (error) return EX_DATAERR;
    return EX_OK;
}


const char *read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error reading file: %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *file_contents = malloc(file_size + 1);
    if (file_contents == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(file_contents, 1, file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "Error reading file contents\n");
        free(file_contents);
        fclose(file);
        return NULL;
    }

    file_contents[file_size] = '\0';

    return file_contents;
}
