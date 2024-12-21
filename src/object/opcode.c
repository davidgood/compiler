#include "opcode.h"

#include <arpa/inet.h> // For big-endian conversions
#include <conversions.h>
#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Lookup opcode definition
const OpcodeDefinition *lookup_opcode_definition(Opcode op) {
    if (op >= sizeof(opcode_definitions) / sizeof(OpcodeDefinition)) {
        return NULL;
    }
    return &opcode_definitions[op];
}

// Generate an instruction
instructions *opcode_make_instruction(Opcode op, const va_list ap) {
    size_t operand;
    instructions    *ins = malloc(sizeof(*ins));
    if (ins == NULL) {
        errx(EXIT_FAILURE, "malloc failed");
    }
    switch (op) {
    case OP_CONSTANT:
    case OP_JUMP_NOT_TRUTHY:
    case OP_JUMP:
    case OP_SET_GLOBAL:
    case OP_GET_GLOBAL:
    case OP_ARRAY:
    case OP_HASH:
        // these opcodes need only one operand 2 bytes wide
        operand = va_arg(ap, size_t);
        uint8_t *boperand = size_t_to_uint8_be(operand, 2);
        ins->bytes = create_uint8_array(3, op, boperand[0], boperand[1]);
        ins->length = 3;
        ins->size = 3;
        free(boperand);
        return ins;
    case OP_SET_LOCAL:
    case OP_GET_LOCAL:
    case OP_CALL:
    case OP_GET_BUILTIN:
    case OP_GET_FREE:
        operand = va_arg(ap, size_t);
        boperand = size_t_to_uint8_be(operand, 1);
        ins->bytes = create_uint8_array(2, op, boperand[0]);
        ins->length = 2;
        ins->size = 2;
        free(boperand);
        return ins;
    case OP_CLOSURE:
        operand = va_arg(ap, size_t);
        boperand = size_t_to_uint8_be(operand, 2);
        ins->bytes = create_uint8_array(4, op, boperand[0], boperand[1], 0);
        free(boperand);
        operand = va_arg(ap, size_t);
        boperand = size_t_to_uint8_be(operand, 1);
        ins->bytes[3] = boperand[0];
        ins->size = 4;
        ins->length = 4;
        free(boperand);
        return ins;
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
    case OP_POP:
    case OP_TRUE:
    case OP_FALSE:
    case OP_GREATER_THAN:
    case OP_EQUAL:
    case OP_NOT_EQUAL:
    case OP_MINUS:
    case OP_BANG:
    case OP_NULL:
    case OP_INDEX:
    case OP_RETURN_VALUE:
    case OP_RETURN:
    case OP_CURRENT_CLOSURE:
        ins->bytes = create_uint8_array(1, op);
        ins->length = 1;
        ins->size = 1;
        return ins;
    default:
        const OpcodeDefinition *op_def = opcode_definition_lookup(op);
        errx(EXIT_FAILURE, "Unsupported opcode %s", op_def->name);
    }
    return ins;
}

instructions * instruction_init(Opcode op, ...) {
    va_list ap;
    va_start(ap, op);
    instructions *ins = opcode_make_instruction(op, ap);
    va_end(ap);
    return ins;
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

void concat_instructions(instructions *dst, const instructions *src) {
    if (dst->size == 0 || dst->size - dst->length < src->length) {
        dst->size = dst->size * 2 + src->length;
        dst->bytes = reallocarray(dst->bytes, dst->size, sizeof(*dst->bytes));
        if (dst->bytes == NULL)
            errx(EXIT_FAILURE, "Could not allocate memory for instructions");
    }
    for (size_t i = 0; i < src->length; i++)
        dst->bytes[dst->length++] = src->bytes[i];
}

instructions *opcode_flatten_instructions(size_t n, instructions *ins_array[n]) {
    size_t bytes_len = 0;
    for (size_t i = 0; i < n; i++) {
        if (!ins_array[i]) {
            errx(EXIT_FAILURE, "Null instruction encountered in array");
        }
        bytes_len += ins_array[i]->length;
    }

    uint8_t *bytes = malloc(bytes_len);
    if (!bytes) {
        errx(EXIT_FAILURE, "Failed to allocate memory for flattened instructions");
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
    flat_ins->size = bytes_len;
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
    free(ins);
}

instructions * opcode_copy_instructions(instructions *ins) {
    instructions *ret = malloc(sizeof(*ret));
    if (ret == NULL) {
        errx(EXIT_FAILURE, "malloc failed");
    }
    ret->bytes = malloc(ins->length);
    if (ret->bytes == NULL) {
        errx(EXIT_FAILURE, "malloc failed");
    }
    memcpy(ret->bytes, ins->bytes, ins->length);

    ret->length = ins->length;
    ret->size = ins->size;
    return ret;
}

OpcodeDefinition * opcode_definition_lookup(const Opcode op) {
    if (op >= sizeof(opcode_definitions) / sizeof(OpcodeDefinition)) {
        return NULL;
    }
    return &opcode_definitions[op-1];
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