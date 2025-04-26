#include "compiler.h"
#include "chunk.h"
#include "scanner.h"
#include <string.h>
#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

Parser parser;
Chunk *compilingChunk;

typedef enum {
    EX_ASSIGN,
    EX_BINARY,
    EX_CALL,
    EX_GET,
    EX_GROUPING,
    EX_LITERAL,
    EX_LOGICAL,
    EX_SET,
    EX_SUPER,
    EX_THIS,
    EX_UNARY,
    EX_VARIABLE,

    EX_EXPR_COUNT
} ExprType;

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_STRING,
} ObjectType;

typedef struct {
    ObjectType type;
    union {
        bool boolean;
        Value number;
        char *string;
    } as;
} Object;

typedef struct Expr Expr;

struct Expr {
    ExprType type;
    int line;
    union {
        struct {
            Token operator;
            Expr *left;
            Expr *right;
        } binary;
        struct {
            Expr *expression;
        } grouping;
        struct {
            Object *value;
        } literal;
        struct {
            Token operator;
            Expr *right;
        } unary;
    } as;
};

static Expr *create_binary_expr(Token binary_op, Expr *left, Expr *right) {
    Expr *expr = malloc(sizeof(Expr));
    expr->type = EX_BINARY;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    expr->as.binary.operator= binary_op;
    expr->line = left->line;
    return expr;
}

static Expr *create_literal_expr(Token token) {
    Expr *expr = malloc(sizeof(Expr));
    expr->type = EX_LITERAL;
    Object *v = malloc(sizeof(Object));

    if (token.type == TOKEN_NUMBER) {
        v->type = VAL_NUMBER;
        v->as.number = strtod(token.start, NULL);
    } else if (token.type == TOKEN_NIL) {
        v->type = VAL_NIL;
        v->as.number = 0;
    } else if (token.type == TOKEN_TRUE || token.type == TOKEN_FALSE) {
        v->type = VAL_BOOL;
        v->as.boolean = token.type == TOKEN_TRUE;
    } else {
        v->type = VAL_STRING;
        char *str = malloc(token.length + 1);
        strncpy(str, token.start, token.length);
        str[token.length] = '\0';
        v->as.string = str;
    }

    expr->as.literal.value = v;
    expr->line = token.line;
    return expr;
}

static Expr *create_grouping_expr(Expr *expression) {
    Expr *expr = malloc(sizeof(Expr));
    expr->type = EX_GROUPING;
    expr->as.grouping.expression = expression;
    expr->line = expression->line;
    return expr;
}

void free_expr(Expr *expr) {
    if (!expr)
        return;
    switch (expr->type) {
        case EX_BINARY:
            free_expr(expr->as.binary.left);
            free_expr(expr->as.binary.right);
            break;
        case EX_GROUPING: free_expr(expr->as.grouping.expression); break;
        case EX_UNARY: free_expr(expr->as.unary.right); break;
        case EX_LITERAL: break;
        default: break;
    }
}

void print_ast(Expr *expr) {
    if (expr != NULL) {
        if (expr->type == EX_LITERAL) {
            if (expr->as.literal.value->type == VAL_STRING) {
                printf("%s", expr->as.literal.value->as.string);
            } else if (expr->as.literal.value->type == VAL_BOOL) {
                printf("%s", expr->as.literal.value->as.boolean == 1 ? "true"
                                                                     : "false");

            } else if (expr->as.literal.value->type == VAL_NIL) {
                printf("nil");

            } else if (expr->as.literal.value->type == VAL_NUMBER) {
                double value = expr->as.literal.value->as.number;
                if (value == (int) value) {
                    printf("%.1f", value);
                } else {
                    printf("%g", value);
                }
            } else {
                printf("%s", expr->as.literal.value->as.string);
            }
        } else if (expr->type == EX_BINARY) {
            printf("(%.*s ", expr->as.binary.operator.length,
                   expr->as.binary.operator.start);
            print_ast(expr->as.binary.left);
            printf(" ");
            print_ast(expr->as.binary.right);
            printf(")");
        } else if (expr->type == EX_GROUPING) {
            printf("(group ");
            print_ast(expr->as.grouping.expression);
            printf(")");
        } else if (expr->type == EX_UNARY) {
            printf("(%.*s ", expr->as.unary.operator.length,
                   expr->as.unary.operator.start);
            print_ast(expr->as.unary.right);
            printf(")");
        }
    }
    printf("\n");
}

static Chunk *currentChunk() { return compilingChunk; }

static void errorAt(Token *token, const char *message) {
    if (parser.panicMode)
        return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // nop
    } else {
        fprintf(stderr, " at %.*s", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void error(const char *message) { errorAt(&parser.previous, message); }

static void errorAtCurrent(const char *message) {
    errorAt(&parser.current, message);
}

static void advance() {
    parser.previous = parser.current;
    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR)
            break;

        errorAtCurrent(parser.current.start);
    }
}

static void consume(TokenType type, const char *message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitReturn() { emitByte(OP_RETURN); }

static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }
    return (uint8_t) constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void endCompiler() {
    emitReturn();
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

static void expression();
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void binary() {
    TokenType operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);
    parsePrecedence((Precedence) (rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_PLUS: emitByte(OP_ADD); break;
        case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
        default: return;
    }
}

static void expression() { parsePrecedence(PREC_ASSIGNMENT); }

static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(value);
}

static void unary() {
    TokenType operatorType = parser.previous.type;

    parsePrecedence(PREC_UNARY);

    switch (operatorType) {
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return;
    }
}

// clang-format off
ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping,    nullptr,   PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {nullptr,     nullptr,   PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_COMMA]         = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_DOT]           = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_MINUS]         = {unary,       binary,    PREC_TERM},
  [TOKEN_PLUS]          = {nullptr,     binary,    PREC_TERM},
  [TOKEN_SEMICOLON]     = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_SLASH]         = {nullptr,     binary,    PREC_FACTOR},
  [TOKEN_STAR]          = {nullptr,     binary,    PREC_FACTOR},
  [TOKEN_BANG]          = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_EQUAL]         = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_GREATER]       = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_GREATER_EQUAL] = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_LESS]          = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_LESS_EQUAL]    = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_IDENTIFIER]    = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_STRING]        = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,      nullptr,   PREC_NONE},
  [TOKEN_AND]           = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_CLASS]         = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_ELSE]          = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_FALSE]         = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_FOR]           = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_FUN]           = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_IF]            = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_NIL]           = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_OR]            = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_PRINT]         = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_RETURN]        = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_SUPER]         = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_THIS]          = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_TRUE]          = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_VAR]           = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_WHILE]         = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_ERROR]         = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_EOF]           = {nullptr,     nullptr,   PREC_NONE},
};
// clang-format on

static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == nullptr) {
        error("Expect expression");
        return;
    }

    prefixRule();

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}

static ParseRule *getRule(TokenType type) { return &rules[type]; }


static bool match(TokenType type) {
    if (parser.current.type == type) {
        advance();
        return true;
    }
    return false;
}

static Expr *equality();
Expr *parse_expression() {
    return equality();
}

static Expr *primary() {
    Token current = parser.current;
    if (current.type == TOKEN_EOF) {
        errorAtCurrent("Unexpected end of input.");
        return nullptr;
    }

    if (match(TOKEN_TRUE) || match(TOKEN_FALSE) ||
        match(TOKEN_NIL)) {
        return create_literal_expr(parser.previous);
    }
    if (match(TOKEN_NUMBER) ||
        match(TOKEN_STRING)) {
        return create_literal_expr(parser.previous);
    }

    if (match(TOKEN_LEFT_PAREN)) {
        Expr *expr = parse_expression();
        consume(TOKEN_RIGHT_PAREN, "Expect ') after expression");
        return create_grouping_expr(expr);
    }
    errorAtCurrent("Expect expression.");
    return nullptr;
}


static Expr *exp_unary() {
    if (match(TOKEN_BANG) || match(TOKEN_MINUS)) {
        Token operator = parser.previous;
        Expr *right = exp_unary();
        Expr *r = malloc(sizeof(Expr));
        r->type = EX_UNARY;
        r->as.unary.right = right;
        r->as.unary.operator = operator;
        r->line = right->line;
        return r;
    }
    return primary();
}


static Expr *factor() {
    Expr *expr = exp_unary();
    while (match(TOKEN_SLASH) || match(TOKEN_STAR)) {
        Token operator = parser.previous;
        Expr *right = exp_unary();
        expr = create_binary_expr(operator, expr, right);
    }
    return expr;
}

static Expr *term() {
    Expr *expr = factor();
    while (match(TOKEN_MINUS) || match(TOKEN_PLUS)) {
        Token operator = parser.previous;
        Expr *right = factor();
        expr = create_binary_expr(operator, expr, right);
    }
    return expr;
}

static Expr *comparison() {
    Expr *expr = term();
    while (match(TOKEN_GREATER) ||
           match(TOKEN_GREATER_EQUAL) ||
           match(TOKEN_LESS) || match(TOKEN_LESS_EQUAL)) {
        Token operator= parser.previous;
        Expr *right = term();
        expr = create_binary_expr(operator, expr, right);
    }
    return expr;
}

static Expr *equality() {
    Expr *expr = comparison();
    while (match(TOKEN_BANG_EQUAL) || match(TOKEN_EQUAL_EQUAL)) {
        Token op = parser.previous;
        Expr *right = comparison();
        expr = create_binary_expr(op, expr, right);
    }
    return expr;
}

Expr *parse(const char *source) { 
    initScanner(source);
    parser.hadError = false;
    parser.panicMode = false;
    advance();
    return equality();
}

bool compile(const char *source, Chunk *chunk) {
    initScanner(source);
    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    endCompiler();
    return !parser.hadError;
}
