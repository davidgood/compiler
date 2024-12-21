//
// Created by dgood on 12/5/24.
//

#ifndef COMPILER_H
#define COMPILER_H
#include "../ast/ast.h"
#include "../datastructures/arraylist.h"
#include "../object/opcode.h"
#include "../object/object.h"
#include "symbol_table.h"

typedef struct {
    Opcode opcode;
    size_t position;
} emitted_instruction;

typedef struct {
    instructions *      instructions;
    emitted_instruction last_instruction;
    emitted_instruction prev_instruction;
} compilation_scope;

typedef struct {
    arraylist *   constants_pool;
    symbol_table *symbol_table;
    arraylist *   scopes;
    size_t        scope_index;
} compiler;

typedef struct {
    instructions *instructions;
    arraylist *   constants_pool;
} bytecode;

typedef enum compiler_error_code {
    COMPILER_ERROR_NONE,
    COMPILER_UNKNOWN_OPERATOR,
    COMPILER_UNDEFINED_VARIABLE
} compiler_error_code;

typedef struct {
    compiler_error_code error_code;
    char *              msg;
} compiler_error;

static const char *compiler_errors[] = {
        "COMPILER_ERROR_NONE",
        "COMPILER_UNKNOWN_OPERATOR",
        "COMPILER_UNDEFINED_VARIABLE"
};


#define get_compiler_error(e) compiler_errors[e]

compiler *compiler_init(void);

compiler *compiler_init_with_state(const symbol_table *symbol_table, arraylist *constants);

void compiler_free(compiler *);

compiler_error compile(compiler *, ast_node *);

size_t add_constant(compiler *, object_object *);

bytecode *get_bytecode(const compiler *);

void bytecode_free(bytecode *);

void compiler_enter_scope(compiler *);

instructions *compiler_leave_scope(compiler *);

#endif //COMPILER_H
