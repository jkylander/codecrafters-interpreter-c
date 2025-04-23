#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "scanner.h"

typedef struct {
    const char *start;
    const char *current;
    int line;
} Scanner;

Scanner scanner;

struct string_pair {
  TokenType type;
  const char *str;
} token_strings[] = {
  #define TOKEN_KIND(e, s) {e, s},
    TOKEN_KINDS
  #undef TOKEN_KIND
};

static const char *str_from_token(TokenType type) {
    return token_strings[type].str;
}

static int count_decimals(double val) {
    int result = 0;
    double epsilon = DBL_EPSILON;
    double exponent = 1.0;

    while (fabs(fmod(val * exponent, 1.0)) > epsilon) {
        ++result;
        epsilon *= 10;
        exponent *= 10;
    }
    return result;
}

void print_token(Token token) {
    if (token.type == TOKEN_STRING) {
        printf("%s %.*s %.*s\n", str_from_token(token.type), token.length, token.start, token.length - 2, token.start + 1);
    } else if (token.type == TOKEN_NUMBER) {
        double value = strtod(token.start, NULL);

        if (value == (int) value) {
            printf("%s %.*s %.1f\n", str_from_token(token.type), token.length, token.start, value);
        } else {
            printf("%s %.*s %.*f\n", str_from_token(token.type), token.length, token.start, count_decimals(value), value);
        }
    } else if (token.type >= TOKEN_AND && token.type <= TOKEN_WHILE) {
        printf("%s %.*s null\n", str_from_token(token.type), token.length, token.start);
    } else if (token.type == TOKEN_ERROR) {
        fprintf(stderr, "[line %d] Error: %.*s\n", token.line, token.length, token.start);
    }
    else {
        printf("%s %.*s null\n", str_from_token(token.type), token.length, token.start);
    }
}

bool lex(const char *source) {
    bool hadError = false;
    initScanner(source);
    int line = -1;
    for (;;) {
        Token token = scanToken();
        print_token(token);
        if (token.type == TOKEN_ERROR) hadError = true;
        if (token.type == TOKEN_EOF) break;
    }
    return hadError;
}

void initScanner(const char *source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}
static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
static bool isDigit(char c) { return c >= '0' && c <= '9'; }
static bool isAtEnd() { return *scanner.current == '\0'; }
static char advance() {
    scanner.current++;
    return scanner.current[-1];
}

static char peek() { return scanner.current[0]; }

static char peekNext() {
    if (isAtEnd())
        return '\0';
    return scanner.current[1];
}

static bool match(char expected) {
    if (isAtEnd())
        return false;
    if (*scanner.current != expected)
        return false;
    scanner.current++;
    return true;
}

static Token makeToken(TokenType type) {
    return (Token) {
        .type = type,
        .start = scanner.start,
        .length = (int) (scanner.current - scanner.start),
        .line = scanner.line,
    };
}

static Token errorToken(const char *message) {
    return (Token) {
        .type = TOKEN_ERROR,
        .start = message,
        .length = (int) strlen(message),
        .line = scanner.line,
    };
}

static void skipWhitespace() {
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t': advance(); break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '/':
                if (peekNext() == '/') {
                    // A comment goes until the end of the line.
                    while (peek() != '\n' && !isAtEnd())
                        advance();
                } else {
                    return;
                }
                break;

            default: return;
        }
    }
}

static TokenType checkKeyword(int start, int length, const char *rest,
                              TokenType type) {
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
    switch (scanner.start[0]) {
        case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
        case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
        case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
        case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
        case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
        case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
        case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier() {
    while (isAlpha(peek()) || isDigit(peek()))
        advance();
    return makeToken(identifierType());
}

static Token string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n')
            scanner.line++;
        advance();
    }
    if (isAtEnd())
        return errorToken("Unterminated string.");
    advance();
    return makeToken(TOKEN_STRING);
}

static Token number() {
    while (isDigit(peek()))
        advance();

    if (peek() == '.' && isDigit(peekNext())) {
        // Consume the dot
        advance();
        while (isDigit(peek()))
            advance();
    }

    return makeToken(TOKEN_NUMBER);
}

Token scanToken() {
    skipWhitespace();
    scanner.start = scanner.current;

    if (isAtEnd())
        return makeToken(TOKEN_EOF);
    char c = advance();
    if (isAlpha(c))
        return identifier();
    if (isDigit(c))
        return number();

    switch (c) {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case '/': return makeToken(TOKEN_SLASH);
        case '*': return makeToken(TOKEN_STAR);
        case '!': return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<': return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"': return string();
        default: break;
    }

    char *error = malloc(25);
    sprintf(error, "Unexpected character: %c", c);
    return errorToken(error);
}
