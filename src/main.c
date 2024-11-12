#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

bool hadError;

enum TokenList {
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS,
    SEMICOLON, SLASH, STAR,

    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    LESS, LESS_EQUAL,

    IDENTIFIER, STRING, NUMBER,

    AND, CLASS, ELSE, FALSE, FUN,
    FOR, IF, NIL, OR, PRINT, RETURN,
    SUPER, THIS, TRUE, VAR, WHILE,
    EOF_TOKEN,

    TOKEN_COUNT,
};

typedef struct {
    char *contents;
    size_t length;
    size_t lines;
} FileData;

typedef struct {
    int type;
    char *lexeme;
} Token;

struct method_string_pair {
    enum TokenList type;
    char *str;
} known_methods[] = {
    {LEFT_PAREN, "LEFT_PAREN"}, {RIGHT_PAREN, "RIGHT_PAREN"},
    {LEFT_BRACE, "LEFT_BRACE"}, {RIGHT_BRACE, "RIGHT_BRACE"},
    {COMMA, "COMMA"}, {DOT, "DOT"}, {MINUS, "MINUS"}, {PLUS, "PLUS"},
    {SEMICOLON, "SEMICOLON"}, {SLASH, "SLASH"}, {STAR, "STAR"},

    {BANG, "BANG"}, {BANG_EQUAL, "BANG_EQUAL"},
    {EQUAL, "EQUAL"}, {EQUAL_EQUAL, "EQUAL_EQUAL"},
    {LESS, "LESS"}, {LESS_EQUAL, "LESS_EQUAL"},

    {IDENTIFIER, "IDENTIFIER"}, {STRING, "STRING"}, {NUMBER, "NUMBER"},

    {AND, "AND"}, {CLASS, "CLASS"}, {ELSE, "ELSE"}, {FALSE, "FALSE"}, {FUN, "FUN"},
    {FOR, "FOR"}, {IF, "IF"}, {NIL, "NIL"}, {OR, "OR"}, {PRINT, "PRINT"}, {RETURN, "RETURN"},
    {SUPER, "SUPER"}, {THIS, "THIS"}, {TRUE, "TRUE"}, {VAR, "VAR"}, {WHILE, "WHILE"},

    {EOF_TOKEN, "EOF"},
};

enum TokenList type_from_str(char *str) {
    int n = sizeof(known_methods) / sizeof(known_methods[0]);
    for (int i = 0; i < n; ++i) {
        if (strcmp(str, known_methods[i].str) == 0) {
            return known_methods[i].type;
        }
    }
    hadError = true;
    return -1;
}

char *str_from_type(enum TokenList type) {
    return known_methods[type].str;
}

FileData *read_file_contents(const char *filename);

Token *scan_tokens(FileData *data, size_t *num_tokens) {
    Token *tokens = malloc((data->length + 1) * sizeof(Token));
    data->lines++;

    for (int i = 0; i < data->length; i++) {
        Token *token = &tokens[*num_tokens];
        (*num_tokens)++;

        switch (data->contents[i]) {
            case '(': {
                token->type = LEFT_PAREN;
                token->lexeme = "(";
            } break;

            case ')': {
                token->type = RIGHT_PAREN;
                token->lexeme = ")";
            } break;

            case '{': {
                token->type = LEFT_BRACE;
                token->lexeme = "{";
            } break;

            case '}': {
                token->type = RIGHT_BRACE;
                token->lexeme = "}";
            } break;

            case ',': {
                token->type = COMMA;
                token->lexeme = ",";
            } break;

            case '.': {
                token->type = DOT;
                token->lexeme = ".";
            } break;

            case '-': {
                token->type = MINUS;
                token->lexeme = "-";
            } break;

            case '+': {
                token->type = PLUS;
                token->lexeme = "+";
            } break;

            case ';': {
                token->type = SEMICOLON;
                token->lexeme = ";";
            } break;

            case '*': {
                token->type = STAR;
                token->lexeme = "*";
            } break;

            default: {
                fprintf(stderr, "[line %zu] Error: Unexpected character: %c\n", data->lines, data->contents[i]);
                hadError = true;
                (*num_tokens)--;
            }
        }
    }

    Token *token = &tokens[*num_tokens];
    token->type = EOF_TOKEN;
    token->lexeme = "";
    (*num_tokens)++;
    return tokens;
}

void print_token(Token token) {
    printf("%s %s null\n", str_from_type(token.type), token.lexeme);
}

void print_tokens(Token *tokens, size_t num) {
    for (int i = 0; i < num; i++) {
        print_token(tokens[i]);
    }
}

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

        FileData *file_data = read_file_contents(argv[2]);

        size_t num_tokens = 0;
        Token *tokens = scan_tokens(file_data, &num_tokens);
        print_tokens(tokens, num_tokens);
        if (hadError) return 65;

        free(tokens);
        free(file_data->contents);
        free(file_data);
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}

FileData *read_file_contents(const char *filename) {
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
    FileData *file_data = malloc(sizeof(FileData));
    file_data->contents = file_contents;
    file_data->length = file_size;
    file_data->lines = 0;
    fclose(file);

    return file_data;
}
