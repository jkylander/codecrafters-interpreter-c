#ifndef PARSE_H
#define PARSE_H
#include "token.h"
typedef struct Parser Parser;
struct Parser {
    Token *tokens;
    TokenList type;
    int current;
};

#endif /* PARSE_H */
