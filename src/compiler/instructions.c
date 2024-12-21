//
// Created by dgood on 12/19/24.
//
#include "instructions.h"
#include <stdarg.h>

#include "scope.h"


instructions * get_current_instructions(const compiler *compiler) {
    return get_top_scope(compiler)->instructions;
}

bool last_instruction_is(const compiler *compiler, const Opcode opcode) {
    if (get_current_instructions(compiler)->length == 0)
        return false;
    return get_top_scope(compiler)->last_instruction.opcode == opcode;
}

void remove_last_instruction(const compiler *compiler) {
    compilation_scope *scope = get_top_scope(compiler);
    scope->instructions->length = scope->last_instruction.position;
    scope->last_instruction.opcode = scope->prev_instruction.opcode;
    scope->last_instruction.position = scope->prev_instruction.position;
}

size_t add_instructions(const compiler *compiler, instructions *ins) {
    const compilation_scope *scope       = get_top_scope(compiler);
    const size_t new_ins_pos = scope->instructions->length;
    concat_instructions(scope->instructions, ins);
    instructions_free(ins);
    return new_ins_pos;
}

static void set_last_instruction(const compiler *compiler, Opcode opcode, size_t pos) {
    compilation_scope *scope = get_top_scope(compiler);
    scope->prev_instruction.opcode = scope->last_instruction.opcode;
    scope->prev_instruction.position = scope->last_instruction.position;
    scope->last_instruction.opcode = opcode;
    scope->last_instruction.position = pos;
}

void replace_instruction(const compiler *compiler, const size_t position, const instructions *ins) {
    const compilation_scope *scope = get_top_scope(compiler);
    for (size_t i = 0; i < ins->length; i++) {
        scope->instructions->bytes[position + i] = ins->bytes[i];
    }
}

void change_operand(const compiler *compiler, const size_t op_pos, const size_t operand) {
    const compilation_scope *scope   = get_top_scope(compiler);
    const Opcode op = scope->instructions->bytes[op_pos];
    instructions *new_ins = instruction_init(op, operand);
    replace_instruction(compiler, op_pos, new_ins);
    instructions_free(new_ins);
}

void replace_last_pop_with_return(const compiler *compiler) {
    compilation_scope *top_scope = get_top_scope(compiler);
    const size_t lastpos = top_scope->last_instruction.position;
    instructions *new_ins = instruction_init(OP_RETURN_VALUE);
    replace_instruction(compiler, lastpos, new_ins);
    instructions_free(new_ins);
    top_scope->last_instruction.opcode = OP_RETURN_VALUE;
}

void load_symbol(const compiler * compiler, const symbol *symbol) {
    switch (symbol->scope) {
        case GLOBAL:
            emit(compiler, OP_GET_GLOBAL, symbol->index);
        break;
        case LOCAL:
            emit(compiler, OP_GET_LOCAL, symbol->index);
        break;
        case BUILTIN:
            emit(compiler, OP_GET_BUILTIN, symbol->index);
        break;
        case FREE:
            emit(compiler, OP_GET_FREE, symbol->index);
        break;
        case FUNCTION_SCOPE:
            emit(compiler, OP_CURRENT_CLOSURE, symbol->index);
        break;
    }
}

bytecode *get_bytecode(const compiler *compiler) {
    bytecode                *bytecode = malloc(sizeof(*bytecode));
    const compilation_scope *scope = get_top_scope(compiler);
    bytecode->instructions = opcode_copy_instructions(scope->instructions);
    bytecode->constants_pool = compiler->constants_pool;
    return bytecode;
}

void bytecode_free(bytecode *bytecode) {
    if (!bytecode) {
        return;
    }
    if (bytecode->instructions) {
        instructions_free(bytecode->instructions);
        bytecode->instructions = NULL;
    }
    free(bytecode);
}

size_t emit(const compiler *compiler, const Opcode op, ...) {
    va_list ap;
    va_start(ap, op);
    instructions *ins         = opcode_make_instruction(op, ap);
    const size_t new_ins_pos = add_instructions(compiler, ins);
    va_end(ap);
    set_last_instruction(compiler, op, new_ins_pos);
    return new_ins_pos;
}