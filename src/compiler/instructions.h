//
// Created by dgood on 12/19/24.
//

#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H
#include <stddef.h>

#include "compiler_core.h"

instructions * get_current_instructions(const compiler *compiler);
bool last_instruction_is(const compiler *compiler, const Opcode opcode);
void remove_last_instruction(const compiler *compiler);
void replace_instruction(const compiler *compiler, const size_t position, const instructions *ins);
void change_operand(const compiler *compiler, const size_t op_pos, const size_t operand);
void replace_last_pop_with_return(const compiler *compiler);
void load_symbol(const compiler * compiler, const symbol *symbol);
size_t emit(const compiler *, Opcode, ...);
#endif //INSTRUCTIONS_H
