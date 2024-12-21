//
// Created by dgood on 12/5/24.
//

#ifndef PARSER_H
#define PARSER_H
#include "../ast/ast.h"
#include "../lexer/lexer.h"
#include "../token/token.h"

typedef struct parser_t {
    lexer *               lexer;
    token *               cur_tok;
    token *               peek_tok;
    linked_list *errors;
} parser;

typedef enum {
    LOWEST = 1,
    LOGICAL_AND,
    EQUALS,
    LESSGREATER,
    SUM,
    PRODUCT,
    PREFIX,
    CALL,
    INDEX
} operator_precedence;

typedef ast_expression * (*prefix_parse_fn)(parser *);

typedef ast_expression * (*infix_parse_fn)(parser *, ast_expression *);

parser *parser_init(lexer *);

void parser_next_token(parser *);

ast_program *parse_program(parser *);

ast_statement *parser_parse_statement(parser *);

ast_program *program_init(void);

void program_free(ast_program *);

void parser_free(parser *);

void *create_statement(const parser *, ast_statement_type);

void free_statement(ast_statement *);

void _statement_node(void);

void _expression_node(void);

char *join_parameters_list(linked_list *);

ast_statement *copy_statement(ast_statement *);

ast_expression *copy_expression(ast_expression *);

linked_list *copy_parameters(linked_list *);

void free_expression(void *);
#endif //PARSER_H
