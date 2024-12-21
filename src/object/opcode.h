#ifndef OPCODE_H
#define OPCODE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_OPERANDS 16

typedef struct {
    uint8_t *bytes; // Pointer to the instruction byte array
    size_t size;    // Current size of the byte array
    size_t length;  // Number of bytes currently used
} instructions;

// Opcode enumeration
typedef enum {
    OP_CONSTANT = 1,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_POP,
    OP_TRUE,
    OP_FALSE,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_GREATER_THAN,
    OP_MINUS,
    OP_BANG,
    OP_JUMP_NOT_TRUTHY,
    OP_JUMP,
    OP_NULL,
    OP_SET_GLOBAL,
    OP_GET_GLOBAL,
    OP_ARRAY,
    OP_HASH,
    OP_INDEX,
    OP_CALL,
    OP_RETURN_VALUE,
    OP_RETURN,
    OP_SET_LOCAL,
    OP_GET_LOCAL,
    OP_GET_BUILTIN,
    OP_CLOSURE,
    OP_GET_FREE,
    OP_CURRENT_CLOSURE,
    OP_INVALID
} Opcode;

// Instruction definition
typedef struct {
    const char *name;
    const char *desc;
    size_t operand_widths[MAX_OPERANDS]; // Maximum of two operands
    size_t operand_count;
} OpcodeDefinition;

static OpcodeDefinition opcode_definitions[] = {
    {"OP_CONSTANT", "constant", {(size_t) 2}},
    {"OP_ADD", "+", {(size_t) 0}},
    {"OP_SUB", "-", {(size_t) 0}},
    {"OP_MUL", "*", {(size_t) 0}},
    {"OP_DIV", "/", {(size_t) 0}},
    {"OP_POP", "pop", {(size_t) 0}},
    {"OP_TRUE", "true", {(size_t) 0}},
    {"OP_FALSE", "false", {(size_t) 0}},
    {"OP_EQUAL", "==", {(size_t) 0}},
    {"OP_NOT_EQUAL", "!=", {(size_t) 0}},
    {"OP_GREATER_THAN", ">", {(size_t) 0}},
    {"OP_MINUS", "-", {(size_t) 0}},
    {"OP_BANG", "not", {(size_t) 0}},
    {"OP_JUMP_NOT_TRUTHY", "jump_if_false", {(size_t) 2}},
    {"OP_JUMP", "jump", {(size_t) 2}},
    {"OP_NULL", "null", {(size_t) 0}},
    {"OP_SET_GLOBAL", "set_global", {(size_t) 2}},
    {"OP_GET_GLOBAL", "get_global", {(size_t) 2}},
    {"OP_ARRAY", "array", {(size_t) 2}},
    {"OP_HASH", "hash", {(size_t) 2}},
    {"OP_INDEX", "index", {(size_t) 0}},
    {"OP_CALL", "call", {(size_t) 1}},
    {"OP_RETURN_VALUE", "return_value", {(size_t) 0}},
    {"OP_RETURN", "return", {(size_t) 0}},
    {"OP_SET_LOCAL", "set_local", {(size_t) 1}},
    {"OP_GET_LOCAL", "get_local", {(size_t) 1}},
    {"OP_GET_BUILTIN", "get_builtin", {(size_t) 1}},
    {"OP_CLOSURE", "closure", {(size_t) 2, (size_t) 1}},
    {"OP_GET_FREE", "get_free", {(size_t) 1}},
    {"OP_CURRENT_CLOSURE", "current_closure", {(size_t) 0}},
    {"OP_INVALID", "invalid", {(size_t) 0}}
};

// Functions
instructions *instruction_init(Opcode, ...);
instructions *opcode_make_instruction(Opcode op, const va_list ap);
void read_operands(const OpcodeDefinition *def, const uint8_t *ins, size_t *operands, size_t *bytes_read);
void concat_instructions(instructions *, const instructions *);
instructions * opcode_flatten_instructions(size_t n, instructions *ins_array[n]);
char *instructions_to_string(instructions *);
void instructions_free(instructions *);
instructions *opcode_copy_instructions(instructions *);
OpcodeDefinition *opcode_definition_lookup(Opcode op);
Opcode vm_instruction_decode(const uint8_t *bytes, size_t *operands);


#endif // OPCODE_H
