//
// Created by dgood on 12/2/24.
//

#ifndef TOKEN_H
#define TOKEN_H
/*
 * If you want to add more token types then add it to the
 * token_type enum as well as the tokens array.
 */
typedef enum {
    ILLEGAL,
    END_OF_FILE,

    // identifiers, literals
    IDENT,
    INT,
    STRING,

    //operators
    ASSIGN,
    PLUS,
    MINUS,
    BANG,
    SLASH,
    ASTERISK,
    PERCENT,
    LT,
    GT,
    EQ,
    NOT_EQ,
    AND,
    OR,

    //delimiters
    COMMA,
    SEMICOLON,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    COLON,

    //keywords
    FUNCTION,
    LET,
    IF,
    ELSE,
    RETURN,
    TRUE,
    FALSE,
    WHILE
} token_type;

static const char *tokens[] = {
    "ILLEGAL",
    "END_OF_FILE",

    // identifiers, literals
    "IDENT",
    "INT",
    "STRING",

    //operators
    "ASSIGN",
    "PLUS",
    "MINUS",
    "BANG",
    "SLASH",
    "ASTERISK",
    "PERCENT",
    "LT",
    "GT",
    "EQ",
    "NOT_EQ",
    "AND",
    "OR",

    //delimiters
    "COMMA",
    "SEMICOLON",
    "LPAREN",
    "RPAREN",
    "LBRACE",
    "RBRACE",
    "LBRACKET",
    "RBRACKET",
    "COLON",

    //keywords
    "FUNCTION",
    "LET",
    "IF",
    "ELSE",
    "RETURN",
    "TRUE",
    "FALSE",
    "WHILE"
};

#define token_get_type_from_name(tok) tokens[tok->type]
#define token_get_name_from_type(tok_type) tokens[tok_type]

typedef struct {
    token_type  type;
    char        *literal;
} token;

void        token_free(token *);
token       *token_copy(token *);
token_type  token_get_type(const char *);

#endif //TOKEN_H
