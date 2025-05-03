
#include "compiler.h"
#include "chunk.h"
#include "common.h"
#include "memory.h"
#include "object.h"
#include "scanner.h"
#include "value.h"
#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;
    int depth;
    bool isCaptured;
} Local;

typedef struct {
    uint8_t index;
    bool isLocal;
} Upvalue;

typedef enum {
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_SCRIPT,
} FunctionType;

typedef struct Compiler {
    struct Compiler *enclosing;
    ObjFunction *function;
    FunctionType type;
    int localCount;
    Local locals[UINT8_COUNT];
    Upvalue upvalues[UINT8_COUNT];
    int scopeDepth;
} Compiler;

typedef struct ClassCompiler {
    struct ClassCompiler *enclosing;
    bool hasSuperclass;
} ClassCompiler;

Parser parser;
Compiler *current = nullptr;
ClassCompiler *currentClass = nullptr;
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
            Value *value;
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
    Value *v = malloc(sizeof(Value));

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
        v->type = VAL_OBJ;
        ObjString *s = copyString(token.start + 1, token.length - 2);
        v->as.obj = (Obj *) s;
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
    if (parser.panicMode)
        return;
    if (expr != NULL) {
        if (expr->type == EX_LITERAL) {
            if (expr->as.literal.value->type == VAL_OBJ) {
                Value v = *expr->as.literal.value;
                printf("%s", AS_CSTRING(v));
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
                    printf("%.*f", count_decimals(value), value);
                }
            } else {
                Value v = *expr->as.literal.value;
                printf("%s", AS_CSTRING(v));
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
}

static Chunk *currentChunk() { return &current->function->chunk; }

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

static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX)
        error("Loop body too large.");

    emitByte((offset >> 8) & 0xFF);
    emitByte(offset & 0xFF);
}

static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xFF);
    emitByte(0xFF);
    return currentChunk()->count - 2;
}

static void emitReturn() {
    if (current->type == TYPE_INITIALIZER) {
        emitBytes(OP_GET_LOCAL, 0);
    } else {
        emitByte(OP_NIL);
    }
    emitByte(OP_RETURN);
}

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

static void patchJump(int offset) {
    int jump = currentChunk()->count - offset - 2;
    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xFF;
    currentChunk()->code[offset + 1] = jump & 0xFF;
}

static void initCompiler(Compiler *compiler, FunctionType type) {
    compiler->enclosing = current;
    compiler->function = nullptr;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction();
    current = compiler;
    if (type != TYPE_SCRIPT) {
        current->function->name =
            copyString(parser.previous.start, parser.previous.length);
    }

    Local *local = &current->locals[current->localCount++];
    local->depth = 0;
    local->isCaptured = false;
    if (type != TYPE_FUNCTION) {
        local->name.start = "this";
        local->name.length = 4;

    } else {
        local->name.start = "";
        local->name.length = 0;
    }
}

static ObjFunction *endCompiler() {
    emitReturn();
    ObjFunction *function = current->function;
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), function->name != nullptr
                                             ? function->name->chars
                                             : "<script>");
    }
#endif
    current = current->enclosing;
    return function;
}

static void beginScope() { current->scopeDepth++; }

static void endScope() {
    current->scopeDepth--;
    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth >
               current->scopeDepth) {
        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitByte(OP_POP);
        }
        current->localCount--;
    }
}

static void expression();
static void statement();
static void declaration();
static bool match(TokenType type);
static bool check(TokenType type);
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static uint8_t identifierConstant(Token *name) {
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token *a, Token *b) {
    if (a->length != b->length)
        return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler *compiler, Token *name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }
    return -1;
}

static int addUpvalue(Compiler *compiler, uint8_t index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        Upvalue *upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error("Too many closures in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler *compiler, Token *name) {
    if (compiler->enclosing == nullptr) {
        return -1;
    }
    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t) local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint8_t) upvalue, false);
    }
    return -1;
}

static void addLocal(Token name) {
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }
    Local *local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
}

static void declareVariable() {
    if (current->scopeDepth == 0)
        return;
    Token *name = &parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local *local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error("Already a variable with this name in this scope.");
        }
    }
    addLocal(*name);
}

static uint8_t parseVariable(const char *errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);
    declareVariable();
    if (current->scopeDepth > 0)
        return 0;
    return identifierConstant(&parser.previous);
}

static void markInitialized() {
    if (current->scopeDepth == 0)
        return;
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }
    emitBytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t argumentList() {
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("Can't have more than 255 arguments.");
            }
            argCount++;

        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

static void and_(bool canAssign) {
    (void) canAssign;
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    parsePrecedence(PREC_AND);
    patchJump(endJump);
}

static void binary(bool canAssign) {
    (void) canAssign;
    TokenType operatorType = parser.previous.type;
    ParseRule *rule = getRule(operatorType);
    parsePrecedence((Precedence) (rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL: emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL: emitByte(OP_EQUAL); break;
        case TOKEN_GREATER: emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS: emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS: emitByte(OP_ADD); break;
        case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
        case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
        case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
        default: return;
    }
}

static void call(bool canAssign) {
    (void) canAssign;
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

static void dot(bool canAssign) {
    consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint8_t name = identifierConstant(&parser.previous);

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(OP_SET_PROPERTY, name);
    } else if (match(TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList();
        emitBytes(OP_INVOKE, name);
        emitByte(argCount);
    } else {
        emitBytes(OP_GET_PROPERTY, name);
    }
}

static void literal(bool canAssign) {
    (void) canAssign;
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        default: return;
    }
}

static void expression() { parsePrecedence(PREC_ASSIGNMENT); }

static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();

    ObjFunction *function = endCompiler();
    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));
    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }
}

static void method() {
    consume(TOKEN_IDENTIFIER, "Expect method name.");
    uint8_t constant = identifierConstant(&parser.previous);
    FunctionType type = TYPE_METHOD;
    if (parser.previous.length == 4 &&
        0 == memcmp(parser.previous.start, "init", 4)) {
        type = TYPE_INITIALIZER;
    }
    function(type);
    emitBytes(OP_METHOD, constant);
}

static void variable(bool canAssign);
static void namedVariable(Token name, bool canAssign);

static Token syntheticToken(const char *text) {
    Token token;
    token.start = text;
    token.length = (int)strlen(text);
    return token;
}

static void super(bool canAssign) {
    (void) canAssign;
    if (currentClass == nullptr) {
        error("Can't use 'super' outside of a class.");
    } else if (!currentClass->hasSuperclass) {
        error("Can't use 'super' in a class with no superclass.");
    }

    consume(TOKEN_DOT, "Expect '.' after 'super'.");
    consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
    uint8_t name = identifierConstant(&parser.previous);
    namedVariable(syntheticToken("this"), false);
    if (match(TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList();
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_SUPER_INVOKE, name);
        emitByte(argCount);
    } else {
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_GET_SUPER, name);
    }
}

static void classDeclaration() {
    consume(TOKEN_IDENTIFIER, "Expect class name.");
    Token className = parser.previous;
    uint8_t nameConstant = identifierConstant(&parser.previous);
    declareVariable();

    emitBytes(OP_CLASS, nameConstant);
    defineVariable(nameConstant);

    ClassCompiler classCompiler;
    classCompiler.hasSuperclass = false;
    classCompiler.enclosing = currentClass;
    currentClass = &classCompiler;

    if (match(TOKEN_LESS)) {
        consume(TOKEN_IDENTIFIER, "Expect superclass name.");
        variable(false);

        if (identifiersEqual(&className, &parser.previous)) {
            error("A class can't inherit from itself.");
        }
        beginScope();
        addLocal(syntheticToken("super"));
        defineVariable(0);

        namedVariable(className, false);
        emitByte(OP_INHERIT);
        classCompiler.hasSuperclass = true;
    }

    namedVariable(className, false);
    consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        method();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
    emitByte(OP_POP);

    if (classCompiler.hasSuperclass) {
        endScope();
    }

    currentClass = currentClass->enclosing;
}

static void funDeclaration() {
    uint8_t global = parseVariable("Expect function name.");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}

static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    defineVariable(global);
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void forStatement() {
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TOKEN_SEMICOLON)) {
        // no initializer
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        expressionStatement();
    }

    int loopStart = currentChunk()->count;
    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
    }

    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }

    statement();
    emitLoop(loopStart);
    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP);
    }
    endScope();
}

static void ifStatement() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after if.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();

    int elseJump = emitJump(OP_JUMP);
    patchJump(thenJump);
    emitByte(OP_POP);

    if (match(TOKEN_ELSE))
        statement();
    patchJump(elseJump);
}

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ; after value");
    emitByte(OP_PRINT);
}

static void returnStatement() {
    if (current->type == TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }

    if (match(TOKEN_SEMICOLON)) {
        emitReturn();
    } else {
        if (current->type == TYPE_INITIALIZER) {
            error("Can't return a value from an initializer.");
        }

        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OP_RETURN);
    }
}

static void whileStatement() {
    int loopStart = currentChunk()->count;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    emitLoop(loopStart);
    patchJump(exitJump);
    emitByte(OP_POP);
}

static void synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON)
            return;
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN: return;

            default: // nop
                ;
        }
        advance();
    }
}

static void declaration() {
    if (match(TOKEN_CLASS)) {
        classDeclaration();
    } else if (match(TOKEN_FUN)) {
        funDeclaration();
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }
    if (parser.panicMode)
        synchronize();
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_FOR)) {
        forStatement();
    } else if (match(TOKEN_IF)) {
        ifStatement();
    } else if (match(TOKEN_RETURN)) {
        returnStatement();
    } else if (match(TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

static void grouping(bool canAssign) {
    (void) canAssign;
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign) {
    (void) canAssign;
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void or_(bool canAssign) {
    (void) canAssign;
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);
    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

static void string(bool canAssign) {
    (void) canAssign;
    emitConstant(OBJ_VAL(
        copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t) arg);
    } else {
        emitBytes(getOp, (uint8_t) arg);
    }
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static void this(bool canAssign) {
    (void) canAssign;
    if (currentClass == nullptr) {
        error("Can't use 'this' outside of a class.");
        return;
    }
    variable(false);
}

static void unary(bool canAssign) {
    (void) canAssign;
    TokenType operatorType = parser.previous.type;

    parsePrecedence(PREC_UNARY);

    switch (operatorType) {
        case TOKEN_BANG: emitByte(OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return;
    }
}

// clang-format off
ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping,    call,      PREC_CALL},
  [TOKEN_RIGHT_PAREN]   = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {nullptr,     nullptr,   PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_COMMA]         = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_DOT]           = {nullptr,     dot,       PREC_CALL},
  [TOKEN_MINUS]         = {unary,       binary,    PREC_TERM},
  [TOKEN_PLUS]          = {nullptr,     binary,    PREC_TERM},
  [TOKEN_SEMICOLON]     = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_SLASH]         = {nullptr,     binary,    PREC_FACTOR},
  [TOKEN_STAR]          = {nullptr,     binary,    PREC_FACTOR},
  [TOKEN_BANG]          = {unary,       nullptr,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {nullptr,     binary,    PREC_EQUALITY},
  [TOKEN_EQUAL]         = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {nullptr,     binary,    PREC_EQUALITY},
  [TOKEN_GREATER]       = {nullptr,     binary,    PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {nullptr,     binary,    PREC_COMPARISON},
  [TOKEN_LESS]          = {nullptr,     binary,    PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {nullptr,     binary,    PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable,    nullptr,   PREC_NONE},
  [TOKEN_STRING]        = {string,      nullptr,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,      nullptr,   PREC_NONE},
  [TOKEN_AND]           = {nullptr,     and_,      PREC_AND},
  [TOKEN_CLASS]         = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_ELSE]          = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,     nullptr,   PREC_NONE},
  [TOKEN_FOR]           = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_FUN]           = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_IF]            = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_NIL]           = {literal,     nullptr,   PREC_NONE},
  [TOKEN_OR]            = {nullptr,     or_,       PREC_OR},
  [TOKEN_PRINT]         = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_RETURN]        = {nullptr,     nullptr,   PREC_NONE},
  [TOKEN_SUPER]         = {super,     nullptr,   PREC_NONE},
  [TOKEN_THIS]          = {this,        nullptr,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,     nullptr,   PREC_NONE},
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

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }
    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static ParseRule *getRule(TokenType type) { return &rules[type]; }

static bool check(TokenType type) { return parser.current.type == type; }

static bool match(TokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();
    return true;
}

static Expr *equality();
Expr *parse_expression() { return equality(); }

static Expr *primary() {
    Token current = parser.current;
    if (current.type == TOKEN_EOF) {
        errorAtCurrent("Unexpected end of input.");
        return nullptr;
    }

    if (match(TOKEN_TRUE) || match(TOKEN_FALSE) || match(TOKEN_NIL)) {
        return create_literal_expr(parser.previous);
    }
    if (match(TOKEN_NUMBER) || match(TOKEN_STRING)) {
        return create_literal_expr(parser.previous);
    }

    if (match(TOKEN_LEFT_PAREN)) {
        Expr *expr = parse_expression();
        if (expr == nullptr)
            return nullptr;
        consume(TOKEN_RIGHT_PAREN, "Expect ') after expression");
        return create_grouping_expr(expr);
    }
    errorAtCurrent("Expect expression.");
    return nullptr;
}

static Expr *exp_unary() {
    if (match(TOKEN_BANG) || match(TOKEN_MINUS)) {
        Token operator= parser.previous;
        Expr *right = exp_unary();
        if (right == nullptr)
            return nullptr;
        Expr *r = malloc(sizeof(Expr));
        r->type = EX_UNARY;
        r->as.unary.right = right;
        r->as.unary.operator= operator;
        r->line = right->line;
        return r;
    }
    return primary();
}

static Expr *factor() {
    Expr *expr = exp_unary();
    if (expr == nullptr)
        return nullptr;
    while (match(TOKEN_SLASH) || match(TOKEN_STAR)) {
        Token operator= parser.previous;
        Expr *right = exp_unary();
        expr = create_binary_expr(operator, expr, right);
    }
    return expr;
}

static Expr *term() {
    Expr *expr = factor();
    if (expr == nullptr)
        return nullptr;
    while (match(TOKEN_MINUS) || match(TOKEN_PLUS)) {
        Token operator= parser.previous;
        Expr *right = factor();
        expr = create_binary_expr(operator, expr, right);
    }
    return expr;
}

static Expr *comparison() {
    Expr *expr = term();
    if (expr == nullptr)
        return nullptr;
    while (match(TOKEN_GREATER) || match(TOKEN_GREATER_EQUAL) ||
           match(TOKEN_LESS) || match(TOKEN_LESS_EQUAL)) {
        Token operator= parser.previous;
        Expr *right = term();
        expr = create_binary_expr(operator, expr, right);
    }
    return expr;
}

static Expr *equality() {
    Expr *expr = comparison();
    if (expr == nullptr)
        return nullptr;
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

bool hadError() { return parser.hadError; }

ObjFunction *compile_expression(const char *source) {
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    emitByte(OP_PRINT);
    ObjFunction *function = endCompiler();
    return parser.hadError ? nullptr : function;
}

ObjFunction *compile(const char *source) {
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    while (!match(TOKEN_EOF)) {
        declaration();
    }
    ObjFunction *function = endCompiler();
    return parser.hadError ? nullptr : function;
}

void markCompilerRoots() {
    Compiler *compiler = current;
    while (compiler != nullptr) {
        markObject((Obj *) compiler->function);
        compiler = compiler->enclosing;
    }
}
