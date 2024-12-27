//
// Created by dgood on 12/19/24.
//

#include "node_compiler.h"

#include <string.h>
#include "../opcode/opcode.h"
#include "compiler_core.h"
#include "compiler_utils.h"
#include "instructions.h"
#include "scope.h"

compiler_error compile_expression_node(compiler *compiler, ast_expression *expression_node) {
    compiler_error          error;
    compiler_error          none_error = {COMPILER_ERROR_NONE, NULL};
    ast_infix_expression *  infix_exp;
    ast_prefix_expression * prefix_exp;
    ast_integer *           int_exp;
    ast_boolean_expression *bool_exp;
    ast_identifier *        ident_exp;
    ast_if_expression *     if_exp;
    object_int *            int_obj;
    object_bool *           bool_obj;
    ast_string *            str_exp;
    object_string *         str_obj;
    ast_array_literal *     array_exp;
    ast_hash_literal *      hash_exp;
    ast_index_expression *  index_exp;
    ast_function_literal *  func_exp;
    ast_call_expression *   call_exp;
    size_t                  constant_idx;
    size_t                  op_jmp_false_pos, after_consequence_pos, jmp_pos, after_alternative_pos;
    compilation_scope *     scope;
    switch (expression_node->expression_type) {
        case INFIX_EXPRESSION:
            infix_exp = (ast_infix_expression *) expression_node;
            if (strcmp(infix_exp->operator, "<") == 0) {
                error = compile(compiler, (ast_node *) infix_exp->right);
                if (error.error_code != COMPILER_ERROR_NONE)
                    return error;
                error = compile(compiler, (ast_node *) infix_exp->left);
                if (error.error_code != COMPILER_ERROR_NONE)
                    return error;
                emit(compiler, OP_GREATER_THAN, 0);
                break;
            }
            error = compile(compiler, (ast_node *) infix_exp->left);
            if (error.error_code != COMPILER_ERROR_NONE)
                return error;
            error = compile(compiler, (ast_node *) infix_exp->right);
            if (error.error_code != COMPILER_ERROR_NONE)
                return error;
            if (strcmp(infix_exp->operator, "+") == 0)
                emit(compiler, OP_ADD, 0);
            else if (strcmp(infix_exp->operator, "-") == 0)
                emit(compiler, OP_SUB, 0);
            else if (strcmp(infix_exp->operator, "*") == 0)
                emit(compiler, OP_MUL, 0);
            else if (strcmp(infix_exp->operator, "/") == 0)
                emit(compiler, OP_DIV, 0);
            else if (strcmp(infix_exp->operator, ">") == 0)
                emit(compiler, OP_GREATER_THAN, 0);
            else if (strcmp(infix_exp->operator, "==") == 0)
                emit(compiler, OP_EQUAL, 0);
            else if (strcmp(infix_exp->operator, "!=") == 0)
                emit(compiler, OP_NOT_EQUAL, 0);
            else {
                error.error_code = COMPILER_UNKNOWN_OPERATOR;
                error.msg        = get_err_msg("Unknown operator %s", infix_exp->operator);
                return error;
            }
            break;
        case PREFIX_EXPRESSION:
            prefix_exp = (ast_prefix_expression *) expression_node;
            error = compile(compiler, (ast_node *) prefix_exp->right);
            if (error.error_code != COMPILER_ERROR_NONE)
                return error;
            if (strcmp(prefix_exp->operator, "-") == 0)
                emit(compiler, OP_MINUS, 0);
            else if (strcmp(prefix_exp->operator, "!") == 0)
                emit(compiler, OP_BANG, 0);
            else {
                error.error_code = COMPILER_UNKNOWN_OPERATOR;
                error.msg        = get_err_msg("Unknown operator %s", prefix_exp->operator);
                return error;
            }
            break;
        case INTEGER_EXPRESSION:
            int_exp = (ast_integer *) expression_node;
            int_obj      = object_create_int(int_exp->value);
            constant_idx = add_constant(compiler, (object_object *) int_obj);
            emit(compiler, OP_CONSTANT, (size_t[]){constant_idx});
            break;
        case BOOLEAN_EXPRESSION:
            bool_exp = (ast_boolean_expression *) expression_node;
            bool_obj = object_create_bool(bool_exp->value);
            if (bool_obj->value)
                emit(compiler, OP_TRUE, 0);
            else
                emit(compiler, OP_FALSE, 0);
            break;
        case STRING_EXPRESSION:
            str_exp = (ast_string *) expression_node;
            str_obj      = object_create_string(str_exp->value, strlen(str_exp->value));
            constant_idx = add_constant(compiler, (object_object *) str_obj);
            emit(compiler, OP_CONSTANT, (size_t[]){constant_idx});
            break;
        case IF_EXPRESSION:
            if_exp = (ast_if_expression *) expression_node;
            error = compile(compiler, (ast_node *) if_exp->condition);
            if (error.error_code != COMPILER_ERROR_NONE)
                return error;
            op_jmp_false_pos = emit(compiler, OP_JUMP_NOT_TRUTHY, (size_t[]){9999});
            error            = compile(compiler, (ast_node *) if_exp->consequence);
            if (error.error_code != COMPILER_ERROR_NONE)
                return error;
            if (last_instruction_is(compiler, OP_POP))
                remove_last_instruction(compiler);
            jmp_pos               = emit(compiler, OP_JUMP, (size_t[]){9999});
            scope                 = get_top_scope(compiler);
            after_consequence_pos = scope->instructions->length;
            change_operand(compiler, op_jmp_false_pos, after_consequence_pos);
            if (if_exp->alternative == NULL) {
                emit(compiler, OP_NULL, 0);
            } else {
                error = compile(compiler, (ast_node *) if_exp->alternative);
                if (error.error_code != COMPILER_ERROR_NONE)
                    return error;
                if (last_instruction_is(compiler, OP_POP))
                    remove_last_instruction(compiler);
            }
            after_alternative_pos = scope->instructions->length;
            change_operand(compiler, jmp_pos, after_alternative_pos);
            break;
        case IDENTIFIER_EXPRESSION:
            ident_exp = (ast_identifier *) expression_node;
            symbol *sym = symbol_resolve(compiler->symbol_table, ident_exp->value);
            if (sym == NULL) {
                error.error_code = COMPILER_UNDEFINED_VARIABLE;
                error.msg        = get_err_msg("undefined variable: %s\n", ident_exp->value);
                return error;
            }
            load_symbol(compiler, sym);
            break;
        case ARRAY_LITERAL:
            array_exp = (ast_array_literal *) expression_node;
            for (size_t i = 0; i < array_exp->elements->size; i++) {
                error = compile(compiler, arraylist_get(array_exp->elements, i));
                if (error.error_code != COMPILER_ERROR_NONE)
                    return error;
            }
            emit(compiler, OP_ARRAY, (size_t[]){array_exp->elements->size});
            break;
        case HASH_LITERAL:
            hash_exp = (ast_hash_literal *) expression_node;
            arraylist *keys = hashtable_get_keys(hash_exp->pairs);
            if (keys != NULL) {
                arraylist_sort(keys, compare_object_hash_keys);
                for (size_t i = 0; i < keys->size; i++) {
                    ast_node *key   = arraylist_get(keys, i);
                    ast_node *value = hashtable_get(hash_exp->pairs, key);
                    error           = compile(compiler, key);
                    if (error.error_code != COMPILER_ERROR_NONE)
                        return error;
                    error = compile(compiler, value);
                    if (error.error_code != COMPILER_ERROR_NONE)
                        return error;
                }
                arraylist_destroy(keys);
            }
            emit(compiler, OP_HASH, (size_t[]){2 * hash_exp->pairs->key_count});
            break;
        case INDEX_EXPRESSION:
            index_exp = (ast_index_expression *) expression_node;
            error = compile(compiler, (ast_node *) index_exp->left);
            if (error.error_code != COMPILER_ERROR_NONE)
                return error;
            error = compile(compiler, (ast_node *) index_exp->index);
            if (error.error_code != COMPILER_ERROR_NONE)
                return error;
            emit(compiler, OP_INDEX, 0);
            break;
        case FUNCTION_LITERAL:
            func_exp = (ast_function_literal *) expression_node;
            compiler_enter_scope(compiler);
            if (func_exp->name != NULL)
                symbol_define_function(compiler->symbol_table, func_exp->name);
            list_node *param_list_node = func_exp->parameters->head;
            while (param_list_node != NULL) {
                ast_identifier *param = param_list_node->data;
                symbol_define(compiler->symbol_table, param->value);
                param_list_node = param_list_node->next;
            }
            error = compile(compiler, (ast_node *) func_exp->body);
            if (error.error_code != COMPILER_ERROR_NONE)
                return error;
            if (last_instruction_is(compiler, OP_POP))
                replace_last_pop_with_return(compiler);
            if (!last_instruction_is(compiler, OP_RETURN_VALUE))
                emit(compiler, OP_RETURN, 0);

            arraylist *free_symbols = arraylist_clone(compiler->symbol_table->free_symbols, _copy_symbol, symbol_free);
            size_t num_locals = compiler->symbol_table->symbol_count;
            size_t free_symbols_count = free_symbols->size;
            instructions *ins = compiler_leave_scope(compiler);
            for (size_t i = 0; i < free_symbols->size; i++) {
                symbol *s = arraylist_get(free_symbols, i);
                load_symbol(compiler, s);
            }
            arraylist_destroy(free_symbols, symbol_free);
            object_compiled_fn *compiled_fn = object_create_compiled_fn(ins,
                                                                        num_locals, func_exp->parameters->size);
            instructions_free(ins);
            constant_idx = add_constant(compiler, (object_object *) compiled_fn);
            emit(compiler, OP_CLOSURE, (size_t[]){constant_idx, free_symbols_count});
            break;
        case CALL_EXPRESSION:
            call_exp = (ast_call_expression *) expression_node;
            error = compile(compiler, (ast_node *) call_exp->function);
            if (error.error_code != COMPILER_ERROR_NONE)
                return error;
            for (size_t i = 0; i < call_exp->arguments->size; i++) {
                ast_node *arg = ((ast_node *) linked_list_get_at(call_exp->arguments, i)->data);
                error         = compile(compiler, arg);
                if (error.error_code != COMPILER_ERROR_NONE)
                    return error;
            }
            emit(compiler, OP_CALL, (size_t[]){call_exp->arguments->size});
            break;
        default:
            return none_error;
    }
    return none_error;
}

compiler_error compile_statement_node(compiler *compiler, ast_statement *statement_node) {
    compiler_error            error;
    compiler_error            none_error = {COMPILER_ERROR_NONE, NULL};
    ast_expression_statement *expression_stmt;
    ast_block_statement *     block_stmt;
    ast_let_statement *       let_stmt;
    ast_return_statement *    ret_stmt;
    symbol *                  sym;
    size_t                    i;
    switch (statement_node->statement_type) {
        case EXPRESSION_STATEMENT:
            expression_stmt = (ast_expression_statement *) statement_node;
            error = compile_expression_node(compiler, expression_stmt->expression);
            if (error.error_code != COMPILER_ERROR_NONE)
                return error;
            emit(compiler, OP_POP, 0);
            break;
        case BLOCK_STATEMENT:
            block_stmt = (ast_block_statement *) statement_node;
            for (i = 0; i < block_stmt->statement_count; i++) {
                error = compile(compiler, (ast_node *) block_stmt->statements[i]);
                if (error.error_code != COMPILER_ERROR_NONE)
                    return error;
            }
            break;
        case LET_STATEMENT:
            let_stmt = (ast_let_statement *) statement_node;
            sym   = symbol_define(compiler->symbol_table, let_stmt->name->value);
            error = compile(compiler, (ast_node *) let_stmt->value);
            if (error.error_code != COMPILER_ERROR_NONE)
                return error;
            if (sym->scope == GLOBAL)
                emit(compiler, OP_SET_GLOBAL, (size_t[]){sym->index});
            else
                emit(compiler, OP_SET_LOCAL, (size_t[]){sym->index});
            break;
        case RETURN_STATEMENT:
            ret_stmt = (ast_return_statement *) statement_node;
            error = compile(compiler, (ast_node *) ret_stmt->return_value);
            if (error.error_code != COMPILER_ERROR_NONE)
                return error;
            emit(compiler, OP_RETURN_VALUE, 0);
            break;
        default:
            return none_error;
    }
    return none_error;
}
