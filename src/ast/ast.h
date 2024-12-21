//
// Created by dgood on 12/3/24.
//

#ifndef AST_H
#define AST_H
#include <stdio.h>

#include "../datastructures/arraylist.h"
#include "../datastructures/hashmap.h"
#include "../token/token.h"
#include <stdbool.h>

typedef enum {
    STATEMENT,
    EXPRESSION,
    PROGRAM
} ast_node_type;

typedef enum {
    LET_STATEMENT,
    RETURN_STATEMENT,
    EXPRESSION_STATEMENT,
    BLOCK_STATEMENT
} ast_statement_type;

static const char *statement_type_values[] = {
        "LET_STATEMENT",
        "RETURN_STATEMENT",
        "EXPRESSION_STATEMENT",
        "BLOCK_STATEMENT"
};

typedef enum {
    IDENTIFIER_EXPRESSION,
    INTEGER_EXPRESSION,
    STRING_EXPRESSION,
    PREFIX_EXPRESSION,
    INFIX_EXPRESSION,
    BOOLEAN_EXPRESSION,
    IF_EXPRESSION,
    FUNCTION_LITERAL,
    CALL_EXPRESSION,
    ARRAY_LITERAL,
    INDEX_EXPRESSION,
    HASH_LITERAL,
    WHILE_EXPRESSION
} expression_type;

static const char *expression_type_values[] = {
        "IDENTIFIER_EXPRESSION",
        "INTEGER_EXPRESSION",
        "STRING_EXPRESSION",
        "PREFIX_EXPRESSION",
        "INFIX_EXPRESSION",
        "BOOLEAN_EXPRESSION",
        "IF_EXPRESSION",
        "FUNCTION_LITERAL",
        "CALL_EXPRESSION",
        "ARRAY_LITERAL",
        "INDEX_EXPRESSION",
        "HASH_LITERAL",
        "WHILE_EXPRESSION"
};

typedef struct {
    ast_node_type type;

    char *(*token_literal)(void *);

    char *(*string)(void *);
} ast_node;

typedef struct {
    ast_node           node;
    ast_statement_type statement_type;
} ast_statement;

typedef struct {
    ast_node        node;
    expression_type expression_type;
} ast_expression;

typedef struct {
    ast_node        node;
    ast_statement **statements;
    size_t          statement_count;
    size_t          array_size;
} ast_program;

typedef struct {
    ast_expression expression;
    token *        token;
    char *         value;
} ast_identifier;

typedef struct {
    ast_expression expression;
    token *        token;
    long           value;
} ast_integer;

typedef struct {
    ast_expression expression;
    token *        token;
    char *         value;
    size_t         length;
} ast_string;

typedef struct {
    ast_expression  expression;
    token *         token;
    ast_expression *right;
    char *          operator;
} ast_prefix_expression;

typedef struct {
    ast_expression  expression;
    token *         token;
    ast_expression *left;
    ast_expression *right;
    char *          operator;
} ast_infix_expression;

typedef struct {
    ast_statement   statement;
    token *         token;
    ast_identifier *name;
    ast_expression *value;
} ast_let_statement;

typedef struct {
    ast_statement   statement;
    token *         token;
    ast_expression *return_value;
} ast_return_statement;

typedef struct {
    ast_statement   statement;
    token *         token;
    ast_expression *expression;
} ast_expression_statement;

typedef struct {
    ast_statement   statement;
    token *         token;
    ast_statement **statements;
    size_t          statement_count;
    size_t          array_size;
} ast_block_statement;

typedef struct {
    ast_expression expression;
    token *        token;
    bool           value;
} ast_boolean_expression;

typedef struct {
    ast_expression       expression;
    token *              token;
    ast_expression *     condition;
    ast_block_statement *consequence;
    ast_block_statement *alternative;
} ast_if_expression;

typedef struct {
    ast_expression       expression;
    token *              token;
    ast_expression *     condition;
    ast_block_statement *body;
} ast_while_expression;

typedef struct {
    ast_expression        expression;
    token *               token;
    char *                name;
    linked_list *parameters;
    ast_block_statement * body;
} ast_function_literal;

typedef struct {
    ast_expression  expression;
    token          *token;
    ast_expression *function;
    linked_list    *arguments;
} ast_call_expression;

typedef struct {
    ast_expression expression;
    token         *token;
    arraylist     *elements;
} ast_array_literal;

typedef struct {
    ast_expression  expression;
    token *         token;
    ast_expression *left;
    ast_expression *index;
} ast_index_expression;

typedef struct {
    ast_expression expression;
    token *        token;
    hashtable *    pairs;
} ast_hash_literal;

#define ast_get_statement_type_name(type) statement_type_values[type]
#define ast_get_expression_type_name(type) expression_type_values[type]

#endif //AST_H
