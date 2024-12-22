#include "opcode.h"

#include <arpa/inet.h> // For big-endian conversions
#include <conversions.h>
#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

OpcodeDefinition * opcode_definition_lookup(const Opcode op) {
    if (op >= sizeof(opcode_definitions) / sizeof(OpcodeDefinition)) {
        return NULL;
    }
    return &opcode_definitions[op-1];
}

// Function to create instruction
instructions *opcode_make_instruction(Opcode op, size_t *operands) {
    OpcodeDefinition *def = opcode_definition_lookup(op);

    if (!def) {
        return NULL;
    }

    // Calculate instruction length
    char instruction_len = 1; // Opcode itself
    for (char i = 0; i < def->operand_count; i++) {
        instruction_len += def->operand_widths[i];
    }

    // Allocate memory for the instruction
    instructions *instruction = malloc(sizeof(instructions));
    if (!instruction) {
        err(EXIT_FAILURE, "Could not allocate memory for instruction");
        return NULL;
    }

    instruction->bytes = malloc(instruction_len);
    if (!instruction->bytes) {
        err(EXIT_FAILURE, "Could not allocate memory for instruction bytes");
        return NULL;
    }

    // Set the Opcode
    instruction->bytes[0] = op;

    // TODO: Check if operands array matches required operand_count
    // Set the operands
    int offset = 1;
    for (int i = 0; i < def->operand_count; i++) {
        int width = def->operand_widths[i];
        if (width == 2) {
            put_uint16_big_endian(&instruction->bytes[offset], 2, (uint16_t)operands[i]);
        } else if (width == 1) {
            instruction->bytes[offset] = (uint8_t)operands[i];
        }
        offset += width;
    }

    instruction->length = instruction->capacity = instruction_len;

    return instruction;
}

// Read operands from instruction
void read_operands(const OpcodeDefinition *def, const uint8_t *ins, size_t *operands, size_t *bytes_read) {
    *bytes_read = 0;
    for (size_t i = 0; i < def->operand_count; i++) {
        switch (def->operand_widths[i]) {
        case 2:
            operands[i] = ntohs(*(uint16_t *)(ins + *bytes_read));
            *bytes_read += 2;
            break;
        case 1:
            operands[i] = ins[*bytes_read];
            *bytes_read += 1;
            break;
        default:
            fprintf(stderr, "Unsupported operand width\n");
            break;
        }
    }
}

void concat_instructions(instructions *dst, instructions *src) {
    // Ensure there is enough space in `dst->bytes` to accommodate `src->bytes`
    if (dst->capacity == 0 || dst->capacity - dst->length < src->length) {
        if (dst->capacity == 0) {
            dst->capacity = src->length * 2;
        } else {
            dst->capacity = dst->capacity * 2 + src->length;
        }

        // Safely handle memory reallocation
        void *new_bytes = reallocarray(dst->bytes, dst->capacity, sizeof(*dst->bytes));
        if (new_bytes == NULL) {
            free(dst->bytes); // Prevent memory leak
            err(EXIT_FAILURE, "Could not allocate memory for instructions");
        }
        dst->bytes = new_bytes;
    }

    // Use memcpy to append `src->bytes` to `dst->bytes`
    memcpy(dst->bytes + dst->length, src->bytes, src->length);
    dst->length += src->length; // Update the length of `dst`
}

instructions *opcode_flatten_instructions(size_t n, instructions *ins_array[n]) {
    size_t bytes_len = 0;
    for (size_t i = 0; i < n; i++) {
        if (!ins_array[i]) {
            err(EXIT_FAILURE, "Null instruction encountered in array");
        }
        bytes_len += ins_array[i]->length;
    }

    uint8_t *bytes = malloc(bytes_len);
    if (!bytes) {
        err(EXIT_FAILURE, "Failed to allocate memory for flattened instructions");
    }

    size_t bytes_offset = 0;
    for (size_t i = 0; i < n; i++) {
        instructions *ins = ins_array[i];
        memcpy(bytes + bytes_offset, ins->bytes, ins->length);
        bytes_offset += ins->length;
    }

    instructions *flat_ins = malloc(sizeof(*flat_ins));
    if (!flat_ins) {
        free(bytes);
        err(EXIT_FAILURE, "Failed to allocate memory for flattened instructions object");
    }

    flat_ins->bytes = bytes;
    flat_ins->length = bytes_len;
    flat_ins->capacity = bytes_len;
    return flat_ins;
}

char * instructions_to_string(instructions *instructions) {
    char *string = NULL;
    for (size_t i = 0; i < instructions->length; i++) {
        Opcode op = instructions->bytes[i];
        size_t operand;
        OpcodeDefinition *op_def = opcode_definition_lookup(op);
        switch (op) {
        case OP_CONSTANT:
        case OP_JUMP_NOT_TRUTHY:
        case OP_JUMP:
        case OP_SET_GLOBAL:
        case OP_GET_GLOBAL:
        case OP_ARRAY:
        case OP_HASH:
            operand = be_to_size_t(instructions->bytes + i + 1);
            if (string == NULL) {
                int retval = asprintf(&string, "%04zu %s %zu", i, op_def->name, operand);
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
            } else {
                char *temp = NULL;
                int retval = asprintf(&temp, "%s\n%04zu %s %zu", string, i, op_def->name, operand);
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
                free(string);
                string = temp;
            }
            i += 2;
            break;
        case OP_SET_LOCAL:
        case OP_GET_LOCAL:
        case OP_CALL:
        case OP_GET_BUILTIN:
        case OP_GET_FREE:
            operand = be_to_size_t_1(instructions->bytes + i + 1);
            if (string == NULL) {
                int retval = asprintf(&string, "%04zu %s %zu", i, op_def->name, operand);
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
            } else {
                char *temp = NULL;
                int retval = asprintf(&temp, "%s\n%04zu %s %zu", string, i, op_def->name, operand);
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
                free(string);
                string = temp;
            }
            i++;
            break;
        case OP_CLOSURE:
            operand = be_to_size_t(instructions->bytes + i + 1)    ;
            if (string == NULL) {
                int retval = asprintf(&string, "%04zu %s %zu", i, op_def->name, operand);
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
            } else {
                char *temp = NULL;
                int retval = asprintf(&temp, "%s\n%04zu %s %zu", string, i, op_def->name, operand);
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
                free(string);
                string = temp;
            }
            i += 2;
            operand = be_to_size_t_1(instructions->bytes + i + 1);
            if (string == NULL) {
                int retval = asprintf(&string, " %zu", operand);
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
            } else {
                char *temp = NULL;
                int retval = asprintf(&temp, "%s %zu", string, operand);
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
                free(string);
                string = temp;
            }
            i++;
            break;
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_TRUE:
        case OP_FALSE:
        case OP_GREATER_THAN:
        case OP_EQUAL:
        case OP_NOT_EQUAL:
        case OP_MINUS:
        case OP_BANG:
        case OP_NULL:
        case OP_INDEX:
        case OP_RETURN:
        case OP_RETURN_VALUE:
        case OP_CURRENT_CLOSURE:
            if (string == NULL) {
                int retval = asprintf(&string, "%04zu %s", i, op_def->name);
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
            } else {
                char *temp = NULL;
                int retval = asprintf(&temp, "%s\n%04zu %s", string, i, op_def->name);
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
                free(string);
                string = temp;
            }
            break;
        case OP_POP:
            if (string == NULL) {
                int retval = asprintf(&string, "%04zu %s", i, "OPPOP");
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
            } else {
                char *temp = NULL;
                int retval = asprintf(&temp, "%s\n%04zu %s", string, i, "OPPOP");
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
                free(string);
                string = temp;
            }
            break;
        default:
            return "";
        }
    }
    return string;
}

void instructions_free(instructions *ins) {
    if (!ins) {
        return;
    }
    if (ins->bytes) {
        free(ins->bytes);
        ins->bytes = NULL;
    }
    ins->capacity = ins->length = 0;
    free(ins);
}



instructions *opcode_copy_instructions(instructions *ins) {
    if (!ins || !ins->bytes || ins->length == 0) {
        err(EXIT_FAILURE, "Invalid input instructions");
    }

    // Validate that capacity is consistent with length
    if (ins->capacity < ins->length) {
        err(EXIT_FAILURE, "Instruction capacity is smaller than its length");
    }

    // Allocate memory for the new instructions structure
    instructions *ret = malloc(sizeof(*ret));
    if (!ret) {
        err(EXIT_FAILURE, "Failed to allocate memory for instructions");
    }

    // Allocate memory for the instruction bytes
    ret->bytes = malloc(ins->length);
    if (!ret->bytes) {
        free(ret); // Prevent memory leak
        err(EXIT_FAILURE, "Failed to allocate memory for instruction bytes");
    }

    // Copy the instruction bytes
    memcpy(ret->bytes, ins->bytes, ins->length);

    // Copy the length and capacity
    ret->length = ins->length;
    ret->capacity = ins->capacity;

    return ret;
}


Opcode vm_instruction_decode(const uint8_t *bytes, size_t *operands) {
    if (!bytes) {
        return OP_INVALID; // Return an invalid opcode if input is NULL
    }

    Opcode op = (Opcode)bytes[0];
    OpcodeDefinition *def = opcode_definition_lookup(op);

    if (!def) {
        return OP_INVALID; // Return invalid opcode if not found
    }

    size_t offset = 1;
    for (size_t i = 0; i < def->operand_count; i++) {
        operands[i] = 0;
        for (size_t j = 0; j < def->operand_widths[i]; j++) {
            operands[i] <<= 8;
            operands[i] |= bytes[offset++];
        }
    }

    return op;
}