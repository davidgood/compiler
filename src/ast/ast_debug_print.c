#include "ast_debug_print.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to create indentation
static void indent(FILE *out, int level) {
    for (int i = 0; i < level; i++) {
        fprintf(out, "  ");
    }
}

// Recursive function to pretty print an AST node
void ast_debug_print_node(FILE *out, ast_node *node, const int level) {
    if (node == NULL) {
        indent(out, level);
        fprintf(out, "NULL\n");
        return;
    }

    indent(out, level);
    fprintf(out, "Node Type: ");

    switch (node->type) {
        case PROGRAM:
            fprintf(out, "PROGRAM\n");
        const ast_program *program = (ast_program *)node;
        for (size_t i = 0; i < program->statement_count; i++) {
            ast_debug_print_node(out, (ast_node *)program->statements[i], level + 1);
        }
        break;
        case STATEMENT: {
            const ast_statement *stmt = (ast_statement *)node;
            fprintf(out, "STATEMENT (%s)\n", statement_type_values[stmt->statement_type]);
            switch (stmt->statement_type) {
                case LET_STATEMENT: {
                    const ast_let_statement *let_stmt = (ast_let_statement *)node;
                    indent(out, level + 1);
                    fprintf(out, "Name:\n");
                    ast_debug_print_node(out, (ast_node *)let_stmt->name, level + 2);
                    indent(out, level + 1);
                    fprintf(out, "Value:\n");
                    ast_debug_print_node(out, (ast_node *)let_stmt->value, level + 2);
                    break;
                }
                case RETURN_STATEMENT: {
                    const ast_return_statement *ret_stmt = (ast_return_statement *)node;
                    indent(out, level + 1);
                    fprintf(out, "Return Value:\n");
                    ast_debug_print_node(out, (ast_node *)ret_stmt->return_value, level + 2);
                    break;
                }
                case EXPRESSION_STATEMENT: {
                    const ast_expression_statement *expr_stmt = (ast_expression_statement *)node;
                    indent(out, level + 1);
                    fprintf(out, "Expression:\n");
                    ast_debug_print_node(out, (ast_node *)expr_stmt->expression, level + 2);
                    break;
                }
                case BLOCK_STATEMENT: {
                    const ast_block_statement *block = (ast_block_statement *)node;
                    for (size_t i = 0; i < block->statement_count; i++) {
                        ast_debug_print_node(out, (ast_node *)block->statements[i], level + 1);
                    }
                    break;
                }
                default:
                    indent(out, level);
                fprintf(out, "Unknown statement type\n");
            }
            break;
        }
        case EXPRESSION: {
            const ast_expression *expr = (ast_expression *)node;
            fprintf(out, "EXPRESSION (%d)\n", expr->expression_type);
            switch (expr->expression_type) {
                case IDENTIFIER_EXPRESSION: {
                    ast_identifier *identifier = (ast_identifier *)node;
                    indent(out, level + 1);
                    fprintf(out, "Identifier: %s\n", identifier->value);
                    break;
                }
                case INTEGER_EXPRESSION: {
                    const ast_integer *integer = (ast_integer *)node;
                    indent(out, level + 1);
                    fprintf(out, "Integer: %ld\n", integer->value);
                    break;
                }
                case STRING_EXPRESSION: {
                    const ast_string *string = (ast_string *)node;
                    indent(out, level + 1);
                    fprintf(out, "String: %s\n", string->value);
                    break;
                }
                case PREFIX_EXPRESSION: {
                    const ast_prefix_expression *prefix = (ast_prefix_expression *)node;
                    indent(out, level + 1);
                    fprintf(out, "Operator: %s\n", prefix->operator);
                    indent(out, level + 1);
                    fprintf(out, "Right:\n");
                    ast_debug_print_node(out, (ast_node *)prefix->right, level + 2);
                    break;
                }
                case INFIX_EXPRESSION: {
                    const ast_infix_expression *infix = (ast_infix_expression *)node;
                    indent(out, level + 1);
                    fprintf(out, "Operator: %s\n", infix->operator);
                    indent(out, level + 1);
                    fprintf(out, "Left:\n");
                    ast_debug_print_node(out, (ast_node *)infix->left, level + 2);
                    indent(out, level + 1);
                    fprintf(out, "Right:\n");
                    ast_debug_print_node(out, (ast_node *)infix->right, level + 2);
                    break;
                }
                case BOOLEAN_EXPRESSION: {
                    const ast_boolean_expression *boolean = (ast_boolean_expression *)node;
                    indent(out, level + 1);
                    fprintf(out, "Boolean: %s\n", boolean->value ? "true" : "false");
                    break;
                }
                case IF_EXPRESSION: {
                    const ast_if_expression *if_exp = (ast_if_expression *)node;
                    indent(out, level + 1);
                    fprintf(out, "IF_EXPRESSION: {\n");
                    indent(out, level + 1);
                    fprintf(out, "  Condition: ");
                    ast_debug_print_node(out, (ast_node *)if_exp->condition, level + 2);
                    printf("  Consequence:\n");
                    ast_debug_print_node(out, (ast_node *)if_exp->consequence->statements, (int)if_exp->consequence->statement_count);
                    if (if_exp->alternative) {
                        indent(out, level + 1);
                        fprintf(out,"  Alternative:\n");
                        ast_debug_print_node(out, (ast_node *)if_exp->alternative->statements, (const int)if_exp->alternative->statement_count);
                    }
                    indent(out, level + 1);
                    fprintf(out, "}\n");
                    break;
                }
                case FUNCTION_LITERAL: {
                    const ast_function_literal *func = (ast_function_literal *)node;
                    indent(out, level + 1);
                    fprintf(out, "FUNCTION_LITERAL: {\n");
                    indent(out, level + 1);
                    fprintf(out, "  Name: %s\n", func->name);
                    indent(out, level + 1);
                    fprintf(out, "  Parameters:\n");
                    const list_node *param = func->parameters->head;
                    while (param) {
                        const ast_identifier *identifier = (ast_identifier *)param->data;
                        indent(out, level + 1);
                        fprintf(out, "    %s\n", identifier->value);
                        param = param->next;
                    }
                    indent(out, level + 1);
                    fprintf(out, "  Body:\n");
                    ast_debug_print_node(out, (ast_node *)func->body, level + 3);
                    indent(out, level + 1);
                    fprintf(out, "}\n");
                    break;
                }
                case CALL_EXPRESSION: {
                    const ast_call_expression *call_exp = (ast_call_expression *)node;
                    indent(out, level + 1);
                    fprintf(out, "CALL_EXPRESSION: {\n");
                    indent(out, level + 1);
                    fprintf(out, "  Function: ");
                    ast_debug_print_node(out, (ast_node *)call_exp->function, level + 2);
                    indent(out, level + 1);
                    fprintf(out, "  Arguments:\n");
                    const list_node *arg = call_exp->arguments->head;
                    while (arg) {
                        ast_debug_print_node(out, arg->data, level + 3);
                        arg = arg->next;
                    }
                    indent(out, level + 1);
                    fprintf(out, "}\n");
                    break;
                }
                case ARRAY_LITERAL: {
                    const ast_array_literal *array = (ast_array_literal *)node;
                    indent(out, level + 1);
                    fprintf(out, "ARRAY_LITERAL: [");
                    for (size_t i = 0; i < array->elements->size; i++) {
                        if (i > 0) {
                            indent(out, level + 1);
                            fprintf(out, ", ");
                        }
                        ast_debug_print_node(out, array->elements->body[i], level + 2);
                    }
                    indent(out, level + 1);
                    fprintf(out, "]\n");
                    break;
                }
                case INDEX_EXPRESSION: {
                    ast_index_expression *index_exp = (ast_index_expression *)node;
                    indent(out, level + 1);
                    fprintf(out, "INDEX_EXPRESSION: {\n");
                    indent(out, level + 1);
                    fprintf(out, "  Left: ");
                    ast_debug_print_node(out, (ast_node *)index_exp->left, level + 2);
                    indent(out, level + 1);
                    fprintf(out, "  Index: ");
                    ast_debug_print_node(out, (ast_node *)index_exp->index, level + 2);
                    indent(out, level + 1);
                    fprintf(out, "}\n");
                    break;
                }
                case HASH_LITERAL: {
                    const ast_hash_literal *hash = (ast_hash_literal *)node;
                    indent(out, level + 1);
                    fprintf(out, "HASH_LITERAL: {\n");
                    for (size_t i = 0; i < hash->pairs->used_slots->size; i++) {
                        const size_t          *index = (size_t *) hash->pairs->used_slots->body[i];
                        const hashtable_entry *entry = (hashtable_entry *)hash->pairs->table[*index]->head->data;
                        const ast_expression    *key = (ast_expression *)entry->key;
                        const ast_expression  *value = (ast_expression *)entry->value;

                        ast_debug_print_node(out, (ast_node *)key, level + 2);
                        ast_debug_print_node(out, (ast_node *)value, level + 2);
                    }
                    indent(out, level + 1);
                    fprintf(out, "}\n");
                    break;
                }
                case WHILE_EXPRESSION: {
                    const ast_while_expression *while_exp = (ast_while_expression *)node;
                    indent(out, level + 1);
                    fprintf(out, "WHILE_EXPRESSION: {\n");
                    indent(out, level + 1);
                    fprintf(out, "  Condition: ");
                    ast_debug_print_node(out, (ast_node *)while_exp->condition, level + 2);
                    indent(out, level + 1);
                    fprintf(out, "  Body:\n");
                    ast_debug_print_node(out, (ast_node *)while_exp->body->statements, (int)while_exp->body->statement_count);
                    indent(out, level + 1);
                    fprintf(out, "}\n");
                    break;
                }
                default:
                    indent(out, level);
                fprintf(out, "Unknown expression type\n");
            }
            break;
        }
        default:
            indent(out, level);
            fprintf(out, "Unknown node type: %d\n", node->type);
    }
}

// Entry point for pretty-printing the program
void ast_debug_print(ast_program *program) {
    ast_debug_print_node(stdout, (ast_node *)program, 0);
}
