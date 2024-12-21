//
// Created by dgood on 12/2/24.
//

#ifndef LEXER_H
#define LEXER_H
#include <stdlib.h>
#include "../token/token.h"

typedef struct {
    char    *input;
    size_t  current_offset;
    size_t  read_offset;
    char    ch;
} lexer;

lexer*  lexer_init(const char *);
token*  lexer_next_token(lexer *);
void    lexer_free(lexer *);

#endif //LEXER_H
