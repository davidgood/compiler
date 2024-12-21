//
// Created by dgood on 12/2/24.
//

#include "token.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

void token_free(token *tok) {
    if (!tok) {
        return;
    }
    if (tok->type != END_OF_FILE) {
        if (tok->literal) {
            free(tok->literal);
        }
    }
    free(tok);
}

static int is_number(const char *literal) {
    while (*literal) {
        char c = *literal;
        if (!c) {
            break;
        }

        if (!isdigit(c))
            return false;
        literal++;
    }
    return true;
}

token_type token_get_type(const char *literal) {
    if (strcmp(literal, "let") == 0)
        return LET;

    if (strcmp(literal, "fn") == 0)
        return FUNCTION;

    if (strcmp(literal, "if") == 0)
        return IF;

    if (strcmp(literal, "else") == 0)
        return ELSE;

    if (strcmp(literal, "return") == 0)
        return RETURN;

    if (strcmp(literal, "true") == 0)
        return TRUE;

    if (strcmp(literal, "false") == 0)
        return FALSE;

    if (strcmp(literal, "while") == 0)
        return WHILE;

    if (is_number(literal))
        return INT;

    return IDENT;
}

token *token_copy(token *source) {
    token *destination = malloc(sizeof(*destination));
    if (destination == NULL) {
        return NULL;
    }

    destination->type    = source->type;
    destination->literal = strdup(source->literal);
    if (destination->literal == NULL) {
        free(destination);
        return NULL;
    }

    return destination;
}
