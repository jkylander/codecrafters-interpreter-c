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
} FileData;

typedef struct {
    TokenList type;
    char *lexeme;
    void *literal;
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

// Globals
static int start = 0;
static int current = 0;
static int line = 1;
FileData *data;

TokenList type_from_str(char *str);
char *str_from_type(TokenList type);
FileData *read_file_contents(const char *filename);
Token *scan_token();
void print_token(Token token);
bool match(char c);
bool isAtEnd(int len);
char advance();
void interpreter();
char peek();
char peekNext();
bool isDigit(char c);
void string(Token *token);
void number(Token *token);
int decimals_to_print(char *src);

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

        data = read_file_contents(argv[2]);
        if ( data->length > 0) {
            interpreter();
        } else {
            printf("EOF  null\n");
            return 0;
        }
        free(data->contents);
        free(data);

    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}

void interpreter() {
    bool error = false;
    Token *token;
    Token **validTokens = NULL;
    int tokenCount = 0;
    bool onlyWhitespace = true;

    while(!isAtEnd(data->length)) {

        if (data->contents[current] != ' ' &&
            data->contents[current] != '\t' &&
            data->contents[current] != '\r' &&
            data->contents[current] != '\n') {
            onlyWhitespace = false;
        }

        if (data->contents[current] == '\n') {
            line++;
            current++;
        } else {
            token = scan_token();

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
        eof->line = line;
        eof->error = false;
        eof->comment = false;

        validTokens = realloc(validTokens, sizeof(Token *) * (tokenCount + 1));
        validTokens[tokenCount] = eof;
        tokenCount++;
    }

    for (int i = 0; i < tokenCount; i++) {
        print_token(*validTokens[i]);
        if (validTokens[i]->lexeme) free(validTokens[i]->lexeme);
        free(validTokens[i]);
    }

    if (error) exit(65);
    exit(0);
}

void print_token(Token token) {
    if (token.type == STRING) {
        printf("%s %s %s\n", str_from_type(token.type), token.lexeme, (char *)token.literal);
    } else if (token.type == NUMBER) {
        printf("%s %s %.*f\n", str_from_type(token.type), token.lexeme, decimals_to_print(token.lexeme), *((double*)token.literal));

    } else {
        printf("%s %s null\n", str_from_type(token.type), token.lexeme);
    }
}

int decimals_to_print(char *src) {
    int len = strlen(src);
    int response = 0;
    int digit = 0;
    int zero = 0;
    for (int i = 0; i < len; i++) {
        if (src[i] == '0' && digit) {
            zero++;
        }
        if (src[i] != '0' && digit) {
            response = response + 1 + zero;
            zero = 0;
        }
        if (src[i] == '.') {
            digit = 1;
        }
    }
    if (!digit || zero) {
        response = 1;
    }
    return response;
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

Token *scan_token() {
    Token *token = malloc(sizeof(Token));
    token->lexeme = calloc(1, 3);
    token->error = false;
    token->comment = false;

    // Skip whitespace and handle newline
    while (!isAtEnd(data->length)) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                current++;
                continue;
            case '\n':
                line++;
                current++;
                continue;
            default:
                goto process_token;
        }
    }

process_token:
    token->line = line;
    if (isAtEnd(data->length)) {
        token->type = EOF_TOKEN;
        return token;
    }
    char c = advance();
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
            token->type = match('=') ? BANG_EQUAL : BANG;
            if (token->type == BANG_EQUAL) {
                token->lexeme[1] = '=';
            }
        } break;

        case '=': {
            token->type = match('=') ? EQUAL_EQUAL : EQUAL;
            if (token->type == EQUAL_EQUAL) {
                token->lexeme[1] = '=';
            }

        } break;

        case '<': {
            token->type = match('=') ? LESS_EQUAL : LESS;
            if (token->type == LESS_EQUAL) {
                token->lexeme[1] = '=';
            }
        } break;

        case '>': {
            token->type = match('=') ? GREATER_EQUAL: GREATER;
            if (token->type == GREATER_EQUAL) {
                token->lexeme[1] = '=';
            }
        } break;

        case '/': {
            if (match('/')) {
                while (!isAtEnd(data->length) &&
                        peek() != '\n') {
                    current++;
                }
                token->comment = true;
            } else {
                token->type = SLASH;
            }
        } break;

        case '"': {
            string(token);
        } break;

        default: {
            if (isDigit(c)) {
                number(token);
                break;
            }
            token->error = true;
            token->line = line;
        }
    }
    return token;
}

void string(Token *token){
    start = current;
    token->type = STRING;
    while (peek() != '"' && !isAtEnd(data->length)) {
        if ((peek() == '\n')) line++;
        advance();
    }
    if (isAtEnd(data->length)) {
        token->line = line;
        token->error = true;
        return;
    }
    int len = current - start;

    // The closing ".
    advance();

    // Allocate and copy the string
    char *lexeme = malloc(len + 3); // 2 for quotes, 1 for null
    lexeme[0] = '"';
    strncpy(lexeme + 1, &data->contents[start], len);
    lexeme[len + 1] = '"';
    lexeme[len + 2] = '\0';
    free(token->lexeme);
    token->lexeme = lexeme;

    char *literal = malloc(len + 1);
    strncpy(literal, &data->contents[start], len);
    literal[len] = '\0';
    token->literal = literal;
    token->line = line;
}

bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

void number(Token *token) {
    token->type = NUMBER;
    start = current - 1;
    while (isDigit(peek())) {
        advance();
    }
    if (peek() == '.' && isDigit(peekNext())) {
        advance();
        while (isDigit(peek())) advance();
    }
    int len = current - start;
    char *lexeme = malloc(len + 1);
    strncpy(lexeme, &data->contents[start], len);
    lexeme[len] = '\0';
    free(token->lexeme);
    token->lexeme = lexeme;

    double number = strtod(lexeme, NULL);
    token->literal = malloc(sizeof(double));
    *((double *)(token->literal)) = number;
    token->line = line;
}

char peek() {
    return data->contents[current];
}

char peekNext() {
    if (current <= data->length) {
    return data->contents[current + 1];
    }
    return '\0';
}

bool match(char c) {
    if (isAtEnd(data->length) ||
        data->contents[current] != c) {
        return false;
    }
    current++;
    return true;
}

char advance() {
    char c = data->contents[current++];
    return c;
}

bool isAtEnd(int len) { return current >= len; }

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
    fclose(file);

    return file_data;
}
