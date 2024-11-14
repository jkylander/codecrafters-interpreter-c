#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef enum {
    LEFT_PAREN, RIGHT_PAREN,
    LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS,
    SEMICOLON, SLASH, STAR,

    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,

    IDENTIFIER, STRING, NUMBER,

    AND, CLASS, ELSE, FALSE, FUN,
    FOR, IF, NIL, OR, PRINT, RETURN,
    SUPER, THIS, TRUE, VAR, WHILE,
    EOF_TOKEN,

    TOKEN_COUNT,
} TokenList;

typedef struct {
    char *contents;
    size_t length;
    size_t line;
} FileData;

typedef struct {
    TokenList type;
    char *lexeme;
    char *literal;
    size_t line;
    bool error;
    bool comment;
} Token;

struct method_string_pair {
    TokenList type;
    char *str;
} known_methods[] = {
    {LEFT_PAREN, "LEFT_PAREN"}, {RIGHT_PAREN, "RIGHT_PAREN"},
    {LEFT_BRACE, "LEFT_BRACE"}, {RIGHT_BRACE, "RIGHT_BRACE"},
    {COMMA, "COMMA"}, {DOT, "DOT"}, {MINUS, "MINUS"}, {PLUS, "PLUS"},
    {SEMICOLON, "SEMICOLON"}, {SLASH, "SLASH"}, {STAR, "STAR"},

    {BANG, "BANG"}, {BANG_EQUAL, "BANG_EQUAL"},
    {EQUAL, "EQUAL"}, {EQUAL_EQUAL, "EQUAL_EQUAL"},
    {GREATER, "GREATER"}, {GREATER_EQUAL, "GREATER_EQUAL"},
    {LESS, "LESS"}, {LESS_EQUAL, "LESS_EQUAL"},

    {IDENTIFIER, "IDENTIFIER"}, {STRING, "STRING"}, {NUMBER, "NUMBER"},

    {AND, "AND"}, {CLASS, "CLASS"}, {ELSE, "ELSE"}, {FALSE, "FALSE"}, {FUN, "FUN"},
    {FOR, "FOR"}, {IF, "IF"}, {NIL, "NIL"}, {OR, "OR"}, {PRINT, "PRINT"}, {RETURN, "RETURN"},
    {SUPER, "SUPER"}, {THIS, "THIS"}, {TRUE, "TRUE"}, {VAR, "VAR"}, {WHILE, "WHILE"},

    {EOF_TOKEN, "EOF"},
};

TokenList type_from_str(char *str);
char *str_from_type(TokenList type);
FileData *read_file_contents(const char *filename);
Token *scan_token(FileData *data, int *position);
void print_token(Token token);
bool match(FileData *data, int *position, char c);
bool isAtEnd(int position, int len);
char advance(FileData *data, int *position);
void interpreter(FileData *data);
char peek(FileData *data, int pos);

FileData *read_file_contents(const char *filename);
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

        FileData *file_data = read_file_contents(argv[2]);
        if ( file_data->length > 0) {
            interpreter(file_data);
        } else {
            printf("EOF  null\n");
            return 0;
        }
        free(file_data->contents);
        free(file_data);

    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}

void interpreter(FileData *data) {
    int position = 0;
    data->line = 1;
    bool error = false;
    Token *token;
    Token **validTokens = NULL;
    int tokenCount = 0;
    bool onlyWhitespace = true;

    while(!isAtEnd(position, data->length)) {

        if (data->contents[position] != ' ' &&
            data->contents[position] != '\t' &&
            data->contents[position] != '\r' &&
            data->contents[position] != '\n') {
            onlyWhitespace = false;
        }

        if (data->contents[position] == '\n') {
            data->line++;
            position++;
        } else {
            token = scan_token(data, &position);

            if (token->comment) {
                continue;
            }
            if (!(token->error)) {
                validTokens = realloc(validTokens, sizeof(Token *) * (tokenCount + 1));
                validTokens[tokenCount] = token;
                tokenCount++;
            } else {
                if (token->type == STRING) {
                    fprintf(stderr, "[line %zu] Error: Unterminated string.\n", token->line);
                } else {
                    fprintf(stderr, "[line %zu] Error: Unexpected character: %s\n", token->line, token->lexeme);
                }
                error = true;
                free(token->lexeme);
                free(token);
            }
        }
    }

    if (tokenCount == 0 || validTokens[tokenCount - 1]->type != EOF_TOKEN) {
        Token *eof = malloc(sizeof(Token));
        eof->type = EOF_TOKEN;
        eof->lexeme = calloc(1, 1);
        eof->literal = NULL;
        eof->line = data->line;
        eof->error = false;
        eof->comment = false;

        validTokens = realloc(validTokens, sizeof(Token *) * (tokenCount + 1));
        validTokens[tokenCount] = eof;
        tokenCount++;
    }

    for (int i = 0; i < tokenCount; i++) {
        print_token(*validTokens[i]);
        if (validTokens[i]->lexeme)  free(validTokens[i]->lexeme);
        if (validTokens[i]->literal) free(validTokens[i]->literal);
        free(validTokens[i]);
    }

    if (error) exit(65);
    exit(0);
}

TokenList type_from_str(char *str) {
    int n = sizeof(known_methods) / sizeof(known_methods[0]);
    for (int i = 0; i < n; ++i) {
        if (strcmp(str, known_methods[i].str) == 0) {
            return known_methods[i].type;
        }
    }
    return -1;
}

char *str_from_type(TokenList type) {
    return known_methods[type].str;
}

Token *scan_token(FileData *data, int *position) {
    Token *token = malloc(sizeof(Token));
    token->lexeme = calloc(1, 3);
    token->error = false;
    token->comment = false;

    // Skip whitespace and handle newline
    while (!isAtEnd(*position, data->length)) {
        char c = peek(data, *position);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                (*position)++;
                continue;
            case '\n':
                data->line++;
                (*position)++;
                continue;
            default:
                goto process_token;
        }
    }

process_token:
    token->line = data->line;
    if (isAtEnd(*position, data->length)) {
        token->type = EOF_TOKEN;
        return token;
    }
    char c = advance(data, position);
    token->lexeme[0] = c;

    switch (c) {
        case '(': token->type = LEFT_PAREN; break;
        case ')': token->type = RIGHT_PAREN; break;
        case '{': token->type = LEFT_BRACE; break;
        case '}': token->type = RIGHT_BRACE; break;
        case ',': token->type = COMMA; break;
        case '.': token->type = DOT; break;
        case '-': token->type = MINUS; break;
        case '+': token->type = PLUS; break;
        case ';': token->type = SEMICOLON; break;
        case '*': token->type = STAR; break;

        case '!': {
            token->type = match(data, position, '=') ? BANG_EQUAL : BANG;
            if (token->type == BANG_EQUAL) {
                token->lexeme[1] = '=';
            }
        } break;

        case '=': {
            token->type = match(data, position, '=') ? EQUAL_EQUAL : EQUAL;
            if (token->type == EQUAL_EQUAL) {
                token->lexeme[1] = '=';
            }

        } break;

        case '<': {
            token->type = match(data, position, '=') ? LESS_EQUAL : LESS;
            if (token->type == LESS_EQUAL) {
                token->lexeme[1] = '=';
            }
        } break;

        case '>': {
            token->type = match(data, position, '=') ? GREATER_EQUAL: GREATER;
            if (token->type == GREATER_EQUAL) {
                token->lexeme[1] = '=';
            }
        } break;

        case '/': {
            if (match(data, position, '/')) {
                while (!isAtEnd(*position, data->length) &&
                        peek(data, *position) != '\n') {
                    (*position)++;
                }
                token->comment = true;
            } else {
                token->type = SLASH;
            }
        } break;

        case '"': {
            int start = *position;
            token->type = STRING;

            // Scan until closing quote or end
            while (data->contents[*position] != '"' &&
                !isAtEnd(*position, data->length)) {
                if (data->contents[*position] == '\n') {
                    data->line++;
                }
                (*position)++;
            }


            // Handle unterminated string
            if (*position >= data->length) {
                token->line = data->line;
                token->error = true;
                break;
            }
            int len = *position - start;
            // Allocate and copy the string
            char *lexeme = malloc(len + 3); // 2 for quotes, 1 for null
            lexeme[0] = '"';
            strncpy(lexeme, &data->contents[start], len);
            lexeme[len + 1] = '"';
            lexeme[len + 2] = '\0';
            free(token->lexeme);
            token->lexeme = lexeme;

            char *literal = malloc(len + 1);
            strncpy(literal, &data->contents[start], len);
            literal[len] = '\0';
            token->literal = literal;

            (*position)++; // Skip closing quote
        } break;

        default: {
            token->error = true;
            token->line = data->line;
        }
    }

    return token;
}

char peek(FileData *data, int pos) {
    return data->contents[pos];
}

bool match(FileData *data, int *position, char c) {
    if (isAtEnd(*position, data->length) ||
        data->contents[*position] != c) {
        return false;
    }
    (*position)++;
    return true;
}

char advance(FileData *data, int *position) {
    char c = data->contents[(*position)++];
    return c;
}

bool isAtEnd(int position, int len) { return position >= len; }

void print_token(Token token) {
    if (token.type == STRING) {
        printf("%s \"%s\" %s\n", str_from_type(token.type), token.literal, token.literal);
    } else {
        printf("%s %s null\n", str_from_type(token.type), token.lexeme);
    }
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
    file_data->line = 0;
    fclose(file);

    return file_data;
}
