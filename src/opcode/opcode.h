#ifndef OPCODE_H
#define OPCODE_H

#include <stddef.h>
#include <stdint.h>

#define MAX_OPERANDS 16

typedef struct {
    uint8_t *bytes;    // Pointer to the instruction byte array
    size_t   capacity; // Current size of the byte array
    size_t   length;   // Number of bytes currently used
} instructions;

// Opcode enumeration, 1 byte
typedef enum : char {
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
    int         operand_widths[MAX_OPERANDS]; // Maximum of two operands
    int         operand_count;
} OpcodeDefinition;

static OpcodeDefinition opcode_definitions[] = {
    {"OP_CONSTANT", "constant", {2}, 1},
    {"OP_ADD", "+", {0}, 0},
    {"OP_SUB", "-", {0}, 0},
    {"OP_MUL", "*", {0}, 0},
    {"OP_DIV", "/", {0}, 0},
    {"OP_POP", "pop", {0}, 0},
    {"OP_TRUE", "true", {0}, 0},
    {"OP_FALSE", "false", {0}, 0},
    {"OP_EQUAL", "==", {0}, 0},
    {"OP_NOT_EQUAL", "!=", {0}, 0},
    {"OP_GREATER_THAN", ">", {0}, 0},
    {"OP_MINUS", "-", {0}, 0},
    {"OP_BANG", "not", {0}, 0},
    {"OP_JUMP_NOT_TRUTHY", "jump_if_false", {2}, 1},
    {"OP_JUMP", "jump", {2}, 1},
    {"OP_NULL", "null", {0}, 0},
    {"OP_SET_GLOBAL", "set_global", {2}, 1},
    {"OP_GET_GLOBAL", "get_global", {2}, 1},
    {"OP_ARRAY", "array", {2}, 1},
    {"OP_HASH", "hash", {2}, 1},
    {"OP_INDEX", "index", {0}, 0},
    {"OP_CALL", "call", {1}, 1},
    {"OP_RETURN_VALUE", "return_value", {0}, 0},
    {"OP_RETURN", "return", {0}, 0},
    {"OP_SET_LOCAL", "set_local", {1}, 1},
    {"OP_GET_LOCAL", "get_local", {1}, 1},
    {"OP_GET_BUILTIN", "get_builtin", {1}, 1},
    {"OP_CLOSURE", "closure", {2, 1}, 2},
    {"OP_GET_FREE", "get_free", {1}, 1},
    {"OP_CURRENT_CLOSURE", "current_closure", {0}, 0},
    {"OP_INVALID", "invalid", {0}, 0}
};

// Functions
//instructions *instruction_init(Opcode, size_t *operands, size_t operand_count);
instructions *opcode_make_instruction(Opcode op, size_t *operands);

void read_operands(const OpcodeDefinition *def, const uint8_t *ins, size_t *operands, size_t *bytes_read);

void concat_instructions(instructions *, instructions *);

instructions *opcode_flatten_instructions(size_t n, instructions *ins_array[n]);

char *instructions_to_string(instructions *);

void instructions_free(instructions *);

instructions *opcode_copy_instructions(instructions *);

OpcodeDefinition *opcode_definition_lookup(Opcode op);

Opcode vm_instruction_decode(const uint8_t *bytes, size_t *operands);


#endif // OPCODE_H
