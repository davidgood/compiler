//
// Created by dgood on 12/18/24.
//
#include "../Unity/src/unity.h"
#include "../src/opcode/opcode.h"
#include "../src/datastructures/conversions.h"
#include "test_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

// Helper function for creating dummy operands
size_t *create_operands(size_t count, ...) {
    va_list ap;
    size_t *operands = malloc(count * sizeof(size_t));
    va_start(ap, count);
    for (size_t i = 0; i < count; i++) {
        operands[i] = va_arg(ap, size_t);
    }
    va_end(ap);
    return operands;
}

void test_opcode_definition_lookup(void) {
    OpcodeDefinition *def = opcode_definition_lookup(OP_CONSTANT);
    TEST_ASSERT_NOT_NULL(def);
    TEST_ASSERT_EQUAL_STRING("OP_CONSTANT", def->name);
    TEST_ASSERT_EQUAL(1, def->operand_count);
    TEST_ASSERT_EQUAL(2, def->operand_widths[0]);

    def = opcode_definition_lookup(OP_ADD);
    TEST_ASSERT_NOT_NULL(&def);
    TEST_ASSERT_EQUAL_STRING("OP_ADD", def->name);
    TEST_ASSERT_EQUAL(0, def->operand_count);
}

void test_vm_instruction_init(void) {
    size_t operands[] = {65534};
    instructions *ins = opcode_make_instruction(OP_CONSTANT, operands);

    TEST_ASSERT_NOT_NULL(ins);
    TEST_ASSERT_EQUAL(3, ins->size);
    TEST_ASSERT_EQUAL(3, ins->length);
    TEST_ASSERT_EQUAL_UINT8(OP_CONSTANT, ins->bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(0xff, ins->bytes[1]);
    TEST_ASSERT_EQUAL_UINT8(0xfe, ins->bytes[2]);

    free(ins->bytes);
    free(ins);
}

void test_vm_instruction_encode(void) {
    size_t operands[] = {256};
    instructions *ins = opcode_make_instruction(OP_CONSTANT, operands);

    TEST_ASSERT_NOT_NULL(ins);
    TEST_ASSERT_EQUAL_UINT8(OP_CONSTANT, ins->bytes[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01, ins->bytes[1]);
    TEST_ASSERT_EQUAL_UINT8(0x00, ins->bytes[2]);

    free(ins->bytes);
    free(ins);
}

void test_vm_instruction_decode(void) {
    uint8_t bytes[] = {OP_CONSTANT, 0xff, 0xff};
    size_t operands[1];

    Opcode op = vm_instruction_decode(bytes, operands);

    TEST_ASSERT_EQUAL(OP_CONSTANT, op);
    TEST_ASSERT_EQUAL(65535, operands[0]);
}

void test_invalid_opcode(void) {
    OpcodeDefinition *def = opcode_definition_lookup(255);
    TEST_ASSERT_NULL(def);
}

void test_instructions(size_t size, uint8_t *expected, uint8_t *actual) {
    for (size_t i = 0; i < size; i++)
        TEST_ASSERT_EQUAL_UINT8(expected[i],actual[i]);
}

void test_instruction_init(void) {
    typedef struct {
        const char *desc;
        Opcode      op;
        size_t      operands[MAX_OPERANDS];
        size_t      expected_instructions_len;
        uint8_t    *expected_instructions;
    } test;

    const test test_cases[] = {
        {
            "Testing OP_CONSTANT 65534",
            OP_CONSTANT, {(size_t) 65534},
            3,
            create_uint8_array(4, OP_CONSTANT, 255, 254)
        },
        {
            "Test OP_ADD",
            OP_ADD,
            {},
            1,
            create_uint8_array(1, OP_ADD)
        },
        {
            "Test OP_SET_LOCAL 255",
            OP_SET_LOCAL, {(size_t) 255},
            2,
            create_uint8_array(2, OP_SET_LOCAL, 255)
        },
        {
            "Test OP_CLOSURE 65534 255",
            OP_CLOSURE, {(size_t) 65534, (size_t) 255},
            4,
            create_uint8_array(4, OP_CLOSURE, 255, 254, 255)
        }
    };
    print_test_separator_line();
    printf("Testing instructions init\n");

    size_t ntests = sizeof(test_cases) / sizeof(test_cases[0]);
    for (size_t i = 0; i < ntests; i++) {
        test t = test_cases[i];
        printf("%s\n", t.desc);
        instructions *actual;
        if (t.op != OP_CLOSURE)
            actual = opcode_make_instruction(t.op, t.operands);
        else
            actual = opcode_make_instruction(t.op, t.operands);
        const size_t actual_len   = actual->length;
        const size_t expected_len = t.expected_instructions_len;
        TEST_ASSERT_EQUAL_INT(actual_len, expected_len);
        test_instructions(actual_len, t.expected_instructions, actual->bytes);
        instructions_free(actual);
        free(t.expected_instructions);
    }
}

void test_instructions_string(void) {
    instructions *ins_array[5] = {
        opcode_make_instruction(OP_ADD, 0),
        opcode_make_instruction(OP_CONSTANT, (size_t[]){2}),
        opcode_make_instruction(OP_CONSTANT, (size_t[]){65535}),
        opcode_make_instruction(OP_GET_LOCAL, (size_t[]){1}),
        opcode_make_instruction(OP_CLOSURE, (size_t[]){65535, 255})
    };

    const char *expected_string = "0000 OP_ADD\n" \
        "0001 OP_CONSTANT 2\n" \
        "0004 OP_CONSTANT 65535\n" \
        "0007 OP_GET_LOCAL 1\n" \
        "0009 OP_CLOSURE 65535 255";

    instructions *flat_ins = opcode_flatten_instructions(5, ins_array);
    char *string = instructions_to_string(flat_ins);
    print_test_separator_line();
    printf("Testing instructions_to_string\n");
    TEST_ASSERT_EQUAL_STRING(string, expected_string);
    free(string);
    instructions_free(flat_ins);
    instructions_free(ins_array[0]);
    instructions_free(ins_array[1]);
    instructions_free(ins_array[2]);
    instructions_free(ins_array[3]);
    instructions_free(ins_array[4]);
}


int main(void) {
    UNITY_BEGIN();

//    RUN_TEST(test_opcode_definition_lookup);
//    RUN_TEST(test_vm_instruction_init);
//    RUN_TEST(test_vm_instruction_encode);
//    RUN_TEST(test_vm_instruction_decode);
//    RUN_TEST(test_invalid_opcode);
    RUN_TEST(test_instructions_string);
    RUN_TEST(test_instruction_init);

    return UNITY_END();
}
