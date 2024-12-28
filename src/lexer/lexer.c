//
// Created by dgood on 12/2/24.
//

#include "lexer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include "../token/token.h"

lexer *lexer_init(const char *input) {
    lexer *l = malloc(sizeof(*l));
    assert(l != NULL);

    l->input = strdup(input);
    assert(l->input != NULL);

    l->current_offset = 0;
    l->read_offset    = 1;
    l->ch             = input[0];

    return l;
}

#define is_character(c) isalnum(c) || c == '_'

static char *read_identifier(lexer *l) {
    const size_t position = l->current_offset;

    while (is_character(l->input[l->current_offset])) {
        l->current_offset++;
    }
    const size_t numCharacters = l->current_offset - position;

    char *identifier = malloc(numCharacters + 1);
    assert(identifier != NULL);

    memcpy(identifier, l->input + position, numCharacters);
    identifier[numCharacters] = '\0';

    l->read_offset = l->current_offset + 1;
    l->ch          = l->input[l->current_offset];

    return identifier;
}

static void read_char(lexer *l) {
    if (l->ch) {
        l->current_offset = l->read_offset;
        l->read_offset++;
        l->ch = l->input[l->current_offset];
    }
}

static void skip_whitespace(lexer *l) {
    while (l->ch && (l->ch == ' ' || l->ch == '\n' || l->ch == '\r' || l->ch == '\t'))
        read_char(l);
}

static char *read_string(lexer *l) {
    const size_t position = l->current_offset + 1;
    l->current_offset++;

    while (l->input[l->current_offset] != '"' && l->input[l->current_offset] != 0) {
        l->current_offset++;
    }

    const size_t length = l->current_offset - position;
    char *       string = malloc(length + 1);
    assert(string != NULL);

    memcpy(string, l->input + position, length);
    string[length] = '\0';

    l->current_offset++;
    l->read_offset = l->current_offset + 1;
    l->ch          = l->input[l->current_offset];

    return string;
}

token *lexer_next_token(lexer *l) {
    token *t = malloc(sizeof(token));
    assert(t);

    //skip_comment(l);
    skip_whitespace(l);

    switch (l->ch) {
        case '=':
            if (l->input[l->read_offset] == '=') {
                t->literal = strdup("==");
                t->type    = EQ;
                read_char(l);
                read_char(l);
                break;
            }
            t->literal = strdup("=");
            t->type    = ASSIGN;
            read_char(l);
            break;
        case '+':
            t->literal = strdup("+");
            t->type = PLUS;
            read_char(l);
            break;
        case ',':
            t->literal = strdup(",");
            t->type = COMMA;
            read_char(l);
            break;
        case ';':
            t->literal = strdup(";");
            t->type = SEMICOLON;
            read_char(l);
            break;
        case '(':
            t->literal = strdup("(");
            t->type = LPAREN;
            read_char(l);
            break;
        case ')':
            t->literal = strdup(")");
            t->type = RPAREN;
            read_char(l);
            break;
        case '{':
            t->literal = strdup("{");
            t->type = LBRACE;
            read_char(l);
            break;
        case '}':
            t->literal = strdup("}");
            t->type = RBRACE;
            read_char(l);
            break;
        case '!':
            if (l->input[l->read_offset] == '=') {
                t->literal = strdup("!=");
                t->type    = NOT_EQ;
                read_char(l);
                read_char(l);
                break;
            }
            t->literal = strdup("!");
            t->type    = BANG;
            read_char(l);
            break;
        case '-':
            t->literal = strdup("-");
            t->type = MINUS;
            read_char(l);
            break;
        case '/':
            t->literal = strdup("/");
            t->type = SLASH;
            read_char(l);
            break;
        case '*':
            t->literal = strdup("*");
            t->type = ASTERISK;
            read_char(l);
            break;
        case '<':
            t->literal = strdup("<");
            t->type = LT;
            read_char(l);
            break;
        case '>':
            t->literal = strdup(">");
            t->type = GT;
            read_char(l);
            break;
        case 0:
            t->literal = "";
            t->type = END_OF_FILE;
            break;
        case '"':
            t->literal = read_string(l);
            t->type = STRING;
            break;
        case '[':
            t->literal = strdup("[");
            t->type = LBRACKET;
            read_char(l);
            break;
        case ']':
            t->literal = strdup("]");
            t->type = RBRACKET;
            read_char(l);
            break;
        case ':':
            t->literal = strdup(":");
            t->type = COLON;
            read_char(l);
            break;
        case '&':
            if (l->input[l->read_offset] == '&') {
                t->literal = strdup("&&");
                t->type    = AND;
                read_char(l);
                read_char(l);
            } else {
                t->literal = nullptr;
                t->type    = ILLEGAL;
                read_char(l);
            }
            break;
        case '|':
            if (l->input[l->read_offset] == '|') {
                t->literal = strdup("||");
                t->type    = OR;
                read_char(l);
                read_char(l);
            } else {
                t->literal = nullptr;
                t->type    = ILLEGAL;
                read_char(l);
            }
            break;
        case '%':
            t->literal = strdup("%");
            t->type = PERCENT;
            read_char(l);
            break;
        default:
            if (is_character(l->ch)) {
                t->literal = read_identifier(l);
                t->type    = token_get_type(t->literal);
            } else {
                t->literal = nullptr;
                t->type    = ILLEGAL;
                read_char(l);
            }
    }

    return t;
}

void lexer_free(lexer *l) {
    free(l->input);
    free(l);
}
