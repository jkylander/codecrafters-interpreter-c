#include "token.h"
#include "ansi.h"
#include <stdlib.h>

// Helper definitions
void print_token(Token token);
bool match(char c);
bool isAtEnd(int len);
char advance();
void interpreter();
char peek();
char peekNext();
bool isDigit(char c);
bool isAlpha(char c);
bool isAlphaNumeric(char c);
void string(Token *token);
void number(Token *token);
void identifier(Token *token);
int decimals_to_print(char *src);
void interpreter();
Token scan_token();
FileData *read_file_contents(const char *filename);
TokenList token_from_str(char *str);
const char *str_from_token(TokenList type);

// Globals
static int start = 0;
static int current = 0;
static int line = 1;
static FileData *data;

struct string_pair {
    TokenList type;
    const char *str;
} known_tokens[] = {
    {LEFT_PAREN, "LEFT_PAREN"}, {RIGHT_PAREN, "RIGHT_PAREN"},
    {LEFT_BRACE, "LEFT_BRACE"}, {RIGHT_BRACE, "RIGHT_BRACE"},
    {COMMA, "COMMA"}, {DOT, "DOT"}, {MINUS, "MINUS"}, {PLUS, "PLUS"},
    {SEMICOLON, "SEMICOLON"}, {SLASH, "SLASH"}, {STAR, "STAR"},

    {BANG, "BANG"}, {BANG_EQUAL, "BANG_EQUAL"},
    {EQUAL, "EQUAL"}, {EQUAL_EQUAL, "EQUAL_EQUAL"},
    {GREATER, "GREATER"}, {GREATER_EQUAL, "GREATER_EQUAL"},
    {LESS, "LESS"}, {LESS_EQUAL, "LESS_EQUAL"},

    {IDENTIFIER, "IDENTIFIER"}, {STRING, "STRING"}, {NUMBER, "NUMBER"},

    {AND, "and"}, {CLASS, "class"}, {ELSE, "else"}, {FALSE, "false"}, {FUN, "fun"},
    {FOR, "for"}, {IF, "if"}, {NIL, "nil"}, {OR, "or"}, {PRINT, "print"}, {RETURN, "return"},
    {SUPER, "super"}, {THIS, "this"}, {TRUE, "true"}, {VAR, "var"}, {WHILE, "while"},

    {EOF_TOKEN, "EOF"},
};

TokenArray create_token_array(size_t initial_capacity) {
    TokenArray array;
    array.tokens = malloc(initial_capacity * sizeof(Token));
    array.count = 0;
    array.capacity = initial_capacity;
    return array;
}

void token_array_add(TokenArray *array, Token token) {
    if (array->count == array->capacity) {
        array->capacity *= 2;
        array->tokens = realloc(array->tokens, array->capacity * sizeof(Token));
    }
    array->tokens[array->count++] = token;
}

void free_token_array(TokenArray *array) {
    for (size_t i = 0; i < array->count ; i++) {
        if (array->tokens[i].lexeme) free(array->tokens[i].lexeme);
        if (array->tokens[i].literal) free(array->tokens[i].literal);
    }
    free(array->tokens);
    array->tokens = NULL;
    array->count = array->capacity = 0;
}

TokenArray tokenize(char *file, bool *error) {
    TokenArray token_array = create_token_array(16); // 16 tokens initial size

    data = read_file_contents(file);
    if (data == NULL) {
        exit(EX_USAGE);
    }
    if ( data->length <= 0) {
        printf("EOF  null\n");
        exit(EX_OK);
    }

    Token token;
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

            if (token.comment) {
                continue;
            }
            if (!(token.error)) {
                token_array_add(&token_array, token);
            } else {
                if (token.type == STRING) {
                    fprintf(stderr, "[line %zu] Error: Unterminated string.\n", token.line);
                } else {
                    fprintf(stderr, "[line %zu] Error: Unexpected character: %s\n", token.line, token.lexeme);
                }
                *error = true;
                if (token.lexeme) free(token.lexeme);
                if (token.literal) free(token.literal);
            }
        }
    }

    if (token_array.count == 0 || token_array.tokens[token_array.count -1].type != EOF_TOKEN) {
        Token eof = {
            .type = EOF_TOKEN,
            .lexeme = calloc(1, 1),
            .literal = NULL,
            .line = line,
            .error = false,
            .comment = false,
        };
        token_array_add(&token_array, eof);
    }


    free(data->contents);
    free(data);
    return token_array;
}

void print_token_array(TokenArray token_array) {
    for (int i = 0; i < token_array.count; i++) {
        print_token(token_array.tokens[i]);
    }
}

void print_token(Token token) {
    if (token.type == STRING) {
        printf("%s %s %s\n", str_from_token(token.type), token.lexeme, (char *)token.literal);
    } else if (token.type == NUMBER) {
        printf("%s %s %.*f\n", str_from_token(token.type), token.lexeme, decimals_to_print(token.lexeme), *((double*)token.literal));
    } else if (token.type >= AND && token.type <= WHILE) {
        char type[7];
        strcpy(type, token.lexeme);
        for (int i = 0, n = strlen(token.lexeme); i < n; i++) {
            type[i] = toupper(token.lexeme[i]);
        }
        printf("%s %s null\n", type, token.lexeme);
    }
    else {
        printf("%s %s null\n", str_from_token(token.type), token.lexeme);
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

TokenList token_from_str(char *str) {
    for (int i = 0; i < TOKEN_COUNT; ++i) {
        if (strcmp(str, known_tokens[i].str) == 0) {
            return known_tokens[i].type;
        }
    }
    return -1;
}

const char *str_from_token(TokenList type) {
    return known_tokens[type].str;
}

Token scan_token() {
    Token token = {0};
    token.lexeme = calloc(1, 3);
    token.error = false;
    token.comment = false;

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
    token.line = line;
    if (isAtEnd(data->length)) {
        token.type = EOF_TOKEN;
        return token;
    }
    char c = advance();
    token.lexeme[0] = c;

    switch (c) {
        case '(': token.type = LEFT_PAREN; break;
        case ')': token.type = RIGHT_PAREN; break;
        case '{': token.type = LEFT_BRACE; break;
        case '}': token.type = RIGHT_BRACE; break;
        case ',': token.type = COMMA; break;
        case '.': token.type = DOT; break;
        case '-': token.type = MINUS; break;
        case '+': token.type = PLUS; break;
        case ';': token.type = SEMICOLON; break;
        case '*': token.type = STAR; break;

        case '!': {
            token.type = match('=') ? BANG_EQUAL : BANG;
            if (token.type == BANG_EQUAL) {
                token.lexeme[1] = '=';
            }
        } break;

        case '=': {
            token.type = match('=') ? EQUAL_EQUAL : EQUAL;
            if (token.type == EQUAL_EQUAL) {
                token.lexeme[1] = '=';
            }

        } break;

        case '<': {
            token.type = match('=') ? LESS_EQUAL : LESS;
            if (token.type == LESS_EQUAL) {
                token.lexeme[1] = '=';
            }
        } break;

        case '>': {
            token.type = match('=') ? GREATER_EQUAL: GREATER;
            if (token.type == GREATER_EQUAL) {
                token.lexeme[1] = '=';
            }
        } break;

        case '/': {
            if (match('/')) {
                while (!isAtEnd(data->length) &&
                        peek() != '\n') {
                    current++;
                }
                token.comment = true;
            } else {
                token.type = SLASH;
            }
        } break;

        case '"': {
            string(&token);
        } break;

        default: {
            if (isDigit(c)) {
                number(&token);
                break;
            } else if (isAlpha(c)) {
                identifier(&token);
                break;
            }
            token.error = true;
            token.line = line;
        }
    }
    return token;
}

void identifier(Token *token) {
    token->line = line;
    start = current - 1;
    while (isAlphaNumeric(peek())) advance();
    int len = current - start;
    char *lexeme = malloc(len + 1);
    strncpy(lexeme, &data->contents[start], len);
    lexeme[len] = '\0';

    TokenList keyword = token_from_str(lexeme);
    if (keyword == -1) token->type = IDENTIFIER;
    else token->type = keyword;
    free(token->lexeme);
    token->lexeme = lexeme;
    token->length = len;
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
    token->length = len;
}

bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c == '_');
}

bool isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
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
    token->length = len;

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
