//
// Created by dgood on 12/5/24.
//

// #define TRACE

#include "parser.h"

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../ast/ast.h"
#include "../datastructures/conversions.h"
#include "../lexer/lexer.h"
#include <stdbool.h>

static ast_expression *parse_identifier_expression(parser *);

static ast_expression *parse_integer_expression(parser *);

static ast_expression *parse_string_expression(parser *);

static ast_expression *parse_prefix_expression(parser *);

static ast_expression *parse_boolean_expression(parser *);

static ast_expression *parse_grouped_expression(parser *);

static ast_expression *parse_if_expression(parser *);

static ast_expression *parse_function_literal(parser *);

static ast_expression *parse_array_literal(parser *);

static ast_expression *parse_hash_literal(parser *);

static ast_expression *parse_while_expression(parser *);

static ast_expression *parse_infix_expression(parser *, ast_expression *);

static ast_expression *parse_call_expression(parser *, ast_expression *);

static ast_expression *parse_index_expression(parser *, ast_expression *);

/**
 *
 */
static prefix_parse_fn prefix_fns[] = {
        NULL,                        // ILLEGAL
        NULL,                        // END OF FILE
        parse_identifier_expression, // IDENT
        parse_integer_expression,    // INT
        parse_string_expression,     // STRING
        NULL,                        // ASSIGN
        NULL,                        // PLUS
        parse_prefix_expression,     // MINUS
        parse_prefix_expression,     // BANG
        NULL,                        // SLASH
        NULL,                        // ASTERISK
        NULL,                        // PERCENT
        NULL,                        // LT
        NULL,                        // GT
        NULL,                        // EQ
        NULL,                        // NOT_EQ
        NULL,                        // AND
        NULL,                        // OR
        NULL,                        // COMMA
        NULL,                        // SEMICOLON
        parse_grouped_expression,    // LPAREN
        NULL,                        // RPAREN
        parse_hash_literal,          // LBRACE
        NULL,                        // RBRACE
        parse_array_literal,         // LBRACKET
        NULL,                        // RBRACKET
        NULL,                        // COLON
        parse_function_literal,      // FUNCTION
        NULL,                        // LET
        parse_if_expression,         // IF
        NULL,                        // ELSE
        NULL,                        // RETURN
        parse_boolean_expression,    // TRUE
        parse_boolean_expression,    // FALSE
        parse_while_expression       // WHILE
};

static infix_parse_fn infix_fns[] = {
        NULL,                   // ILLEGAL
        NULL,                   // END OF FILE
        NULL,                   // IDENT
        NULL,                   // INT
        NULL,                   // STRING
        NULL,                   // ASSIGN
        parse_infix_expression, // PLUS
        parse_infix_expression, // MINUS
        NULL,                   // BANG
        parse_infix_expression, // SLASH
        parse_infix_expression, // ASTERISK
        parse_infix_expression, // PERCENT
        parse_infix_expression, // LT
        parse_infix_expression, // GT
        parse_infix_expression, // EQ
        parse_infix_expression, // NOT_EQ
        parse_infix_expression, // AND
        parse_infix_expression, // OR
        NULL,                   // COMMA
        NULL,                   // SEMICOLON
        parse_call_expression,  // LPAREN
        NULL,                   // RPAREN
        NULL,                   // LBRACE
        NULL,                   // RBRACE
        parse_index_expression, // LBRACKET
        NULL,                   // RBRACKET
        NULL,                   // COLON
        NULL,                   // FUNCTION
        NULL,                   // LET
        NULL,                   // IF
        NULL,                   // ELSE
        NULL,                   // RETURN
        NULL,                   // TRUE
        NULL,                   // FALSE
        NULL                    // WHILE
};

/*
************************************************************************
*                FREE FUNCTIONS
************************************************************************
* */

static void free_identifier(void *id) {
    ast_identifier *ident = (ast_identifier *) id;
    if (ident->token) {
        token_free(ident->token);
        ident->token = NULL;
    }
    if (ident->value) {
        free(ident->value);
        ident->value = NULL;
    }
    free(ident);
}

static void free_integer_expression(ast_integer *int_exp) {
    if (int_exp->token) {
        token_free(int_exp->token);
        int_exp->token = NULL;
    }
    free(int_exp);
}

static void free_prefix_expression(ast_prefix_expression *prefix_exp) {
    if (prefix_exp->token) {
        token_free(prefix_exp->token);
        prefix_exp->token = NULL;
    }
    if (prefix_exp->operator) {
        free(prefix_exp->operator);
        prefix_exp->operator = NULL;
    }
    if (prefix_exp->right) {
        free_expression(prefix_exp->right);
        prefix_exp->right = NULL;
    }
    free(prefix_exp);
}

static void free_infix_expression(ast_infix_expression *infix_exp) {
    if (infix_exp->operator) {
        free(infix_exp->operator);
        infix_exp->operator = NULL;
    }
    if (infix_exp->token) {
        token_free(infix_exp->token);
        infix_exp->token = NULL;
    }
    if (infix_exp->left) {
        free_expression(infix_exp->left);
        infix_exp->left = NULL;
    }
    if (infix_exp->right) {
        free_expression(infix_exp->right);
        infix_exp->right = NULL;
    }
    free(infix_exp);
}

static void free_boolean_expression(ast_boolean_expression *bool_exp) {
    token_free(bool_exp->token);
    free(bool_exp);
}

static void free_string(ast_string *string) {
    free(string->value);
    token_free(string->token);
    free(string);
}

static void free_return_statement(ast_return_statement *ret_stmt) {
    if (ret_stmt->token) {
        token_free(ret_stmt->token);
    }
    if (ret_stmt->return_value) {
        free_expression(ret_stmt->return_value);
    }
    free(ret_stmt);
}


static void free_letstatement(ast_let_statement *let_stmt) {
    if (let_stmt->name) {
        free_identifier(let_stmt->name); // Free the variable name
    }
    if (let_stmt->token) {
        token_free(let_stmt->token); // Free the token
    }
    if (let_stmt->value) {
        free_expression(let_stmt->value); // Free the value expression
    }
    free(let_stmt);
}

static void free_block_statement(ast_block_statement *block_stmt) {
    for (size_t i = 0; i < block_stmt->statement_count; i++) {
        free_statement(block_stmt->statements[i]);
    }
    free(block_stmt->statements);
    token_free(block_stmt->token);
    free(block_stmt);
}

static void free_while_expression(ast_while_expression *while_exp) {
    if (while_exp->token) {
        token_free(while_exp->token);
    }
    if (while_exp->condition) {
        free_expression(while_exp->condition);
    }
    if (while_exp->body) {
        free_block_statement(while_exp->body);
    }
    free(while_exp);
}

static void free_if_expression(ast_if_expression *if_exp) {
    token_free(if_exp->token);
    if (if_exp->condition) {
        free_expression(if_exp->condition);
    }
    if (if_exp->consequence) {
        free_statement((ast_statement *) if_exp->consequence);
    }
    if (if_exp->alternative) {
        free_statement((ast_statement *) if_exp->alternative);
    }
    free(if_exp);
}


static void free_function_literal(ast_function_literal *function) {
    if (function->token) {
        token_free(function->token);
        function->token = NULL;
    }
    if (function->parameters) {
        linked_list_free(function->parameters, free_identifier);
        function->parameters = NULL;
    }
    if (function->body) {
        free_block_statement(function->body);
        function->body = NULL;
    }
    free(function);
}

static void free_hash_literal(ast_hash_literal *hash_exp) {
    if (hash_exp->token) {
        token_free(hash_exp->token);
    }
    if (hash_exp->pairs) {
        hashtable_destroy(hash_exp->pairs);
    }
    free(hash_exp);
}

static void free_call_expression(ast_call_expression *call_exp) {
    if (call_exp->token) {
        token_free(call_exp->token);
        call_exp->token = NULL;
    }
    if (call_exp->function) {
        free_expression(call_exp->function);
        call_exp->function = NULL;
    }
    if (call_exp->arguments) {
        linked_list_free(call_exp->arguments, free_expression);
        call_exp->arguments = NULL;
    }
    free(call_exp);
}

static void free_array_literal(ast_array_literal *array) {
    if (array->token) {
        token_free(array->token);
    }
    if (array->elements) {
        arraylist_destroy(array->elements, free_expression);
    }
    free(array);
}

static void free_index_expression(ast_index_expression *index_exp) {
    if (index_exp->token) {
        token_free(index_exp->token);
    }
    if (index_exp->left) {
        free_expression(index_exp->left);
    }
    if (index_exp->index) {
        free_expression(index_exp->index);
    }
    free(index_exp);
}

void free_expression(void *e) {
    ast_expression *exp = e;
    if (!exp)
        return;

    switch (exp->expression_type) {
        case IDENTIFIER_EXPRESSION:
            free_identifier((ast_identifier *) exp);
            break;
        case INTEGER_EXPRESSION:
            free_integer_expression((ast_integer *) exp);
            break;
        case PREFIX_EXPRESSION:
            free_prefix_expression((ast_prefix_expression *) exp);
            break;
        case INFIX_EXPRESSION:
            free_infix_expression((ast_infix_expression *) exp);
            break;
        case BOOLEAN_EXPRESSION:
            free_boolean_expression((ast_boolean_expression *) exp);
            break;
        case IF_EXPRESSION:
            free_if_expression((ast_if_expression *) exp);
            break;
        case FUNCTION_LITERAL:
            free_function_literal((ast_function_literal *) exp);
            break;
        case CALL_EXPRESSION:
            free_call_expression((ast_call_expression *) exp);
            break;
        case STRING_EXPRESSION:
            free_string((ast_string *) exp);
            break;
        case ARRAY_LITERAL:
            free_array_literal((ast_array_literal *) exp);
            break;
        case INDEX_EXPRESSION:
            free_index_expression((ast_index_expression *) exp);
            break;
        case HASH_LITERAL:
            free_hash_literal((ast_hash_literal *) exp);
            break;
        case WHILE_EXPRESSION:
            free_while_expression((ast_while_expression *) exp);
            break;
        default:
            free(exp);
            break;
    }
}

static void free_expression_statement(ast_expression_statement *exp_stmt) {
    if (exp_stmt->token) {
        token_free(exp_stmt->token);
    }
    if (exp_stmt->expression) {
        free_expression(exp_stmt->expression);
    }
    free(exp_stmt);
}

void free_statement(ast_statement *stmt) {
    if (!stmt)
        return;

    switch (stmt->statement_type) {
        case LET_STATEMENT:
            free_letstatement((ast_let_statement *) stmt);
            break;
        case RETURN_STATEMENT:
            free_return_statement((ast_return_statement *) stmt);
            break;
        case EXPRESSION_STATEMENT:
            free_expression_statement((ast_expression_statement *) stmt);
            break;
        case BLOCK_STATEMENT:
            free_block_statement((ast_block_statement *) stmt);
            break;
        default:
            free(stmt);
            break;
    }
}


void parser_free(parser *parser) {
    if (parser == NULL)
        return;

    if (parser->cur_tok) {
        token_free(parser->cur_tok);
    }
    if (parser->peek_tok) {
        token_free(parser->peek_tok);
    }
    if (parser->lexer) {
        lexer_free(parser->lexer);
    }
    // Free error list
    if (parser->errors) {
        linked_list_free(parser->errors, free); // Assuming errors are strings
    }

    // Free the parser itself
    free(parser);
}

void program_free(ast_program *program) {
    if (program == NULL)
        return;

    for (int i = 0; i < program->statement_count; i++) {
        free_statement(program->statements[i]);
    }

    free(program->statements);
    free(program);
}


static void add_parse_error(parser *parser, char *errmsg) {
    if (parser->errors == NULL) {
        parser->errors = linked_list_create();
        if (parser->errors == NULL) {
            err(EXIT_FAILURE, "malloc failed");
        }
    }
    linked_list_addNode(parser->errors, errmsg);
}

static void handle_no_prefix_fn(parser *parser) {
    char *msg = NULL;
    asprintf(&msg, "no prefix parse function for the token \"%s\"", parser->cur_tok->literal);
    if (msg == NULL)
        err(EXIT_FAILURE, "malloc failed");
    add_parse_error(parser, msg);
}

static operator_precedence precedence(token_type tok_type) {
    switch (tok_type) {
        case EQ:
        case NOT_EQ:
            return EQUALS;
        case LT:
        case GT:
            return LESSGREATER;
        case PLUS:
        case MINUS:
            return SUM;
        case SLASH:
        case ASTERISK:
        case PERCENT:
            return PRODUCT;
        case LPAREN:
            return CALL;
        case LBRACKET:
            return INDEX;
        case AND:
        case OR:
            return LOGICAL_AND;
        default:
            return LOWEST;
    }
}

static operator_precedence peek_precedence(const parser *parser) { return precedence(parser->peek_tok->type); }

static operator_precedence cur_precedence(const parser *parser) { return precedence(parser->cur_tok->type); }

static char *program_token_literal(void *prog_obj) {
    ast_program *program = prog_obj;
    if (program == NULL) {
        return "";
    }

    ast_node head_node = program->statements[0]->node;
    return head_node.token_literal(&head_node);
}

static char *hash_literal_token_literal(void *exp) {
    ast_hash_literal *hash_exp = (ast_hash_literal *) exp;
    return hash_exp->token->literal;
}

static char *let_statement_token_literal(void *stmt) {
    ast_let_statement *ls = stmt;
    return ls->token->literal;
}

static char *return_statement_token_literal(void *stmt) {
    ast_return_statement *ret_stmt = stmt;
    return ret_stmt->token->literal;
}

static char *string_token_literal(void *exp) {
    ast_string *string = exp;
    return string->token->literal;
}

static char *expression_statement_token_literal(void *stmt) {
    ast_expression_statement *exp_stmt = stmt;
    return exp_stmt->token->literal;
}

static char *array_literal_token_literal(void *exp) {
    ast_array_literal *array = exp;
    return array->token->literal;
}

static char *block_statement_token_literal(void *stmt) {
    ast_block_statement *block_stmt = stmt;
    return block_stmt->token->literal;
}

static char *prefix_expression_token_literal(void *exp) {
    ast_prefix_expression *prefix_exp = exp;
    return prefix_exp->token->literal;
}

static char *infix_expression_token_literal(void *exp) {
    ast_infix_expression *infix_exp = exp;
    return infix_exp->token->literal;
}

static char *boolean_expression_token_literal(void *exp) {
    ast_boolean_expression *bool_exp = (ast_boolean_expression *) exp;
    return bool_exp->token->literal;
}

static char *if_expression_token_literal(void *exp) {
    ast_if_expression *if_exp = (ast_if_expression *) exp;
    return if_exp->token->literal;
}

static char *string_string(void *exp) {
    ast_string *string = exp;
    return strdup(string->value);
}

static char *let_statement_string(void *stmt) {
    ast_let_statement *let_stmt        = stmt;
    char *             let_stmt_string = NULL;
    char *             ident_string    = let_stmt->name->expression.node.string(let_stmt->name);
    char *             value_string    = let_stmt->value ? let_stmt->value->node.string(let_stmt->value) : strdup("");
    char *             let_string      = strdup(let_stmt->token->literal);
    asprintf(&let_stmt_string, "%s %s = %s;", let_string, ident_string, value_string);
    free(ident_string);
    free(value_string);
    free(let_string);
    if (let_stmt_string == NULL)
        err(EXIT_FAILURE, "malloc failed");
    return let_stmt_string;
}

static char *return_statement_string(void *stmt) {
    ast_return_statement *ret_stmt        = stmt;
    char *                ret_stmt_string = NULL;
    char *                value_string    =
            ret_stmt->return_value ? ret_stmt->return_value->node.string(ret_stmt->return_value) : strdup("");
    asprintf(&ret_stmt_string, "%s %s;", ret_stmt->statement.node.string(ret_stmt), value_string);
    if (ret_stmt_string == NULL)
        err(EXIT_FAILURE, "malloc failed");
    return ret_stmt_string;
}

static char *identifier_string(void *id) {
    ast_identifier *ident        = id;
    char *          ident_string = strdup(ident->value);
    if (ident_string == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    return ident_string;
}

static char *integer_string(void *node) {
    ast_integer *int_exp = (ast_integer *) node;
    return long_to_string(int_exp->value);
}

static char *prefix_expression_string(void *node) {
    ast_prefix_expression *prefix_exp     = (ast_prefix_expression *) node;
    char *                 str            = NULL;
    char *                 operand_string = prefix_exp->right->node.string(prefix_exp->right);
    asprintf(&str, "(%s%s)", prefix_exp->operator, operand_string);
    free(operand_string);
    if (str == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    return str;
}

static char *infix_expression_string(void *node) {
    ast_infix_expression *infix_exp    = node;
    char *                str          = NULL;
    char *                left_string  = infix_exp->left->node.string(infix_exp->left);
    char *                right_string = infix_exp->right->node.string(infix_exp->right);
    asprintf(&str, "(%s %s %s)", left_string, infix_exp->operator, right_string);
    free(left_string);
    free(right_string);
    if (str == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    return str;
}

static char *expression_statement_string(void *stmt) {
    ast_expression_statement *exp_stmt = stmt;
    if (exp_stmt->expression) {
        return (exp_stmt->expression->node.string(exp_stmt->expression));
    }
    return strdup("");
}

static char *block_statement_string(void *stmt) {
    ast_block_statement *block_stmt = stmt;
    char *               string     = NULL;
    for (size_t i = 0; i < block_stmt->statement_count; i++) {
        ast_statement *s           = block_stmt->statements[i];
        char *         stmt_string = s->node.string(s);
        if (string == NULL)
            string = stmt_string;
        else {
            char *temp = NULL;
            asprintf(&temp, "%s %s", string, stmt_string);
            if (temp == NULL) {
                free(stmt_string);
                free(string);
                err(EXIT_FAILURE, "malloc failed");
            }
            free(string);
            free(stmt_string);
            string = temp;
        }
    }
    return string;
}

static char *boolean_expression_string(void *exp) {
    // TODO: error check for strdup needed
    ast_boolean_expression *bool_exp = exp;
    if (bool_exp->value) {
        return strdup("true");
    }
    return strdup("false");
}

static char *hash_literal_string(void *exp) {
    ast_hash_literal *hash_exp = exp;
    char *            string   = NULL;
    char *            temp     = NULL;
    int               ret;
    for (size_t i = 0; i < hash_exp->pairs->table_size; i++) {
        void *obj = hash_exp->pairs->table[i];
        if (obj == NULL)
            continue;
        hashtable_entry *entry       = (hashtable_entry *) obj;
        ast_expression * keyexp      = entry->key;
        ast_expression * valuexp     = entry->value;
        char *           keystring   = keyexp->node.string(keyexp);
        char *           valuestring = valuexp->node.string(valuexp);
        if (string == NULL) {
            ret = asprintf(&temp, "%s:%s", keystring, valuestring);
        } else {
            ret = asprintf(&temp, "%s, %s:%s", string, keystring, valuestring);
            free(string);
        }
        free(keystring);
        free(valuestring);
        if (ret == -1)
            err(EXIT_FAILURE, "malloc failed");
        string = temp;
        temp   = NULL;
    }
    ret = asprintf(&temp, "{%s}", string);
    free(string);
    if (ret == -1)
        err(EXIT_FAILURE, "malloc failed");
    return temp;
}

static char *while_expression_token_literal(void *exp) {
    ast_while_expression *while_exp = (ast_while_expression *) exp;
    return while_exp->token->literal;
}

static char *while_expression_string(void *exp) {
    ast_while_expression *while_exp        = (ast_while_expression *) exp;
    char *                string           = NULL;
    char *                condition_string = while_exp->condition->node.string(while_exp->condition);
    char *                body_string      = while_exp->body->statement.node.string(while_exp->body);
    int                   ret              = asprintf(&string, "while%s %s", condition_string, body_string);
    free(condition_string);
    free(body_string);
    if (ret == -1)
        err(EXIT_FAILURE, "malloc failed");
    return string;
}

static char *if_expression_string(void *exp) {
    ast_if_expression *if_exp             = (ast_if_expression *) exp;
    char *             string             = NULL;
    char *             condition_string   = if_exp->condition->node.string(if_exp->condition);
    char *             consequence_string = if_exp->consequence->statement.node.string(if_exp->consequence);
    if (if_exp->alternative != NULL) {
        char *alternative_string = if_exp->alternative->statement.node.string(if_exp->alternative);
        asprintf(&string, "if%s %s else %s", condition_string, consequence_string, alternative_string);
        free(alternative_string);
    } else {
        asprintf(&string, "if%s %s", condition_string, consequence_string);
    }

    free(condition_string);
    free(consequence_string);

    if (string == NULL)
        err(EXIT_FAILURE, "malloc failed");
    return string;
}

static char *array_literal_string(void *exp) {
    ast_array_literal *array  = (ast_array_literal *) exp;
    char *             string = NULL;
    char *             temp   = NULL;
    int                ret;
    for (size_t i = 0; i < array->elements->size; i++) {
        ast_expression *element        = (ast_expression *) arraylist_get(array->elements, i);
        char *          element_string = element->node.string(element);
        if (string == NULL)
            ret = asprintf(&temp, "%s", element_string);
        else {
            ret = asprintf(&temp, "%s, %s", string, element_string);
            free(string);
        }
        free(element_string);
        if (ret == -1)
            err(EXIT_FAILURE, "malloc failed");
        string = temp;
        temp   = NULL;
    }
    ret = asprintf(&temp, "[%s]", string);
    if (ret == -1)
        err(EXIT_FAILURE, "malloc failed");
    free(string);
    return temp;
}

static char *index_exp_string(void *exp) {
    ast_index_expression *index_exp    = (ast_index_expression *) exp;
    char *                string       = NULL;
    char *                left_string  = index_exp->left->node.string(index_exp->left);
    char *                index_string = index_exp->index->node.string(index_exp->index);
    int                   ret          = asprintf(&string, "(%s[%s])", left_string, index_string);
    free(left_string);
    free(index_string);
    if (ret == -1) {
        err(EXIT_FAILURE, "malloc failed");
    }
    return string;
}

char *join_parameters_list(linked_list *parameters_list) {
    char *string = NULL;
    char *temp   = NULL;
    if (parameters_list == NULL || parameters_list->size == 0)
        return strdup("");

    list_node *list_node = parameters_list->head;
    while (list_node != NULL) {
        ast_identifier *param        = (ast_identifier *) list_node->data;
        char *          param_string = param->expression.node.string(param);
        if (string == NULL) {
            asprintf(&temp, "%s", param_string);
            free(param_string);
            if (temp == NULL)
                err(EXIT_FAILURE, "malloc failed");
            string = temp;
        } else {
            asprintf(&temp, "%s, %s", string, param_string);
            free(param_string);
            free(string);
            if (temp == NULL)
                err(EXIT_FAILURE, "malloc failed");
            string = temp;
        }
        list_node = list_node->next;
    }
    return string;
}

static char *function_literal_string(void *exp) {
    ast_function_literal *func               = (ast_function_literal *) exp;
    char *                params_string      = join_parameters_list(func->parameters);
    char *                func_string        = NULL;
    char *                func_token_literal = func->expression.node.token_literal(func);
    char *                body_string        = func->body->statement.node.string(func->body);
    if (func->name != NULL) {
        asprintf(&func_string, "%s(%s) %s", func_token_literal, params_string, body_string);
    } else {
        asprintf(&func_string, "%s<%s>(%s) %s", func_token_literal, func->name, params_string, body_string);
    }
    free(params_string);
    free(body_string);
    if (func_string == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    return func_string;
}

static char *function_literal_token_literal(void *exp) {
    ast_function_literal *func = (ast_function_literal *) exp;
    return func->token->literal;
}

static char *call_expression_string(void *exp) {
    ast_call_expression *call_exp        = (ast_call_expression *) exp;
    char *               args_string     = join_parameters_list(call_exp->arguments);
    char *               function_string = call_exp->function->node.string(call_exp->function);
    char *               string          = NULL;
    asprintf(&string, "%s(%s)", function_string, args_string);
    free(function_string);
    free(args_string);
    if (string == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    return string;
}

static char *call_expression_token_literal(void *exp) {
    ast_call_expression *call_exp = (ast_call_expression *) exp;
    return call_exp->token->literal;
}

static char *program_string(void *prog_ptr) {
    // TODO: maybe we could optimize this
    ast_program *program     = (ast_program *) prog_ptr;
    char *       prog_string = NULL;
    char *       temp_string = NULL;
    for (int i = 0; i < program->statement_count; i++) {
        ast_statement *stmt        = program->statements[i];
        char *         stmt_string = stmt->node.string(stmt);
        if (prog_string != NULL) {
            asprintf(&temp_string, "%s %s", prog_string, stmt_string);
            free(prog_string);
        } else {
            asprintf(&temp_string, "%s", stmt_string);
        }
        free(stmt_string);
        if (temp_string == NULL) {
            if (prog_string != NULL) {
                free(prog_string);
            }
            err(EXIT_FAILURE, "malloc failed");
        }
        prog_string = temp_string;
        temp_string = NULL;
    }
    return prog_string;
}

static ast_let_statement *create_let_statement(parser *parser) {
    ast_let_statement *let_stmt = malloc(sizeof(*let_stmt));
    if (let_stmt == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    let_stmt->token = token_copy(parser->cur_tok);
    if (let_stmt->token == NULL) {
        free(let_stmt);
        err(EXIT_FAILURE, "malloc failed");
    }
    let_stmt->statement.statement_type     = LET_STATEMENT;
    let_stmt->statement.node.token_literal = let_statement_token_literal;
    let_stmt->statement.node.string        = let_statement_string;
    let_stmt->statement.node.type          = STATEMENT;
    let_stmt->name                         = NULL;
    let_stmt->value                        = NULL;
    return let_stmt;
}

static ast_return_statement *create_return_statement(parser *parser) {
    ast_return_statement *ret_stmt = malloc(sizeof(*ret_stmt));
    if (ret_stmt == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    ret_stmt->token = token_copy(parser->cur_tok);
    if (ret_stmt->token == NULL) {
        free(ret_stmt);
        err(EXIT_FAILURE, "malloc failed");
    }
    ret_stmt->return_value                 = NULL;
    ret_stmt->statement.statement_type     = RETURN_STATEMENT;
    ret_stmt->statement.node.token_literal = return_statement_token_literal;
    ret_stmt->statement.node.string        = return_statement_string;
    ret_stmt->statement.node.type          = STATEMENT;
    return ret_stmt;
}

static ast_expression_statement *create_expression_statement(parser *parser) {
    ast_expression_statement *exp_stmt = malloc(sizeof(*exp_stmt));
    if (exp_stmt == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    exp_stmt->token = token_copy(parser->cur_tok);
    if (exp_stmt->token == NULL) {
        free(exp_stmt);
        err(EXIT_FAILURE, "malloc failed");
    }
    exp_stmt->expression                   = NULL;
    exp_stmt->statement.statement_type     = EXPRESSION_STATEMENT;
    exp_stmt->statement.node.token_literal = expression_statement_token_literal;
    exp_stmt->statement.node.string        = expression_statement_string;
    exp_stmt->statement.node.type          = STATEMENT;
    return exp_stmt;
}

static ast_block_statement *create_block_statement(parser *parser) {
    ast_block_statement *block_stmt = malloc(sizeof(*block_stmt));
    if (block_stmt == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    block_stmt->statement.node.string        = block_statement_string;
    block_stmt->statement.node.token_literal = block_statement_token_literal;
    block_stmt->statement.node.type          = STATEMENT;
    block_stmt->statement.statement_type     = BLOCK_STATEMENT;
    block_stmt->array_size                   = 8;
    block_stmt->statements                   = calloc(block_stmt->array_size, sizeof(*block_stmt->statements));
    if (block_stmt->statements == NULL) {
        free(block_stmt);
        err(EXIT_FAILURE, "malloc failed");
    }
    block_stmt->statement_count = 0;
    block_stmt->token           = token_copy(parser->cur_tok);
    return block_stmt;
}

static ast_function_literal *create_function_literal(parser *parser) {
    ast_function_literal *func = malloc(sizeof(*func));
    if (func == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    func->expression.node.string        = function_literal_string;
    func->expression.node.token_literal = function_literal_token_literal;
    func->expression.node.type          = EXPRESSION;
    func->expression.expression_type    = FUNCTION_LITERAL;
    func->parameters                    = linked_list_create();
    func->token                         = token_copy(parser->cur_tok);
    func->body                          = NULL;
    func->name                          = NULL;
    return func;
}

static ast_call_expression *create_call_expression(parser *parser) {
    ast_call_expression *call_exp = malloc(sizeof(*call_exp));
    if (call_exp == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }

    call_exp->expression.node.token_literal = call_expression_token_literal;
    call_exp->expression.node.string        = call_expression_string;
    call_exp->expression.node.type          = EXPRESSION;
    call_exp->expression.expression_type    = CALL_EXPRESSION;
    call_exp->arguments                     = linked_list_create();
    if (call_exp->arguments == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    call_exp->token = token_copy(parser->peek_tok);
    if (call_exp->token == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    call_exp->function = NULL;
    return call_exp;
}


void *create_statement(parser *parser, ast_statement_type stmt_type) {
    switch (stmt_type) {
        case LET_STATEMENT:
            return create_let_statement(parser);
        case RETURN_STATEMENT:
            return create_return_statement(parser);
        case EXPRESSION_STATEMENT:
            return create_expression_statement(parser);
        case BLOCK_STATEMENT:
            return create_block_statement(parser);
        default:
            return NULL;
    }
}


parser *parser_init(lexer *l) {
    parser *parser = malloc(sizeof(*parser));
    if (parser == NULL) {
        return NULL;
    }
    parser->lexer    = l;
    parser->cur_tok  = NULL;
    parser->peek_tok = NULL;
    parser->errors   = NULL;
    parser_next_token(parser);
    parser_next_token(parser);
    return parser;
}

void parser_next_token(parser *parser) {
    if (parser->cur_tok) {
        token_free(parser->cur_tok);
    }
    parser->cur_tok  = parser->peek_tok;
    parser->peek_tok = lexer_next_token(parser->lexer);
}

static int add_statement_to_program(ast_program *program, ast_statement *stmt) {
    if (program->statement_count == program->array_size) {
        size_t new_size     = program->array_size * 2;
        program->statements = reallocarray(program->statements, new_size, sizeof(*program->statements));
        if (program->statements == NULL) {
            return 1;
        }
        program->array_size = new_size;
    }
    program->statements[program->statement_count++] = stmt;
    return 0;
}

static int add_statement_to_block(ast_block_statement *block_stmt, ast_statement *stmt) {
    if (block_stmt->statement_count == block_stmt->array_size) {
        size_t new_size        = block_stmt->array_size * 2;
        block_stmt->statements = reallocarray(block_stmt->statements, new_size, sizeof(*block_stmt->statements));
        if (block_stmt->statements == NULL)
            return 1;
        block_stmt->array_size = new_size;
    }
    block_stmt->statements[block_stmt->statement_count++] = stmt;
    return 0;
}


static void peek_error(parser *parser, token_type tok_type) {
    char *msg = NULL;
    asprintf(&msg, "expected next token to be %s, got %s instead", token_get_name_from_type(tok_type),
             token_get_name_from_type(parser->peek_tok->type));
    if (msg == NULL)
        err(EXIT_FAILURE, "malloc failed");
    add_parse_error(parser, msg);
}

static int expect_peek(parser *parser, token_type tok_type) {
    if (parser->peek_tok->type == tok_type) {
        parser_next_token(parser);
        return 1;
    }
    peek_error(parser, tok_type);
    return 0;
}

static char *ident_token_literal(void *node) {
    ast_identifier *ident = (ast_identifier *) node;
    return ident->token->literal;
}

static ast_identifier *create_identifier(parser *parser) {
    ast_identifier *ident = malloc(sizeof(*ident));
    if (ident == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    ident->token = token_copy(parser->cur_tok);
    if (ident->token == NULL) {
        free(ident);
        err(EXIT_FAILURE, "malloc failed");
    }
    ident->expression.node.token_literal = ident_token_literal;
    ident->expression.expression_type    = IDENTIFIER_EXPRESSION;
    ident->expression.node.string        = identifier_string;
    ident->expression.node.type          = EXPRESSION;
    ident->value                         = strdup(parser->cur_tok->literal);
    if (ident->value == NULL) {
        token_free(ident->token);
        free(ident);
        err(EXIT_FAILURE, "malloc failed");
    }
    return ident;
}

// TODO: can we create a create_expression API like we have for statements?
static ast_expression *parse_identifier_expression(parser *parser) {
    ast_identifier *ident = create_identifier(parser);
    return (ast_expression *) ident;
}

static ast_expression *parse_expression(parser *parser, operator_precedence precedence) {
#ifdef TRACE
    trace("parse_expression");
#endif
    prefix_parse_fn prefix_fn = prefix_fns[parser->cur_tok->type];
    if (prefix_fn == NULL) {
        handle_no_prefix_fn(parser);
        return NULL;
    }
    ast_expression *left_exp = prefix_fn(parser);

    while (parser->peek_tok->type != SEMICOLON) {
        if (precedence >= peek_precedence(parser)) {
            break;
        }

        infix_parse_fn infix_fn = infix_fns[parser->peek_tok->type];
        if (infix_fn == NULL) {
            return left_exp;
        }

        parser_next_token(parser);
        ast_expression *right = infix_fn(parser, left_exp);
        left_exp              = right;
    }

#ifdef TRACE
    untrace("parse_expression");
#endif
    return left_exp;
}

static ast_let_statement *parse_let_statement(parser *parser) {
    ast_let_statement *let_stmt = (ast_let_statement *) create_statement(parser, LET_STATEMENT);

    if (!expect_peek(parser, IDENT)) {
        free_statement((ast_statement *) let_stmt);
        return NULL;
    }

    ast_identifier *ident = (ast_identifier *) parse_identifier_expression(parser);
    let_stmt->name        = ident;
    if (!expect_peek(parser, ASSIGN)) {
        free_statement((ast_statement *) let_stmt);
        return NULL;
    }
    parser_next_token(parser);
    let_stmt->value = parse_expression(parser, LOWEST);
    if (let_stmt->value->expression_type == FUNCTION_LITERAL) {
        ast_function_literal *fn_literal = (ast_function_literal *) let_stmt->value;
        fn_literal->name                 = let_stmt->name->value;
        if (fn_literal->name == NULL) {
            err(EXIT_FAILURE, "malloc failed");
        }
    }
    if (parser->peek_tok->type == SEMICOLON) {
        parser_next_token(parser);
    }

    return let_stmt;
}

static ast_return_statement *parse_return_statement(parser *parser) {
    ast_return_statement *ret_stmt = (ast_return_statement *) create_statement(parser, RETURN_STATEMENT);
    parser_next_token(parser);
    ret_stmt->return_value = parse_expression(parser, LOWEST);
    if (parser->peek_tok->type == SEMICOLON) {
        parser_next_token(parser);
    }
    return ret_stmt;
}

ast_program *program_init(void) {
    ast_program *program = malloc(sizeof(*program));
    if (program == NULL)
        return NULL;
    program->node.token_literal = program_token_literal;
    program->node.string        = program_string;
    program->node.type          = PROGRAM;
    program->array_size         = 64;
    program->statements         = calloc(64, sizeof(*program->statements));
    if (program->statements == NULL) {
        free(program);
        return NULL;
    }
    program->statement_count = 0;
    return program;
}

ast_program *parse_program(parser *parser) {
    ast_program *program = program_init();
    if (program == NULL)
        err(EXIT_FAILURE, "malloc failed");
    while (parser->cur_tok->type != END_OF_FILE) {
        ast_statement *stmt = parser_parse_statement(parser);
        if (stmt != NULL) {
            int status = add_statement_to_program(program, stmt);
            if (status != 0) {
                program_free(program);
                return NULL;
            }
        }
        parser_next_token(parser);
    }
    return program;
}

static ast_expression_statement *parse_expression_statement(parser *parser) {
#ifdef TRACE
    trace("parse_expression_statement");
#endif
    ast_expression_statement *exp_stmt = create_expression_statement(parser);
    exp_stmt->expression               = parse_expression(parser, LOWEST);
    if (parser->peek_tok->type == SEMICOLON)
        parser_next_token(parser);
#ifdef TRACE
    untrace("parse_expression_statement");
#endif
    return exp_stmt;
}


ast_statement *parser_parse_statement(parser *parser) {
    switch (parser->cur_tok->type) {
        case LET:
            return (ast_statement *) parse_let_statement(parser);
        case RETURN:
            return (ast_statement *) parse_return_statement(parser);
        default:
            return (ast_statement *) parse_expression_statement(parser);
    }
}

static char *int_exp_token_literal(void *node) {
    ast_integer *int_exp = (ast_integer *) node;
    return int_exp->token->literal;
}

static char *index_exp_token_literal(void *exp) {
    ast_index_expression *index_exp = (ast_index_expression *) exp;
    return index_exp->token->literal;
}

ast_expression *parse_integer_expression(parser *parser) {
#ifdef TRACE
    trace("parse_integer_expression");
#endif
    ast_integer *int_exp = malloc(sizeof(ast_integer));
    if (int_exp == NULL)
        err(EXIT_FAILURE, "malloc failed");
    int_exp->expression.node.token_literal = int_exp_token_literal;
    int_exp->expression.node.string        = integer_string;
    int_exp->expression.node.type          = EXPRESSION;
    int_exp->expression.expression_type    = INTEGER_EXPRESSION;
    int_exp->token                         = token_copy(parser->cur_tok);
    errno                                  = 0;
    char *ep;
    int_exp->value = strtol(parser->cur_tok->literal, &ep, 10);
    if (ep == parser->cur_tok->literal || *ep != 0 || errno != 0) {
        char *errmsg = NULL;
        asprintf(&errmsg, "could not parse %s as integer", parser->cur_tok->literal);
        if (errmsg == NULL)
            err(EXIT_FAILURE, "malloc failed");
        add_parse_error(parser, errmsg);
    }

#ifdef TRACE
    untrace("parse_integer_expression");
#endif

    return (ast_expression *) int_exp;
}

ast_expression *parse_string_expression(parser *parser) {
#ifdef TRACE
    trace("parse_string_expression");
#endif
    ast_string *string = malloc(sizeof(*string));
    if (string == NULL)
        err(EXIT_FAILURE, "malloc failed");
    string->expression.node.string        = string_string;
    string->expression.node.token_literal = string_token_literal;
    string->expression.node.type          = EXPRESSION;
    string->expression.expression_type    = STRING_EXPRESSION;
    string->token                         = token_copy(parser->cur_tok);
    string->value                         = strdup(parser->cur_tok->literal);
    string->length                        = strlen(parser->cur_tok->literal);
    if (string->value == NULL)
        err(EXIT_FAILURE, "malloc failed");
#ifdef TRACE
    untrace("parse_string_expression");
#endif
    return (ast_expression *) string;
}

ast_expression *parse_prefix_expression(parser *parser) {
#ifdef TRACE
    trace("parse_prefix_expression");
#endif
    ast_prefix_expression *prefix_exp = malloc(sizeof(*prefix_exp));
    if (prefix_exp == NULL)
        err(EXIT_FAILURE, "malloc failed");
    prefix_exp->expression.expression_type    = PREFIX_EXPRESSION;
    prefix_exp->expression.node.string        = prefix_expression_string;
    prefix_exp->expression.node.token_literal = prefix_expression_token_literal;
    prefix_exp->expression.node.type          = EXPRESSION;
    prefix_exp->token                         = token_copy(parser->cur_tok);
    prefix_exp->operator                      = strdup(parser->cur_tok->literal);
    if (prefix_exp->operator == NULL)
        err(EXIT_FAILURE, "malloc failed");
    parser_next_token(parser);
    prefix_exp->right = parse_expression(parser, PREFIX);

#ifdef TRACE
    untrace("parse_prefix_expression");
#endif
    return (ast_expression *) prefix_exp;
}


static ast_hash_literal *create_hash_literal(token *cur_tok) {
    ast_hash_literal *hash_exp = malloc(sizeof(*hash_exp));
    if (hash_exp == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    hash_exp->token = token_copy(cur_tok);
    hash_exp->expression.node.string = hash_literal_string;
    hash_exp->expression.node.token_literal = hash_literal_token_literal;
    hash_exp->expression.node.type = EXPRESSION;
    hash_exp->expression.expression_type = HASH_LITERAL;
    hash_exp->pairs = hashtable_create(pointer_hash_function, pointer_equals, free_expression, free_expression);
    return hash_exp;
}

static ast_expression *parse_hash_literal(parser *parser) {
    ast_hash_literal *hash_exp = create_hash_literal(parser->cur_tok);
    while (parser->peek_tok->type != RBRACE) {
        parser_next_token(parser);
        ast_expression *key = parse_expression(parser, LOWEST);
        if (!expect_peek(parser, COLON)) {
            hashtable_destroy(hash_exp->pairs);
            token_free(hash_exp->token);
            free(hash_exp);
            return NULL;
        }

        parser_next_token(parser);
        ast_expression *value = parse_expression(parser, LOWEST);
        hashtable_set(hash_exp->pairs, key, value);
        if (parser->peek_tok->type != RBRACE && !expect_peek(parser, COMMA)) {
            hashtable_destroy(hash_exp->pairs);
            token_free(hash_exp->token);
            free(hash_exp);
            return NULL;
        }
    }

    if (!expect_peek(parser, RBRACE)) {
        free_hash_literal(hash_exp);
        return NULL;
    }
    return (ast_expression *) hash_exp;
}

static ast_expression *parse_boolean_expression(parser *parser) {
#ifdef TRACE
    trace("parse_boolean_expression");
#endif
    ast_boolean_expression *bool_exp = malloc(sizeof(*bool_exp));
    if (bool_exp == NULL)
        err(EXIT_FAILURE, "malloc failed");
    bool_exp->token                         = token_copy(parser->cur_tok);
    bool_exp->expression.expression_type    = BOOLEAN_EXPRESSION;
    bool_exp->expression.node.token_literal = boolean_expression_token_literal;
    bool_exp->expression.node.string        = boolean_expression_string;
    bool_exp->expression.node.type          = EXPRESSION;
    if (parser->cur_tok->type == TRUE)
        bool_exp->value = true;
    else
        bool_exp->value = false;

#ifdef TRACE
    untrace("parse_boolean_expression");
#endif
    return (ast_expression *) bool_exp;
}

static arraylist *parse_expression_list(parser *parser, token_type stop_token_type) {
    arraylist *expression_list = arraylist_create(4, free_expression);
    if (parser->peek_tok->type == stop_token_type) {
        parser_next_token(parser);
        return expression_list;
    }

    parser_next_token(parser);
    ast_expression *exp = parse_expression(parser, LOWEST);
    arraylist_add(expression_list, exp);
    while (parser->peek_tok->type == COMMA) {
        parser_next_token(parser);
        parser_next_token(parser);
        exp = parse_expression(parser, LOWEST);
        arraylist_add(expression_list, exp);
    }

    if (!expect_peek(parser, stop_token_type)) {
        arraylist_destroy(expression_list);
        return NULL;
    }
    return expression_list;
}

static ast_expression *parse_grouped_expression(parser *parser) {
#ifdef TRACE
    trace("parse_grouped_expression");
#endif
    parser_next_token(parser);
    ast_expression *exp = parse_expression(parser, LOWEST);
    if (!expect_peek(parser, RPAREN)) {
        free_expression(exp);
        exp = NULL;
    }

#ifdef TRACE
    untrace("parse_grouped_expression");
#endif
    return exp;
}

static ast_expression *parse_array_literal(parser *parser) {
#ifdef TRACE
    trace("parse_array_literal");
#endif
    ast_array_literal *array = malloc(sizeof(*array));
    if (array == NULL)
        err(EXIT_FAILURE, "malloc failed");
    array->elements                      = parse_expression_list(parser, RBRACKET);
    array->token                         = token_copy(parser->cur_tok);
    array->expression.node.string        = array_literal_string;
    array->expression.node.token_literal = array_literal_token_literal;
    array->expression.node.type          = EXPRESSION;
    array->expression.expression_type    = ARRAY_LITERAL;
#ifdef TRACE
    untrace("parse_array_literal");
#endif
    return (ast_expression *) array;
}

static ast_expression *parse_index_expression(parser *parser, ast_expression *left) {
#ifdef TRACE
    trace("parse_index_expression");
#endif
    ast_index_expression *index_exp = malloc(sizeof(*index_exp));
    if (index_exp == NULL)
        err(EXIT_FAILURE, "malloc failed");
    index_exp->expression.node.string        = index_exp_string;
    index_exp->expression.node.token_literal = index_exp_token_literal;
    index_exp->expression.node.type          = EXPRESSION;
    index_exp->expression.expression_type    = INDEX_EXPRESSION;
    index_exp->left                          = left;
    index_exp->index                         = NULL;
    index_exp->token                         = token_copy(parser->cur_tok);
    parser_next_token(parser);
    index_exp->index = parse_expression(parser, LOWEST);
    if (!expect_peek(parser, RBRACKET)) {
        free_index_expression(index_exp);
        index_exp = NULL;
    }
#ifdef TRACE
    untrace("parse_index_expression");
#endif
    return (ast_expression *) index_exp;
}

static ast_block_statement *parse_block_statement(parser *parser) {
#ifdef TRACE
    trace("parse_block_statement");
#endif
    ast_block_statement *block_stmt = create_block_statement(parser);
    parser_next_token(parser);
    while (parser->cur_tok->type != RBRACE && parser->cur_tok->type != END_OF_FILE) {
        ast_statement *stmt = parser_parse_statement(parser);
        if (stmt != NULL)
            add_statement_to_block(block_stmt, stmt);
        parser_next_token(parser);
    }
#ifdef TRACE
    untrace("parse_block_statement");
#endif
    return block_stmt;
}

static ast_expression *parse_while_expression(parser *parser) {
#ifdef TRACE
    trace("parse_while_expression");
#endif
    ast_while_expression *while_exp = malloc(sizeof(*while_exp));
    if (while_exp == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    while_exp->expression.node.string        = while_expression_string;
    while_exp->expression.node.token_literal = while_expression_token_literal;
    while_exp->expression.node.type          = EXPRESSION;
    while_exp->expression.expression_type    = WHILE_EXPRESSION;
    while_exp->token                         = token_copy(parser->cur_tok);
    while_exp->condition                     = NULL;
    while_exp->body                          = NULL;

    if (!expect_peek(parser, LPAREN)) {
        free_while_expression(while_exp);
        return NULL;
    }
    parser_next_token(parser);
    while_exp->condition = parse_expression(parser, LOWEST);
    if (while_exp->condition == NULL || !expect_peek(parser, RPAREN)) {
        free_while_expression(while_exp);
        return NULL;
    }
    if (!expect_peek(parser, LBRACE)) {
        free_while_expression(while_exp);
        return NULL;
    }
    while_exp->body = parse_block_statement(parser);
    if (while_exp->body == NULL) {
        free_while_expression(while_exp);
        return NULL;
    }
    return (ast_expression *) while_exp;
}


static ast_expression *parse_if_expression(parser *parser) {
#ifdef TRACE
    trace("parse_if_expression");
#endif
    ast_if_expression *if_exp;
    if_exp = malloc(sizeof(*if_exp));
    if (if_exp == NULL)
        err(EXIT_FAILURE, "malloc failed");

    if_exp->expression.node.string        = if_expression_string;
    if_exp->expression.node.token_literal = if_expression_token_literal;
    if_exp->expression.node.type          = EXPRESSION;
    if_exp->expression.expression_type    = IF_EXPRESSION;
    if_exp->token                         = token_copy(parser->cur_tok);
    if_exp->condition                     = NULL;
    if_exp->alternative                   = NULL;
    if_exp->consequence                   = NULL;

    if (!expect_peek(parser, LPAREN)) {
        token_free(if_exp->token);
        free(if_exp);
        return NULL;
    }

    parser_next_token(parser);
    if_exp->condition = parse_expression(parser, LOWEST);
    if (!expect_peek(parser, RPAREN)) {
        free_if_expression(if_exp);
        return NULL;
    }

    if (!expect_peek(parser, LBRACE)) {
        free_if_expression(if_exp);
        return NULL;
    }

    if_exp->consequence = parse_block_statement(parser);

    if (parser->peek_tok->type == ELSE) {
        parser_next_token(parser);
        if (!expect_peek(parser, LBRACE)) {
            free_if_expression(if_exp);
            return NULL;
        }
        if_exp->alternative = parse_block_statement(parser);
    }
#ifdef TRACE
    untrace("parse_if_expression");
#endif
    return (ast_expression *) if_exp;
}

static void parse_function_parameters(parser *parser, ast_function_literal *function) {
    if (parser->peek_tok->type == RPAREN) {
        parser_next_token(parser);
        return;
    }

    parser_next_token(parser);
    ast_identifier *identifier = create_identifier(parser);
    linked_list_addNode(function->parameters, identifier);
    while (parser->peek_tok->type == COMMA) {
        parser_next_token(parser);
        parser_next_token(parser);
        identifier = create_identifier(parser);
        linked_list_addNode(function->parameters, identifier);
    }

    if (!expect_peek(parser, RPAREN)) {
        linked_list_free(function->parameters, free_identifier);
        function->parameters = NULL;
    }
}

static ast_expression *parse_function_literal(parser *parser) {
    ast_function_literal *function = create_function_literal(parser);
    if (!expect_peek(parser, LPAREN)) {
        free_function_literal(function);
        return NULL;
    }
    parse_function_parameters(parser, function);
    if (function->parameters == NULL) {
        free_function_literal(function);
        return NULL;
    }

    if (!expect_peek(parser, LBRACE)) {
        linked_list_free(function->parameters, free_identifier);
        token_free(function->token);
        free(function);
        return NULL;
    }

    function->body = parse_block_statement(parser);
    return (ast_expression *) function;
}


static void parse_call_arguments(parser *parser, ast_call_expression *call_exp) {
    if (parser->peek_tok->type == RPAREN) {
        parser_next_token(parser);
        return;
    }

    parser_next_token(parser);
    ast_expression *arg = parse_expression(parser, LOWEST);
    linked_list_addNode(call_exp->arguments, arg);
    while (parser->peek_tok->type == COMMA) {
        parser_next_token(parser);
        parser_next_token(parser);
        arg = parse_expression(parser, LOWEST);
        linked_list_addNode(call_exp->arguments, arg);
    }

    if (!expect_peek(parser, RPAREN)) {
        linked_list_free(call_exp->arguments, free_expression);
        call_exp->arguments = NULL;
        return;
    }
}

static ast_expression *copy_identifier_expression(ast_expression *exp) {
    ast_identifier *ident_exp = (ast_identifier *) exp;
    ast_identifier *copy      = malloc(sizeof(*copy));
    if (copy == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    copy->expression.node.string        = identifier_string;
    copy->expression.node.token_literal = ident_token_literal;
    copy->expression.node.type          = EXPRESSION;
    copy->expression.expression_type    = IDENTIFIER_EXPRESSION;
    copy->token                         = token_copy(ident_exp->token);
    copy->value                         = strdup(ident_exp->value);
    if (copy->value == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    return (ast_expression *) copy;
}

static ast_expression *copy_integer_expression(ast_expression *exp) {
    ast_integer *int_exp = (ast_integer *) exp;
    ast_integer *copy    = malloc(sizeof(*copy));
    if (copy == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    copy->token                         = token_copy(int_exp->token);
    copy->expression.node.string        = integer_string;
    copy->expression.node.token_literal = int_exp_token_literal;
    copy->expression.node.type          = EXPRESSION;
    copy->expression.expression_type    = INTEGER_EXPRESSION;
    copy->value                         = int_exp->value;
    return (ast_expression *) copy;
}

static ast_expression *copy_prefix_expression(ast_expression *exp) {
    ast_prefix_expression *prefix_exp = (ast_prefix_expression *) exp;
    ast_prefix_expression *copy       = malloc(sizeof(*copy));
    if (copy == NULL)
        err(EXIT_FAILURE, "malloc failed");
    copy->expression.node.string        = prefix_exp->expression.node.string;
    copy->expression.node.token_literal = prefix_exp->expression.node.token_literal;
    copy->expression.node.type          = EXPRESSION;
    copy->expression.expression_type    = PREFIX_EXPRESSION;
    copy->token                         = token_copy(prefix_exp->token);
    copy->operator                      = strdup(prefix_exp->operator);
    if (copy->operator == NULL)
        err(EXIT_FAILURE, "malloc failed");
    copy->right = copy_expression(prefix_exp->right);
    return (ast_expression *) copy;
}

static ast_expression *copy_infix_expression(ast_expression *exp) {
    ast_infix_expression *infix_exp = (ast_infix_expression *) exp;
    ast_infix_expression *copy      = malloc(sizeof(*copy));
    if (copy == NULL)
        err(EXIT_FAILURE, "malloc failed");
    copy->expression.node.string        = infix_exp->expression.node.string;
    copy->expression.node.token_literal = infix_exp->expression.node.token_literal;
    copy->expression.node.type          = EXPRESSION;
    copy->expression.expression_type    = INFIX_EXPRESSION;
    copy->token                         = token_copy(infix_exp->token);
    copy->operator                      = strdup(infix_exp->operator);
    if (copy->operator == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    copy->left  = copy_expression(infix_exp->left);
    copy->right = copy_expression(infix_exp->right);
    return (ast_expression *) copy;
}

static ast_expression *copy_boolean_expression(ast_expression *exp) {
    ast_boolean_expression *bool_exp = (ast_boolean_expression *) exp;
    ast_boolean_expression *copy     = malloc(sizeof(*copy));
    if (copy == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    copy->expression.node.string        = bool_exp->expression.node.string;
    copy->expression.node.token_literal = bool_exp->expression.node.token_literal;
    copy->expression.node.type          = EXPRESSION;
    copy->expression.expression_type    = BOOLEAN_EXPRESSION;
    copy->token                         = token_copy(bool_exp->token);
    copy->value                         = bool_exp->value;
    return (ast_expression *) copy;
}

static ast_expression *copy_if_expression(ast_expression *exp) {
    ast_if_expression *if_exp = (ast_if_expression *) exp;
    ast_if_expression *copy   = malloc(sizeof(*copy));
    if (copy == NULL)
        err(EXIT_FAILURE, "malloc failed");
    copy->expression.node.string        = if_exp->expression.node.string;
    copy->expression.node.token_literal = if_exp->expression.node.token_literal;
    copy->expression.node.type          = EXPRESSION;
    copy->expression.expression_type    = IF_EXPRESSION;
    copy->token                         = token_copy(if_exp->token);
    copy->condition                     = copy_expression(if_exp->condition);
    copy->consequence                   = (ast_block_statement *) copy_statement((ast_statement *) if_exp->consequence);
    if (if_exp->alternative)
        copy->alternative = (ast_block_statement *) copy_statement((ast_statement *) if_exp->alternative);
    else
        copy->alternative = NULL;
    return (ast_expression *) copy;
}

linked_list *copy_parameters(linked_list *parameters) {
    linked_list *copy_list            = linked_list_create();
    list_node *  parameters_list_node = parameters->head;
    while (parameters_list_node) {
        ast_identifier *param = (ast_identifier *) parameters_list_node->data;
        linked_list_addNode(copy_list, copy_expression((ast_expression *) param));
        parameters_list_node = parameters_list_node->next;
    }
    return copy_list;
}

static ast_expression *copy_function_literal(ast_expression *exp) {
    ast_function_literal *func = (ast_function_literal *) exp;
    ast_function_literal *copy = malloc(sizeof(*copy));
    if (copy == NULL)
        err(EXIT_FAILURE, "malloc failed");
    copy->expression.node.string        = func->expression.node.string;
    copy->expression.node.token_literal = func->expression.node.token_literal;
    copy->expression.node.type          = EXPRESSION;
    copy->expression.expression_type    = FUNCTION_LITERAL;
    copy->body                          = (ast_block_statement *) copy_statement((ast_statement *) func->body);
    copy->token                         = token_copy(func->token);
    copy->parameters                    = copy_parameters(func->parameters);
    return (ast_expression *) copy;
}

static ast_expression *copy_call_expression(ast_expression *exp) {
    ast_call_expression *call_exp = (ast_call_expression *) exp;
    ast_call_expression *copy     = malloc(sizeof(*copy));
    if (copy == NULL)
        err(EXIT_FAILURE, "malloc failed");
    copy->expression.node.string        = call_exp->expression.node.string;
    copy->expression.node.token_literal = call_exp->expression.node.token_literal;
    copy->expression.node.type          = EXPRESSION;
    copy->expression.expression_type    = CALL_EXPRESSION;
    copy->token                         = token_copy(call_exp->token);
    copy->arguments                     = copy_parameters(call_exp->arguments);
    copy->function                      = copy_expression(call_exp->function);
    return (ast_expression *) copy;
}

static ast_expression *copy_string_expression(ast_expression *exp) {
    ast_string *string = (ast_string *) exp;
    ast_string *copy   = malloc(sizeof(*copy));
    if (copy == NULL)
        err(EXIT_FAILURE, "malloc failed");
    copy->length                        = string->length;
    copy->token                         = token_copy(string->token);
    copy->expression.node.string        = string_string;
    copy->expression.node.token_literal = string_token_literal;
    copy->expression.node.type          = EXPRESSION;
    copy->expression.expression_type    = STRING_EXPRESSION;
    copy->value                         = strdup(string->value);
    if (copy->value == NULL)
        err(EXIT_FAILURE, "malloc failed");
    return (ast_expression *) copy;
}


static ast_expression *copy_hash_literal(ast_expression *exp) {
    ast_hash_literal *hash_exp = (ast_hash_literal *) exp;
    ast_hash_literal *copy     = create_hash_literal(hash_exp->token);
    for (size_t i = 0; i < hash_exp->pairs->key_count; i++) {
        hashtable_entry *entry     = (hashtable_entry *) hash_exp->pairs->table[i];
        ast_expression * key_exp   = (ast_expression *) entry->key;
        ast_expression * value_exp = (ast_expression *) entry->value;
        hashtable_set(copy->pairs, copy_expression(key_exp), copy_expression(value_exp));
    }
    return (ast_expression *) copy;
}

static ast_expression *copy_array_literal(ast_expression *exp) {
    ast_array_literal *array = (ast_array_literal *) exp;
    ast_array_literal *copy  = malloc(sizeof(*copy));
    if (copy == NULL)
        err(EXIT_FAILURE, "malloc failed");
    copy->token                         = token_copy(array->token);
    copy->expression.node.string        = array_literal_string;
    copy->expression.node.token_literal = array_literal_token_literal;
    copy->expression.node.type          = EXPRESSION;
    copy->expression.expression_type    = ARRAY_LITERAL;
    copy->elements                      = arraylist_create(array->elements->size, free_expression);
    for (size_t i = 0; i < array->elements->size; i++) {
        arraylist_add(copy->elements, copy_expression(array->elements->body[i]));
    }
    return (ast_expression *) copy;
}

static ast_expression *copy_index_expression(ast_expression *exp) {
    ast_index_expression *index_exp = (ast_index_expression *) exp;
    ast_index_expression *copy      = malloc(sizeof(*copy));
    if (copy == NULL)
        err(EXIT_FAILURE, "malloc failed");
    copy->expression.node.string        = index_exp_string;
    copy->expression.node.token_literal = index_exp_token_literal;
    copy->expression.node.type          = EXPRESSION;
    copy->expression.expression_type    = INDEX_EXPRESSION;
    copy->token                         = token_copy(index_exp->token);
    copy->left                          = copy_expression(index_exp->left);
    copy->index                         = copy_expression(index_exp->index);
    return (ast_expression *) copy;
}

ast_expression *copy_expression(ast_expression *exp) {
    switch (exp->expression_type) {
        case IDENTIFIER_EXPRESSION:
            return copy_identifier_expression(exp);
        case INTEGER_EXPRESSION:
            return copy_integer_expression(exp);
        case PREFIX_EXPRESSION:
            return copy_prefix_expression(exp);
        case INFIX_EXPRESSION:
            return copy_infix_expression(exp);
        case BOOLEAN_EXPRESSION:
            return copy_boolean_expression(exp);
        case IF_EXPRESSION:
            return copy_if_expression(exp);
        case FUNCTION_LITERAL:
            return copy_function_literal(exp);
        case CALL_EXPRESSION:
            return copy_call_expression(exp);
        case STRING_EXPRESSION:
            return copy_string_expression(exp);
        case ARRAY_LITERAL:
            return copy_array_literal(exp);
        case INDEX_EXPRESSION:
            return copy_index_expression(exp);
        case HASH_LITERAL:
            return copy_hash_literal(exp);
        default:
            return NULL;
    }
}

static ast_statement *copy_letstatement(ast_statement *stmt) {
    ast_let_statement *let_stmt  = (ast_let_statement *) stmt;
    ast_let_statement *copy_stmt = malloc(sizeof(*let_stmt));
    if (copy_stmt == NULL)
        err(EXIT_FAILURE, "malloc failed");
    copy_stmt->name = malloc(sizeof(*copy_stmt->name));
    if (copy_stmt->name == NULL)
        err(EXIT_FAILURE, "malloc failed");
    copy_stmt->name->token = token_copy(let_stmt->token);
    copy_stmt->name->value = strdup(let_stmt->name->value);
    if (copy_stmt->name->value == NULL)
        err(EXIT_FAILURE, "malloc failed");
    copy_stmt->token                        = token_copy(let_stmt->token);
    copy_stmt->value                        = copy_expression(let_stmt->value);
    copy_stmt->statement.node.string        = let_statement_string;
    copy_stmt->statement.node.token_literal = let_statement_token_literal;
    copy_stmt->statement.node.type          = STATEMENT;
    copy_stmt->statement.statement_type     = LET_STATEMENT;
    return (ast_statement *) copy_stmt;
}

static ast_statement *copy_return_statement(ast_statement *stmt) {
    ast_return_statement *ret_stmt = (ast_return_statement *) stmt;
    ast_return_statement *copy     = malloc(sizeof(*copy));
    if (copy == NULL)
        err(EXIT_FAILURE, "malloc failed");
    copy->token                        = token_copy(ret_stmt->token);
    copy->statement.node.string        = return_statement_string;
    copy->statement.node.token_literal = return_statement_token_literal;
    copy->statement.node.type          = STATEMENT;
    copy->statement.statement_type     = RETURN_STATEMENT;
    copy->return_value                 = copy_expression(ret_stmt->return_value);
    return (ast_statement *) copy;
}

static ast_statement *copy_expression_statement(ast_statement *stmt) {
    ast_expression_statement *exp_stmt = (ast_expression_statement *) stmt;
    ast_expression_statement *copy     = malloc(sizeof(*copy));
    if (copy == NULL)
        err(EXIT_FAILURE, "malloc failed");
    copy->statement.node.string        = exp_stmt->statement.node.string;
    copy->statement.node.token_literal = exp_stmt->statement.node.token_literal;
    copy->statement.node.type          = STATEMENT;
    copy->statement.statement_type     = EXPRESSION_STATEMENT;
    copy->token                        = token_copy(exp_stmt->token);
    copy->expression                   = copy_expression(exp_stmt->expression);
    return (ast_statement *) copy;
}

static ast_statement *copy_block_statement(ast_statement *stmt) {
    ast_block_statement *block_stmt = (ast_block_statement *) stmt;
    ast_block_statement *copy       = malloc(sizeof(*copy));
    if (copy == NULL)
        err(EXIT_FAILURE, "malloc failed");
    copy->statement.node.string        = block_stmt->statement.node.string;
    copy->statement.node.token_literal = block_stmt->statement.node.token_literal;
    copy->statement.node.type          = STATEMENT;
    copy->statement.statement_type     = BLOCK_STATEMENT;
    copy->token                        = token_copy(block_stmt->token);
    copy->statement_count              = block_stmt->statement_count;
    copy->array_size                   = block_stmt->array_size;
    copy->statements                   = calloc(copy->statement_count, sizeof(*copy->statements));
    if (copy->statements == NULL)
        err(EXIT_FAILURE, "malloc failed");
    for (size_t i = 0; i < copy->statement_count; i++) {
        copy->statements[i] = copy_statement(block_stmt->statements[i]);
    }
    return (ast_statement *) copy;
}

ast_statement *copy_statement(ast_statement *stmt) {
    switch (stmt->statement_type) {
        case LET_STATEMENT:
            return copy_letstatement(stmt);
        case RETURN_STATEMENT:
            return copy_return_statement(stmt);
        case EXPRESSION_STATEMENT:
            return copy_expression_statement(stmt);
        case BLOCK_STATEMENT:
            return copy_block_statement(stmt);
    }
}

static ast_expression *parse_infix_expression(parser *parser, ast_expression *left) {
    ast_infix_expression *infix_exp = malloc(sizeof(*infix_exp));
    if (infix_exp == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }

    // Initialize fields
    infix_exp->expression.expression_type    = INFIX_EXPRESSION;
    infix_exp->expression.node.string        = infix_expression_string;
    infix_exp->expression.node.token_literal = infix_expression_token_literal;
    infix_exp->expression.node.type          = EXPRESSION;
    infix_exp->left                          = left;
    infix_exp->right                         = NULL;
    infix_exp->operator                      = strdup(parser->cur_tok->literal);
    if (infix_exp->operator == NULL) {
        free(infix_exp);
        err(EXIT_FAILURE, "malloc failed");
    }

    infix_exp->token = token_copy(parser->cur_tok);
    if (infix_exp->token == NULL) {
        free(infix_exp->operator);
        free(infix_exp);
        err(EXIT_FAILURE, "malloc failed");
    }

    // Parse the right-hand side
    operator_precedence precedence = cur_precedence(parser);
    parser_next_token(parser);
    infix_exp->right = parse_expression(parser, precedence);
    if (infix_exp->right == NULL) {
        // Cleanup on failure
        free(infix_exp->operator);
        token_free(infix_exp->token);
        free(infix_exp);
        return NULL;
    }
    return (ast_expression *) infix_exp;
}


static ast_expression *parse_call_expression(parser *parser, ast_expression *function) {
    ast_call_expression *call_exp = create_call_expression(parser);
    parse_call_arguments(parser, call_exp);
    if (call_exp->arguments == NULL) {
        free_call_expression(call_exp);
        return NULL;
    }
    call_exp->function = function;
    return (ast_expression *) call_exp;
}
