//
// Created by dgood on 12/18/24.
//
#include <err.h>
#include <stdarg.h>
#include <string.h>
#include "../Unity/src/unity.h"
#include "../src/ast/ast.h"
#include "../src/ast/ast_debug_print.h"
#include "../src/compiler/compiler_core.h"
#include "../src/compiler/instructions.h"
#include "../src/compiler/scope.h"
#include "../src/datastructures/arraylist.h"
#include "../src/object/object.h"
#include "../src/opcode/opcode.h"
#include "../src/parser/parser.h"
#include "object_test_utils.h"
#include "test_utils.h"


typedef struct compiler_test {
    const char *  input;
    size_t        instructions_count;
    instructions *expected_instructions[32];
    arraylist *   expected_constants;
} compiler_test;

arraylist *tracked_instructions = NULL;

static void _instructions_free(void *ins) {
    instructions_free(ins);
}

void setUp(void) {
    if (tracked_instructions != NULL) {
        arraylist_destroy(tracked_instructions)
    }
    tracked_instructions = arraylist_create(32, _instructions_free);
}


void tearDown(void) {
    if (tracked_instructions != NULL) {
        arraylist_destroy(tracked_instructions);
        tracked_instructions = NULL;
    }
}

static instructions *opcode_make_instruction_and_track(const Opcode op, size_t *operands) {

    instructions *ins = opcode_make_instruction(op, operands);
    arraylist_add(tracked_instructions, ins);

    return ins;
}

static void run_compiler_tests(compiler_test *);

static arraylist *create_constant_pool(const size_t count, ...) {
    va_list ap;
    va_start(ap, count);
    arraylist *list = arraylist_create(count, object_free);
    for (size_t i = 0; i < count; i++) {
        object_object *obj = va_arg(ap, object_object *);
        arraylist_add(list, obj);
    }
    va_end(ap);
    return list;
}


static void test_instructions(const size_t count, uint8_t *expected, uint8_t *actual) {
    printf("* Testing Instructions\n");
    instructions expected_ins = {expected, count};
    instructions actual_ins   = {actual, count};
    for (size_t i = 0; i < count; i++) {
        char *expected_ins_string = instructions_to_string(&expected_ins);
        char *actual_ins_string   = instructions_to_string(&actual_ins);
        TEST_ASSERT_EQUAL_UINT8(expected[i], actual[i]);
        free(expected_ins_string);
        free(actual_ins_string);
    }
}

static void test_constant_pool_and_free(arraylist *expected, const arraylist *actual) {
    printf("* Testing Constant Pool\n");
    if (expected == NULL) {
        TEST_ASSERT_NULL(actual);
    } else {
        TEST_ASSERT_EQUAL_UINT(expected->size, actual->size);
        for (size_t i = 0; i < expected->size; i++) {
            object_object *expected_obj = arraylist_get(expected, i);
            object_object *actual_obj   = arraylist_get(actual, i);
            TEST_ASSERT_EQUAL_INT(expected_obj->type, actual_obj->type);
            test_object_object(actual_obj, expected_obj);
        }
    }
    arraylist_destroy(expected);
}

static void test_compiler_scopes(void) {
    printf("Testing compiler scopes\n");
    print_test_separator_line();
    compiler *    compiler            = compiler_init();
    symbol_table *global_symbol_table = compiler->symbol_table;
    TEST_ASSERT_EQUAL_INT(compiler->scope_index, 0);
    emit(compiler, OP_MUL, 0);
    compiler_enter_scope(compiler);
    TEST_ASSERT_EQUAL_INT(compiler->scope_index, 1);
    emit(compiler, OP_SUB, 0);
    const compilation_scope *scope = get_top_scope(compiler);
    TEST_ASSERT_NOT_NULL(scope);
    TEST_ASSERT_EQUAL_INT(scope->instructions->length, 1);
    TEST_ASSERT_EQUAL_INT(scope->last_instruction.opcode, OP_SUB);
    TEST_ASSERT_EQUAL_PTR(compiler->symbol_table->outer, global_symbol_table);
    instructions *ins = compiler_leave_scope(compiler);
    TEST_ASSERT_EQUAL_INT(compiler->scope_index, 0);
    TEST_ASSERT_EQUAL_PTR(compiler->symbol_table, global_symbol_table);
    TEST_ASSERT_NULL(compiler->symbol_table->outer);
    instructions_free(ins);
    emit(compiler, OP_ADD, 0);
    scope = get_top_scope(compiler);
    TEST_ASSERT_EQUAL_INT(scope->instructions->length, 2);
    TEST_ASSERT_EQUAL_INT(scope->last_instruction.opcode, OP_ADD);
    TEST_ASSERT_EQUAL_INT(scope->prev_instruction.opcode, OP_MUL);
    compiler_free(compiler);
}


/***************************************************************
************************* ARITHMETIC ***************************
 ***************************************************************/
void test_addition(void) {
    compiler_test test;
    test.input                    = "1 + 2";
    test.instructions_count       = 4;
    test.expected_instructions[0] = opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0});
    test.expected_instructions[1] = opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1});
    test.expected_instructions[2] = opcode_make_instruction_and_track(OP_ADD, 0);
    test.expected_instructions[3] = opcode_make_instruction_and_track(OP_POP, 0);

    arraylist *constants = arraylist_create(2, object_free);
    arraylist_add(constants, object_create_int(1));
    arraylist_add(constants, object_create_int(2));
    test.expected_constants = constants;

    printf("Testing addition\n");
    run_compiler_tests(&test);
}


static void test_multiple_expressions(void) {
    compiler_test test;
    test.input                      = "1; 2",
            test.instructions_count = 4;
    test.expected_instructions[0]   = opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0});
    test.expected_instructions[1]   = opcode_make_instruction_and_track(OP_POP, 0);
    test.expected_instructions[2]   = opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1});
    test.expected_instructions[3]   = opcode_make_instruction_and_track(OP_POP, 0);
    test.expected_constants         = create_constant_pool(2, object_create_int(1), object_create_int(2));

    printf("Testing multiple expressions\n");
    run_compiler_tests(&test);
}

static void test_subtraction(void) {
    compiler_test test;
    test.input                    = "1 - 2";
    test.instructions_count       = 4;
    test.expected_instructions[0] = opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0});
    test.expected_instructions[1] = opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1});
    test.expected_instructions[2] = opcode_make_instruction_and_track(OP_SUB, 0);
    test.expected_instructions[3] = opcode_make_instruction_and_track(OP_POP, 0);
    test.expected_constants       = create_constant_pool(2, object_create_int(1), object_create_int(2));

    printf("Testing subtraction\n");
    run_compiler_tests(&test);
}

static void test_multiplication(void) {
    compiler_test test;
    test.input                    = "1 * 2";
    test.instructions_count       = 4;
    test.expected_instructions[0] = opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0});
    test.expected_instructions[1] = opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1});
    test.expected_instructions[2] = opcode_make_instruction_and_track(OP_MUL, 0);
    test.expected_instructions[3] = opcode_make_instruction_and_track(OP_POP, 0);
    test.expected_constants       = create_constant_pool(2, object_create_int(1), object_create_int(2));

    printf("Testing multiplication\n");
    run_compiler_tests(&test);
}

static void test_division(void) {
    compiler_test test;
    test.input                    = "2 / 1";
    test.instructions_count       = 4;
    test.expected_instructions[0] = opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0});
    test.expected_instructions[1] = opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1});
    test.expected_instructions[2] = opcode_make_instruction_and_track(OP_DIV, 0);
    test.expected_instructions[3] = opcode_make_instruction_and_track(OP_POP, 0);
    test.expected_constants       = create_constant_pool(2, object_create_int(2), object_create_int(1));

    printf("Testing division\n");
    run_compiler_tests(&test);
}

static void test_negation(void) {
    compiler_test test;
    test.input                    = "-1";
    test.instructions_count       = 3;
    test.expected_instructions[0] = opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0});
    test.expected_instructions[1] = opcode_make_instruction_and_track(OP_MINUS, 0);
    test.expected_instructions[2] = opcode_make_instruction_and_track(OP_POP, 0);
    test.expected_constants       = create_constant_pool(1, object_create_int(1));

    printf("Testing negation\n");
    run_compiler_tests(&test);
}

/***************************************************************
********************** CONDITIONALS ****************************
 ***************************************************************/
static void test_if_true_then_block(void) {
    compiler_test test = {
            "if (true) {10}; 3333;",
            8,
            {opcode_make_instruction_and_track(OP_TRUE, 0),
             opcode_make_instruction_and_track(OP_JUMP_NOT_TRUTHY, (size_t[]){10}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_JUMP, (size_t[]){11}),
             opcode_make_instruction_and_track(OP_NULL, 0),
             opcode_make_instruction_and_track(OP_POP, 0),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1}),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(2,
                                 (object_object *) object_create_int(10),
                                 (object_object *) object_create_int(3333))
    };

    printf("Testing conditional: if (true) {10}; 3333;\n");
    run_compiler_tests(&test);

}

static void test_if_true_else_block(void) {
    compiler_test test = {
            "if (true) {10} else {20}; 3333;",
            8,
            {opcode_make_instruction_and_track(OP_TRUE, 0),
             opcode_make_instruction_and_track(OP_JUMP_NOT_TRUTHY, (size_t[]){10}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_JUMP, (size_t[]){13}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1}),
             opcode_make_instruction_and_track(OP_POP, 0),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){2}),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(3,
                                 (object_object *) object_create_int(10),
                                 (object_object *) object_create_int(20),
                                 (object_object *) object_create_int(3333))
    };

    printf("Testing conditional: if (true) {10} else {20}; 3333;\n");
    run_compiler_tests(&test);
}


/***************************************************************
************************** BOOLEANS ****************************
 ***************************************************************/
static void test_boolean_true(void) {
    compiler_test test = {
            "true",
            2,
            {opcode_make_instruction_and_track(OP_TRUE, 0),
             opcode_make_instruction_and_track(OP_POP, 0)},
            NULL
    };

    printf("Testing boolean expression: true\n");
    run_compiler_tests(&test);
}

static void test_boolean_false(void) {
    compiler_test test = {
            "false",
            2,
            {opcode_make_instruction_and_track(OP_FALSE, 0),
             opcode_make_instruction_and_track(OP_POP, 0)},
            NULL
    };

    printf("Testing boolean expression: false\n");
    run_compiler_tests(&test);
}

static void test_greater_than_expression(void) {
    compiler_test test = {
            "1 > 2",
            4,
            {opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1}),
             opcode_make_instruction_and_track(OP_GREATER_THAN, 0),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(2, object_create_int(1), object_create_int(2))
    };

    printf("Testing boolean expression: 1 > 2\n");
    run_compiler_tests(&test);
}

static void test_less_than_expression(void) {
    compiler_test test = {
            "1 < 2",
            4,
            {opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1}),
             opcode_make_instruction_and_track(OP_GREATER_THAN, 0),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(2, object_create_int(2), object_create_int(1))
    };

    printf("Testing boolean expression: 1 < 2\n");
    run_compiler_tests(&test);
}

static void test_equal_expression(void) {
    compiler_test test = {
            "1 == 2",
            4,
            {opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1}),
             opcode_make_instruction_and_track(OP_EQUAL, 0),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(2, object_create_int(1), object_create_int(2))
    };

    printf("Testing boolean expression: 1 == 2\n");
    run_compiler_tests(&test);
}

static void test_not_equal_expression(void) {
    compiler_test test = {
            "1 != 2",
            4,
            {opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1}),
             opcode_make_instruction_and_track(OP_NOT_EQUAL, 0),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(2, object_create_int(1), object_create_int(2))
    };

    printf("Testing boolean expression: 1 != 2\n");
    run_compiler_tests(&test);
}

static void test_true_equals_true(void) {
    compiler_test test = {
            "true == true",
            4,
            {opcode_make_instruction_and_track(OP_TRUE, 0),
             opcode_make_instruction_and_track(OP_TRUE, 0),
             opcode_make_instruction_and_track(OP_EQUAL, 0),
             opcode_make_instruction_and_track(OP_POP, 0)},
            NULL
    };

    printf("Testing boolean expression: true == true\n");
    run_compiler_tests(&test);
}

static void test_true_not_equal_false(void) {
    compiler_test test = {
            "true != false",
            4,
            {opcode_make_instruction_and_track(OP_TRUE, 0),
             opcode_make_instruction_and_track(OP_FALSE, 0),
             opcode_make_instruction_and_track(OP_NOT_EQUAL, 0),
             opcode_make_instruction_and_track(OP_POP, 0)},
            NULL
    };

    printf("Testing boolean expression: true != false\n");
    run_compiler_tests(&test);
}

static void test_bang_operator(void) {
    compiler_test test = {
            "!true",
            3,
            {opcode_make_instruction_and_track(OP_TRUE, 0),
             opcode_make_instruction_and_track(OP_BANG, 0),
             opcode_make_instruction_and_track(OP_POP, 0)},
            NULL
    };

    printf("Testing boolean expression: !true\n");
    run_compiler_tests(&test);
}

/***************************************************************
********************** LET STATEMENTS **************************
 ***************************************************************/

static void test_multiple_global_let_statements(void) {
    compiler_test test = {
            "let one = 1; let two = 2;",
            4,
            {opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_SET_GLOBAL, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1}),
             opcode_make_instruction_and_track(OP_SET_GLOBAL, (size_t[]){1})},
            create_constant_pool(2, object_create_int(1), object_create_int(2))
    };

    printf("Testing global let statements: let one = 1; let two = 2;\n");
    run_compiler_tests(&test);
}

static void test_global_let_and_usage(void) {
    compiler_test test = {
            "let one = 1; one;",
            4,
            {opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_SET_GLOBAL, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_GET_GLOBAL, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(1, object_create_int(1))
    };

    printf("Testing global let and usage: let one = 1; one;\n");
    run_compiler_tests(&test);
}

/***************************************************************
************************** STRINGS ****************************
 ***************************************************************/

static void test_single_string_expression(void) {
    compiler_test test = {
            "\"Lorem Ipsum\"",
            2,
            {opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(1, object_create_string("Lorem Ipsum", 11))
    };

    printf("Testing string expression: \"Lorem Ipsum\"\n");
    run_compiler_tests(&test);
}

static void test_string_concatenation_expression(void) {
    compiler_test test = {
            "\"Lorem\" + \" Ipsum\"",
            4,
            {opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1}),
             opcode_make_instruction_and_track(OP_ADD, 0),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(2, object_create_string("Lorem", 5),
                                 object_create_string(" Ipsum", 6))
    };

    printf("Testing string expression: \"Lorem\" + \" Ipsum\"\n");
    run_compiler_tests(&test);
}


/***************************************************************
*********************** HASH LITERALS **************************
 ***************************************************************/
static void test_empty_hash_literal(void) {
    compiler_test test = {
            "{}",
            2,
            {opcode_make_instruction_and_track(OP_HASH, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_POP, 0)},
            NULL
    };

    printf("Testing empty hash literal: {}\n");
    run_compiler_tests(&test);
}

static void test_hash_literal_with_constants(void) {
    compiler_test test = {
            "{1: 2, 3: 4, 5: 6}",
            8,
            {opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){2}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){3}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){4}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){5}),
             opcode_make_instruction_and_track(OP_HASH, (size_t[]){6}),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(6,
                                 (object_object *) object_create_int(1),
                                 (object_object *) object_create_int(2),
                                 (object_object *) object_create_int(3),
                                 (object_object *) object_create_int(4),
                                 (object_object *) object_create_int(5),
                                 (object_object *) object_create_int(6))
    };

    printf("Testing hash literal with constants: {1: 2, 3: 4, 5: 6}\n");
    run_compiler_tests(&test);
}

static void test_hash_literal_with_expressions(void) {
    compiler_test test = {
            "{1: 2 + 3, 4: 5 * 6}",
            10,
            {opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){2}),
             opcode_make_instruction_and_track(OP_ADD, 0),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){3}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){4}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){5}),
             opcode_make_instruction_and_track(OP_MUL, 0),
             opcode_make_instruction_and_track(OP_HASH, (size_t[]){4}),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(6, (object_object *) object_create_int(1), (object_object *) object_create_int(2),
                                 (object_object *) object_create_int(3), (object_object *) object_create_int(4),
                                 (object_object *) object_create_int(5), (object_object *) object_create_int(6))
    };

    printf("Testing hash literal with expressions: {1: 2 + 3, 4: 5 * 6}\n");
    run_compiler_tests(&test);
}


/***************************************************************
********************** ARRAY LITERALS **************************
 ***************************************************************/
static void test_empty_array_literal(void) {
    compiler_test test = {
            "[]",
            2,
            {opcode_make_instruction_and_track(OP_ARRAY, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_POP, 0)},
            NULL
    };

    printf("Testing empty array literal: []\n");
    run_compiler_tests(&test);
}

static void test_array_literal_with_constants(void) {
    compiler_test test = {
            "[1, 2, 3]",
            5,
            {opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){2}),
             opcode_make_instruction_and_track(OP_ARRAY, (size_t[]){3}),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(3, (object_object *) object_create_int(1), (object_object *) object_create_int(2),
                                 (object_object *) object_create_int(3))
    };

    printf("Testing array literal with constants: [1, 2, 3]\n");
    run_compiler_tests(&test);
}

static void test_array_literal_with_expressions(void) {
    compiler_test test = {
            "[1 + 2, 3 - 4, 5 * 6]",
            11,
            {opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1}),
             opcode_make_instruction_and_track(OP_ADD, 0),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){2}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){3}),
             opcode_make_instruction_and_track(OP_SUB, 0),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){4}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){5}),
             opcode_make_instruction_and_track(OP_MUL, 0),
             opcode_make_instruction_and_track(OP_ARRAY, (size_t[]){3}),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(6, (object_object *) object_create_int(1), (object_object *) object_create_int(2),
                                 (object_object *) object_create_int(3), (object_object *) object_create_int(4),
                                 (object_object *) object_create_int(5), (object_object *) object_create_int(6))
    };

    printf("Testing array literal with expressions: [1 + 2, 3 - 4, 5 * 6]\n");
    run_compiler_tests(&test);
}


/***************************************************************
********************** INDEX EXPRESSIONS ***********************
 ***************************************************************/
static void test_array_index_expression(void) {
    compiler_test test = {
            "[1, 2, 3][1 + 2]",
            9,
            {opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){2}),
             opcode_make_instruction_and_track(OP_ARRAY, (size_t[]){3}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){3}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){4}),
             opcode_make_instruction_and_track(OP_ADD, 0),
             opcode_make_instruction_and_track(OP_INDEX, 0),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(5, (object_object *) object_create_int(1), (object_object *) object_create_int(2),
                                 (object_object *) object_create_int(3), (object_object *) object_create_int(1),
                                 (object_object *) object_create_int(2))
    };

    printf("Testing array index expression: [1, 2, 3][1 + 2]\n");
    run_compiler_tests(&test);
}

static void test_hash_index_expression(void) {
    compiler_test test = {
            "{1: 2}[2 - 1]",
            8,
            {opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1}),
             opcode_make_instruction_and_track(OP_HASH, (size_t[]){2}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){2}),
             opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){3}),
             opcode_make_instruction_and_track(OP_SUB, 0),
             opcode_make_instruction_and_track(OP_INDEX, 0),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(4, (object_object *) object_create_int(1), (object_object *) object_create_int(2),
                                 (object_object *) object_create_int(2), (object_object *) object_create_int(1))
    };

    printf("Testing hash index expression: {1: 2}[2 - 1]\n");
    run_compiler_tests(&test);
}

/***************************************************************
************************** FUNCTIONS ****************************
 ***************************************************************/
static instructions *create_compiled_fn_instructions(const size_t instruction_count, ...) {
    va_list args;
    va_start(args, instruction_count);
    instructions *retval = NULL;
    for (size_t i = 0; i < instruction_count; i++) {
        instructions *ins = va_arg(args, instructions *);
        if (retval == NULL) {
            retval = ins;
        } else {
            concat_instructions(retval, ins);
            instructions_free(ins);
        }
    }
    return retval;
}

/***************************************************************
********************* RECURSIVE FUNCTIONS **********************
 ***************************************************************/
static void test_simple_recursive_function(void) {
    compiler_test test;
    test.input                    = "let countDown = fn(x) { countDown(x - 1); }; countDown(1)";
    test.instructions_count       = 6;
    test.expected_instructions[0] = opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){1, 0});
    test.expected_instructions[1] = opcode_make_instruction_and_track(OP_SET_GLOBAL, (size_t[]){0});
    test.expected_instructions[2] = opcode_make_instruction_and_track(OP_GET_GLOBAL, (size_t[]){0});
    test.expected_instructions[3] = opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){2});
    test.expected_instructions[4] = opcode_make_instruction_and_track(OP_CALL, (size_t[]){1});
    test.expected_instructions[5] = opcode_make_instruction_and_track(OP_POP, (size_t[]){0});
    test.expected_constants       = create_constant_pool(
            3,
            object_create_int(1),
            (object_object *) object_create_compiled_fn(
                    create_compiled_fn_instructions(
                            6,
                            opcode_make_instruction(OP_CURRENT_CLOSURE, (size_t[]){0}),
                            opcode_make_instruction(OP_GET_LOCAL, (size_t[]){0}),
                            opcode_make_instruction(OP_CONSTANT, (size_t[]){0}),
                            opcode_make_instruction(OP_SUB, 0),
                            opcode_make_instruction(OP_CALL, (size_t[]){1}),
                            opcode_make_instruction(OP_RETURN_VALUE, 0)),
                    0, 1),
            object_create_int(1));

    printf("Testing simple recursive function\n");
    run_compiler_tests(&test);
}

static void test_nested_recursive_function_wrapper(void) {
    compiler_test test = {
            "let wrapper = fn() {\n"
            "   let countDown = fn(x) {\n"
            "       countDown(x - 1);\n"
            "   };\n"
            "   countDown(1);\n"
            "   }\n"
            "wrapper();",
            5,
            {opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){3, 0}),
             opcode_make_instruction_and_track(OP_SET_GLOBAL, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_GET_GLOBAL, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_CALL, (size_t[]){0}),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(
                    4,
                    object_create_int(1),
                    (object_object *) object_create_compiled_fn(
                            create_compiled_fn_instructions(
                                    6,
                                    opcode_make_instruction(OP_CURRENT_CLOSURE, 0),
                                    opcode_make_instruction(OP_GET_LOCAL, (size_t[]){0}),
                                    opcode_make_instruction(OP_CONSTANT, (size_t[]){0}),
                                    opcode_make_instruction(OP_SUB, 0),
                                    opcode_make_instruction(OP_CALL, (size_t[]){1}),
                                    opcode_make_instruction(OP_RETURN_VALUE, 0)),
                            0, 1),
                    object_create_int(1),
                    (object_object *) object_create_compiled_fn(
                            create_compiled_fn_instructions(
                                    6,
                                    opcode_make_instruction(OP_CLOSURE, (size_t[]){1, 0}),
                                    opcode_make_instruction(OP_SET_LOCAL, (size_t[]){0}),
                                    opcode_make_instruction(OP_GET_LOCAL, (size_t[]){0}),
                                    opcode_make_instruction(OP_CONSTANT, (size_t[]){2}),
                                    opcode_make_instruction(OP_CALL, (size_t[]){1}),
                                    opcode_make_instruction(OP_RETURN_VALUE, 0)),
                            1, 0))
    };

    printf("Testing nested recursive function in wrapper\n");
    run_compiler_tests(&test);
}


/***************************************************************
*********************** FUNCTION ARGUMENTS **********************
 ***************************************************************/
static void test_function_call_no_arguments(void) {
    compiler_test test = {
            "fn() {24}();",
            3,
            {
                    opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){1, 0}),
                    opcode_make_instruction_and_track(OP_CALL, (size_t[]){0}),
                    opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(2,
                                 (object_object *) object_create_int(24),
                                 (object_object *) object_create_compiled_fn(
                                         create_compiled_fn_instructions(2,
                                                                         opcode_make_instruction(
                                                                                 OP_CONSTANT, (size_t[]){0}),
                                                                         opcode_make_instruction(OP_RETURN_VALUE, 0)),
                                         0, 0))
    };
    print_test_separator_line();
    printf("Testing function call with no arguments\n");
    run_compiler_tests(&test);
}

static void test_function_call_with_noarg_variable(void) {
    compiler_test test = {
            "let noArg = fn() {24};"
            "noArg();",
            5,
            {
                    opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){1, 0}),
                    opcode_make_instruction_and_track(OP_SET_GLOBAL, (size_t[]){0}),
                    opcode_make_instruction_and_track(OP_GET_GLOBAL, (size_t[]){0}),
                    opcode_make_instruction_and_track(OP_CALL, (size_t[]){0}),
                    opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(2,
                                 (object_object *) object_create_int(24),
                                 (object_object *) object_create_compiled_fn(
                                         create_compiled_fn_instructions(2,
                                                                         opcode_make_instruction(
                                                                                 OP_CONSTANT, (size_t[]){0}),
                                                                         opcode_make_instruction(OP_RETURN_VALUE, 0)),
                                         0, 0))
    };
    print_test_separator_line();
    printf("Testing function call with noArg variable\n");
    run_compiler_tests(&test);
}

static void test_function_call_one_argument(void) {
    compiler_test test;
    test.input                    = "let oneArg = fn(a) {a}; oneArg(24);";
    test.instructions_count       = 6;
    test.expected_instructions[0] = opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){0, 0});
    test.expected_instructions[1] = opcode_make_instruction_and_track(OP_SET_GLOBAL, (size_t[]){0});
    test.expected_instructions[2] = opcode_make_instruction_and_track(OP_GET_GLOBAL, (size_t[]){0});
    test.expected_instructions[3] = opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1});
    test.expected_instructions[4] = opcode_make_instruction_and_track(OP_CALL, (size_t[]){1});
    test.expected_instructions[5] = opcode_make_instruction_and_track(OP_POP, 0);
    test.expected_constants       = create_constant_pool(2,
                                                   (object_object *) object_create_compiled_fn(
                                                           create_compiled_fn_instructions(
                                                                   2,
                                                                   opcode_make_instruction(
                                                                           OP_GET_LOCAL, (size_t[]){0}),
                                                                   opcode_make_instruction(OP_RETURN_VALUE, 0)),
                                                           0, 1),
                                                   object_create_int(24));
    print_test_separator_line();
    printf("Testing function call with one argument\n");
    run_compiler_tests(&test);
}

static void test_function_call_many_arguments(void) {
    compiler_test test = {
            "let manyArg = fn(a, b, c) {a; b; c;}; manyArg(24, 25, 26);",
            8,
            {
                    opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){0, 0}),
                    opcode_make_instruction_and_track(OP_SET_GLOBAL, (size_t[]){0}),
                    opcode_make_instruction_and_track(OP_GET_GLOBAL, (size_t[]){0}),
                    opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){1}),
                    opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){2}),
                    opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){3}),
                    opcode_make_instruction_and_track(OP_CALL, (size_t[]){3}),
                    opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(4,
                                 (object_object *) object_create_compiled_fn(
                                         create_compiled_fn_instructions(
                                                 6,
                                                 opcode_make_instruction(OP_GET_LOCAL, (size_t[]){0}),
                                                 opcode_make_instruction(OP_POP, 0),
                                                 opcode_make_instruction(OP_GET_LOCAL, (size_t[]){1}),
                                                 opcode_make_instruction(OP_POP, (size_t[]){0}),
                                                 opcode_make_instruction(OP_GET_LOCAL, (size_t[]){2}),
                                                 opcode_make_instruction(OP_RETURN_VALUE, 0)),
                                         0, 3),
                                 (object_object *) object_create_int(24),
                                 (object_object *) object_create_int(25),
                                 (object_object *) object_create_int(26))
    };

    print_test_separator_line();
    printf("Testing function call with many arguments\n");
    run_compiler_tests(&test);
}

/***************************************************************
*********************** CLOSURES *******************************
 ***************************************************************/
static void test_closure_with_two_nested_functions(void) {
    compiler_test test = {
            "\nfn(a) {\n"
            "  fn(b) {\n"
            "    a + b\n"
            "  }\n"
            "};\n",
            2,
            {opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){1, 0}),
             opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(
                    2,
                    (object_object *) object_create_compiled_fn(
                            create_compiled_fn_instructions(
                                    4,
                                    opcode_make_instruction(OP_GET_FREE, (size_t[]){0}),
                                    opcode_make_instruction(OP_GET_LOCAL, (size_t[]){0}),
                                    opcode_make_instruction(OP_ADD, 0),
                                    opcode_make_instruction(OP_RETURN_VALUE, 0)),
                            0, 1),
                    (object_object *) object_create_compiled_fn(
                            create_compiled_fn_instructions(
                                    3,
                                    opcode_make_instruction(OP_GET_LOCAL, (size_t[]){0}),
                                    opcode_make_instruction(OP_CLOSURE, (size_t[]){0, 1}),
                                    opcode_make_instruction(OP_RETURN_VALUE, 0)),
                            0, 1))
    };

    printf("Testing closure with two nested functions\n");
    run_compiler_tests(&test);
}

static void test_closure_with_three_nested_functions(void) {
    compiler_test test;
    test.input =
            "\nfn(a) {\n"
            "   fn(b) {\n"
            "       fn(c) {\n"
            "           a + b + c\n"
            "       }\n"
            "   }\n"
            "}";
    test.instructions_count       = 2;
    test.expected_instructions[0] = opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){2, 0});
    test.expected_instructions[1] = opcode_make_instruction_and_track(OP_POP, 0);
    test.expected_constants       = create_constant_pool(
            3,
            (object_object *) object_create_compiled_fn(
                    create_compiled_fn_instructions(
                            6,
                            opcode_make_instruction(OP_GET_FREE, (size_t[]){0}),
                            opcode_make_instruction(OP_GET_FREE, (size_t[]){1}),
                            opcode_make_instruction(OP_ADD, 0),
                            opcode_make_instruction(OP_GET_LOCAL, (size_t[]){0}),
                            opcode_make_instruction(OP_ADD, 0),
                            opcode_make_instruction(OP_RETURN_VALUE, 0)),
                    0, 1),
            (object_object *) object_create_compiled_fn(
                    create_compiled_fn_instructions(
                            4,
                            opcode_make_instruction(OP_GET_FREE, (size_t[]){0}),
                            opcode_make_instruction(OP_GET_LOCAL, (size_t[]){0}),
                            opcode_make_instruction(OP_CLOSURE, (size_t[]){0, 2}),
                            opcode_make_instruction(OP_RETURN_VALUE, 0)),
                    0, 1),
            (object_object *) object_create_compiled_fn(
                    create_compiled_fn_instructions(
                            3,
                            opcode_make_instruction(OP_GET_LOCAL, (size_t[]){0}),
                            opcode_make_instruction(OP_CLOSURE, (size_t[]){1, 1}),
                            opcode_make_instruction(OP_RETURN_VALUE, 0)),
                    0, 1));

    printf("Testing closure with three nested functions\n");
    run_compiler_tests(&test);
}

static void test_closures_with_global_variables(void) {
    compiler_test test = {
            "let global = 55;\n"
            "fn() {\n"
            "   let a = 66;\n"
            "   fn() {\n"
            "       let b = 77;\n"
            "       fn() {\n"
            "           let c = 88;\n"
            "           global + a + b + c;\n"
            "       }\n"
            "   }\n"
            "}",
            4,
            {
                    opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
                    opcode_make_instruction_and_track(OP_SET_GLOBAL, (size_t[]){0}),
                    opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){6, 0}),
                    opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(
                    7,
                    (object_object *) object_create_int(55),
                    (object_object *) object_create_int(66),
                    (object_object *) object_create_int(77),
                    (object_object *) object_create_int(88),
                    (object_object *) object_create_compiled_fn(
                            create_compiled_fn_instructions(
                                    10,
                                    opcode_make_instruction(OP_CONSTANT, (size_t[]){3}),
                                    opcode_make_instruction(OP_SET_LOCAL, (size_t[]){0}),
                                    opcode_make_instruction(OP_GET_GLOBAL, (size_t[]){0}),
                                    opcode_make_instruction(OP_GET_FREE, (size_t[]){0}),
                                    opcode_make_instruction(OP_ADD, 0),
                                    opcode_make_instruction(OP_GET_FREE, (size_t[]){1}),
                                    opcode_make_instruction(OP_ADD, 0),
                                    opcode_make_instruction(OP_GET_LOCAL, (size_t[]){0}),
                                    opcode_make_instruction(OP_ADD, 0),
                                    opcode_make_instruction(OP_RETURN_VALUE, 0)),
                            1, 0),
                    (object_object *) object_create_compiled_fn(
                            create_compiled_fn_instructions(
                                    6,
                                    opcode_make_instruction(OP_CONSTANT, (size_t[]){2}),
                                    opcode_make_instruction(OP_SET_LOCAL, (size_t[]){0}),
                                    opcode_make_instruction(OP_GET_FREE, (size_t[]){0}),
                                    opcode_make_instruction(OP_GET_LOCAL, (size_t[]){0}),
                                    opcode_make_instruction(OP_CLOSURE, (size_t[]){4, 2}),
                                    opcode_make_instruction(OP_RETURN_VALUE, 0)),
                            1, 0),
                    (object_object *) object_create_compiled_fn(
                            create_compiled_fn_instructions(
                                    5,
                                    opcode_make_instruction(OP_CONSTANT, (size_t[]){1}),
                                    opcode_make_instruction(OP_SET_LOCAL, (size_t[]){0}),
                                    opcode_make_instruction(OP_GET_LOCAL, (size_t[]){0}),
                                    opcode_make_instruction(OP_CLOSURE, (size_t[]){5, 1}),
                                    opcode_make_instruction(OP_RETURN_VALUE, 0)),
                            1, 0))
    };

    printf("Testing closures with global variables\n");
    run_compiler_tests(&test);
}

/***************************************************************
************************** FUNCTIONS ***************************
 ***************************************************************/
static void test_function_with_return(void) {
    compiler_test test = {
            "fn() {return 5 + 10}",
            2,
            {
                    opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){2, 0}),
                    opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(
                    3,
                    (object_object *) object_create_int(5),
                    (object_object *) object_create_int(10),
                    (object_object *) object_create_compiled_fn(
                            create_compiled_fn_instructions(
                                    4,
                                    opcode_make_instruction(OP_CONSTANT, (size_t[]){0}),
                                    opcode_make_instruction(OP_CONSTANT, (size_t[]){1}),
                                    opcode_make_instruction(OP_ADD, 0),
                                    opcode_make_instruction(OP_RETURN_VALUE, 0)),
                            0, 0))
    };

    printf("Testing function with return statement\n");
    run_compiler_tests(&test);
}

static void test_function_with_implicit_return(void) {
    compiler_test test = {
            "fn() {5 + 10}",
            2,
            {
                    opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){2, 0}),
                    opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(
                    3,
                    (object_object *) object_create_int(5),
                    (object_object *) object_create_int(10),
                    (object_object *) object_create_compiled_fn(
                            create_compiled_fn_instructions(
                                    4,
                                    opcode_make_instruction(OP_CONSTANT, (size_t[]){0}),
                                    opcode_make_instruction(OP_CONSTANT, (size_t[]){1}),
                                    opcode_make_instruction(OP_ADD, 0),
                                    opcode_make_instruction(OP_RETURN_VALUE, 0)),
                            0, 0))
    };

    printf("Testing function with implicit return\n");
    run_compiler_tests(&test);
}

static void test_function_with_multiple_statements(void) {
    compiler_test test = {
            "fn() {1; 2;}",
            2,
            {
                    opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){2, 0}),
                    opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(
                    3,
                    (object_object *) object_create_int(1),
                    (object_object *) object_create_int(2),
                    (object_object *) object_create_compiled_fn(
                            create_compiled_fn_instructions(
                                    4,
                                    opcode_make_instruction(OP_CONSTANT, (size_t[]){0}),
                                    opcode_make_instruction(OP_POP, 0),
                                    opcode_make_instruction(OP_CONSTANT, (size_t[]){1}),
                                    opcode_make_instruction(OP_RETURN_VALUE, 0)),
                            0, 0))
    };

    printf("Testing function with multiple statements\n");
    run_compiler_tests(&test);
}

static void test_empty_function(void) {
    compiler_test test;
    test.input                    = "fn() {}";
    test.instructions_count       = 2;
    test.expected_instructions[0] = opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){0, 0});
    test.expected_instructions[1] = opcode_make_instruction_and_track(OP_POP, 0);
    test.expected_constants       = create_constant_pool(
            1,
            object_create_compiled_fn(
                    create_compiled_fn_instructions(1,
                                                    opcode_make_instruction(OP_RETURN, 0)),
                    0, 0));

    printf("Testing empty function\n");

    run_compiler_tests(&test);
}

/***************************************************************
******************** LET STATEMENT SCOPE ***********************
 ***************************************************************/
static void test_global_let_statement(void) {
    compiler_test test = {
            "let num = 55;\n"
            "fn() { num };",
            4,
            {
                    opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
                    opcode_make_instruction_and_track(OP_SET_GLOBAL, (size_t[]){0}),
                    opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){1, 0}),
                    opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(
                    2,
                    (object_object *) object_create_int(55),
                    (object_object *) object_create_compiled_fn(
                            create_compiled_fn_instructions(2,
                                                            opcode_make_instruction(OP_GET_GLOBAL, (size_t[]){0}),
                                                            opcode_make_instruction(OP_RETURN_VALUE, 0)),
                            0, 0))
    };

    printf("Testing global let statement\n");
    run_compiler_tests(&test);
}

static void test_local_let_statement(void) {
    compiler_test test = {
            "fn() {\n"
            "  let num = 55;\n"
            "  num;\n"
            "}",
            2,
            {
                    opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){1, 0}),
                    opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(
                    2,
                    (object_object *) object_create_int(55),
                    (object_object *) object_create_compiled_fn(
                            create_compiled_fn_instructions(
                                    4,
                                    opcode_make_instruction(OP_CONSTANT, (size_t[]){0}),
                                    opcode_make_instruction(OP_SET_LOCAL, (size_t[]){0}),
                                    opcode_make_instruction(OP_GET_LOCAL, (size_t[]){0}),
                                    opcode_make_instruction(OP_RETURN_VALUE, 0)),
                            1, 0))
    };

    printf("Testing local let statement\n");
    run_compiler_tests(&test);
}

static void test_multiple_local_let_statements(void) {
    compiler_test test = {
            "fn() {\n"
            "  let a = 55;\n"
            "  let b = 77;\n"
            "  a + b;\n"
            "}",
            2,
            {
                    opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){2, 0}),
                    opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(
                    3,
                    (object_object *) object_create_int(55),
                    (object_object *) object_create_int(77),
                    (object_object *) object_create_compiled_fn(
                            create_compiled_fn_instructions(
                                    8,
                                    opcode_make_instruction(OP_CONSTANT, (size_t[]){0}),
                                    opcode_make_instruction(OP_SET_LOCAL, (size_t[]){0}),
                                    opcode_make_instruction(OP_CONSTANT, (size_t[]){1}),
                                    opcode_make_instruction(OP_SET_LOCAL, (size_t[]){1}),
                                    opcode_make_instruction(OP_GET_LOCAL, (size_t[]){0}),
                                    opcode_make_instruction(OP_GET_LOCAL, (size_t[]){1}),
                                    opcode_make_instruction(OP_ADD, 0),
                                    opcode_make_instruction(OP_RETURN_VALUE, 0)),
                            2, 0))
    };

    printf("Testing multiple local let statements\n");
    run_compiler_tests(&test);
}

/***************************************************************
************************* BUILTINS *****************************
 ***************************************************************/
static void test_builtin_function_calls(void) {
    compiler_test test = {
            "len([]);\n"
            "push([], 1);",
            9,
            {
                    opcode_make_instruction_and_track(OP_GET_BUILTIN, (size_t[]){0}),
                    opcode_make_instruction_and_track(OP_ARRAY, (size_t[]){0}),
                    opcode_make_instruction_and_track(OP_CALL, (size_t[]){1}),
                    opcode_make_instruction_and_track(OP_POP, 0),
                    opcode_make_instruction_and_track(OP_GET_BUILTIN, (size_t[]){5}),
                    opcode_make_instruction_and_track(OP_ARRAY, (size_t[]){0}),
                    opcode_make_instruction_and_track(OP_CONSTANT, (size_t[]){0}),
                    opcode_make_instruction_and_track(OP_CALL, (size_t[]){2}),
                    opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(1, (object_object *) object_create_int(1))
    };

    printf("Testing built-in function calls: len and push\n");
    run_compiler_tests(&test);
}

static void test_builtin_function_in_closure(void) {
    compiler_test test = {
            "fn() {len([]);};",
            2,
            {
                    opcode_make_instruction_and_track(OP_CLOSURE, (size_t[]){0, 0}),
                    opcode_make_instruction_and_track(OP_POP, 0)},
            create_constant_pool(1,
                                 (object_object *) object_create_compiled_fn(
                                         create_compiled_fn_instructions(4,
                                                                         opcode_make_instruction(
                                                                                 OP_GET_BUILTIN, (size_t[]){0}),
                                                                         opcode_make_instruction(
                                                                                 OP_ARRAY, (size_t[]){0}),
                                                                         opcode_make_instruction(
                                                                                 OP_CALL, (size_t[]){1}),
                                                                         opcode_make_instruction(OP_RETURN_VALUE, 0)),
                                         0, 0))
    };

    printf("Testing built-in function within a closure: len\n");
    run_compiler_tests(&test);
}

static void run_compiler_tests(compiler_test *test) {
    print_test_separator_line();

    printf("** Testing compilation for %s\n", test->input);
    lexer *              lexer    = lexer_init(test->input);
    parser *             parser   = parser_init(lexer);
    ast_program *        program  = parse_program(parser);
    compiler *           compiler = compiler_init();
    const compiler_error e        = compile(compiler, (ast_node *) program);

#ifdef DEBUG
    ast_debug_print(program);
#endif
    if (e.error_code != COMPILER_ERROR_NONE) {
        err(EXIT_FAILURE, "Compilation failed for input %s with error %s\n", test->input, e.msg);
    }

    bytecode *    bytecode               = get_bytecode(compiler);
    instructions *flattened_instructions = opcode_flatten_instructions(test->instructions_count,
                                                                       test->expected_instructions);

    char *actual_ins_string   = instructions_to_string(bytecode->instructions);
    char *expected_ins_string = instructions_to_string(flattened_instructions);
    printf("Expected instructions:\n%s\n", expected_ins_string);
    printf("Actual instructions:\n%s\n", actual_ins_string);

    TEST_ASSERT_EQUAL_STRING(expected_ins_string, actual_ins_string);
    TEST_ASSERT_EQUAL_INT(flattened_instructions->length, bytecode->instructions->length);

    test_instructions(flattened_instructions->length, flattened_instructions->bytes, bytecode->instructions->bytes);
    test_constant_pool_and_free(test->expected_constants, bytecode->constants_pool);

    compiler_free(compiler);
    program_free(program);
    parser_free(parser);
    free(expected_ins_string);
    free(actual_ins_string);
    bytecode_free(bytecode);
    instructions_free(flattened_instructions);
}

static void run_specific_test(const char *test_name) {
    if (strcmp(test_name, "test_addition") == 0) {
        RUN_TEST(test_addition);
    } else if (strcmp(test_name, "test_multiple_expressions") == 0) {
        RUN_TEST(test_multiple_expressions);
    } else if (strcmp(test_name, "test_subtraction") == 0) {
        RUN_TEST(test_subtraction);
    } else if (strcmp(test_name, "test_multiplication") == 0) {
        RUN_TEST(test_multiplication);
    } else if (strcmp(test_name, "test_division") == 0) {
        RUN_TEST(test_division);
    } else if (strcmp(test_name, "test_negation") == 0) {
        RUN_TEST(test_negation);
    } else if (strcmp(test_name, "test_boolean_true") == 0) {
        RUN_TEST(test_boolean_true);
    } else if (strcmp(test_name, "test_boolean_false") == 0) {
        RUN_TEST(test_boolean_false);
    } else if (strcmp(test_name, "test_greater_than_expression") == 0) {
        RUN_TEST(test_greater_than_expression);
    } else if (strcmp(test_name, "test_less_than_expression") == 0) {
        RUN_TEST(test_less_than_expression);
    } else if (strcmp(test_name, "test_equal_expression") == 0) {
        RUN_TEST(test_equal_expression);
    } else if (strcmp(test_name, "test_not_equal_expression") == 0) {
        RUN_TEST(test_not_equal_expression);
    } else if (strcmp(test_name, "test_true_equals_true") == 0) {
        RUN_TEST(test_true_equals_true);
    } else if (strcmp(test_name, "test_true_not_equal_false") == 0) {
        RUN_TEST(test_true_not_equal_false);
    } else if (strcmp(test_name, "test_bang_operator") == 0) {
        RUN_TEST(test_bang_operator);
    } else if (strcmp(test_name, "test_if_true_then_block") == 0) {
        RUN_TEST(test_if_true_then_block);
    } else if (strcmp(test_name, "test_if_true_else_block") == 0) {
        RUN_TEST(test_if_true_else_block);
    } else if (strcmp(test_name, "test_multiple_global_let_statements") == 0) {
        RUN_TEST(test_multiple_global_let_statements);
    } else if (strcmp(test_name, "test_global_let_and_usage") == 0) {
        RUN_TEST(test_global_let_and_usage);
    } else if (strcmp(test_name, "test_single_string_expression") == 0) {
        RUN_TEST(test_single_string_expression);
    } else if (strcmp(test_name, "test_string_concatenation_expression") == 0) {
        RUN_TEST(test_string_concatenation_expression);
    } else if (strcmp(test_name, "test_empty_array_literal") == 0) {
        RUN_TEST(test_empty_array_literal);
    } else if (strcmp(test_name, "test_array_literal_with_constants") == 0) {
        RUN_TEST(test_array_literal_with_constants);
    } else if (strcmp(test_name, "test_array_literal_with_expressions") == 0) {
        RUN_TEST(test_array_literal_with_expressions);
    } else if (strcmp(test_name, "test_empty_hash_literal") == 0) {
        RUN_TEST(test_empty_hash_literal);
    } else if (strcmp(test_name, "test_hash_literal_with_constants") == 0) {
        RUN_TEST(test_hash_literal_with_constants);
    } else if (strcmp(test_name, "test_hash_literal_with_expressions") == 0) {
        RUN_TEST(test_hash_literal_with_expressions);
    } else if (strcmp(test_name, "test_array_index_expression") == 0) {
        RUN_TEST(test_array_index_expression);
    } else if (strcmp(test_name, "test_hash_index_expression") == 0) {
        RUN_TEST(test_hash_index_expression);
    } else if (strcmp(test_name, "test_compiler_scopes") == 0) {
        RUN_TEST(test_compiler_scopes);
    } else if (strcmp(test_name, "test_simple_recursive_function") == 0) {
        RUN_TEST(test_simple_recursive_function);
    } else if (strcmp(test_name, "test_nested_recursive_function_wrapper") == 0) {
        RUN_TEST(test_nested_recursive_function_wrapper);
    } else if (strcmp(test_name, "test_closure_with_two_nested_functions") == 0) {
        RUN_TEST(test_closure_with_two_nested_functions);
    } else if (strcmp(test_name, "test_closure_with_three_nested_functions") == 0) {
        RUN_TEST(test_closure_with_three_nested_functions);
    } else if (strcmp(test_name, "test_closures_with_global_variables") == 0) {
        RUN_TEST(test_closures_with_global_variables);
    } else if (strcmp(test_name, "test_function_with_return") == 0) {
        RUN_TEST(test_function_with_return);
    } else if (strcmp(test_name, "test_function_with_implicit_return") == 0) {
        RUN_TEST(test_function_with_implicit_return);
    } else if (strcmp(test_name, "test_function_with_multiple_statements") == 0) {
        RUN_TEST(test_function_with_multiple_statements);
    } else if (strcmp(test_name, "test_empty_function") == 0) {
        RUN_TEST(test_empty_function);
    } else if (strcmp(test_name, "test_function_call_no_arguments") == 0) {
        RUN_TEST(test_function_call_no_arguments);
    } else if (strcmp(test_name, "test_function_call_with_noarg_variable") == 0) {
        RUN_TEST(test_function_call_with_noarg_variable);
    } else if (strcmp(test_name, "test_function_call_one_argument") == 0) {
        RUN_TEST(test_function_call_one_argument);
    } else if (strcmp(test_name, "test_function_call_many_arguments") == 0) {
        RUN_TEST(test_function_call_many_arguments);
    } else if (strcmp(test_name, "test_global_let_statement") == 0) {
        RUN_TEST(test_global_let_statement);
    } else if (strcmp(test_name, "test_global_let_and_usage") == 0) {
        RUN_TEST(test_global_let_and_usage);
    } else if (strcmp(test_name, "test_local_let_statement") == 0) {
        RUN_TEST(test_local_let_statement);
    } else if (strcmp(test_name, "test_multiple_local_let_statements") == 0) {
        RUN_TEST(test_multiple_local_let_statements);
    } else if (strcmp(test_name, "test_builtin_function_calls") == 0) {
        RUN_TEST(test_builtin_function_calls);
    } else if (strcmp(test_name, "test_builtin_function_in_closure") == 0) {
        RUN_TEST(test_builtin_function_in_closure);
    } else {
        printf("Test '%s' not found.\n", test_name);
    }
}


int main(int argc, char **argv) {
    UNITY_BEGIN();
    if (argc > 1) {
        // Run a specific test by name
        printf("Running test: %s\n", argv[1]);
        run_specific_test(argv[1]);
    } else {
        // Run all tests if no arguments are provided
        printf("Running all tests\n");
        RUN_TEST(test_addition);
        RUN_TEST(test_multiple_expressions);
        RUN_TEST(test_subtraction);
        RUN_TEST(test_multiplication);
        RUN_TEST(test_division);
        RUN_TEST(test_negation);
        RUN_TEST(test_boolean_true);
        RUN_TEST(test_boolean_false);
        RUN_TEST(test_greater_than_expression);
        RUN_TEST(test_less_than_expression);
        RUN_TEST(test_equal_expression);
        RUN_TEST(test_not_equal_expression);
        RUN_TEST(test_true_equals_true);
        RUN_TEST(test_true_not_equal_false);
        RUN_TEST(test_bang_operator);
        RUN_TEST(test_if_true_then_block);
        RUN_TEST(test_if_true_else_block);
        RUN_TEST(test_multiple_global_let_statements);
        RUN_TEST(test_global_let_and_usage);
        RUN_TEST(test_single_string_expression);
        RUN_TEST(test_string_concatenation_expression);
        RUN_TEST(test_empty_array_literal);
        RUN_TEST(test_array_literal_with_constants);
        RUN_TEST(test_array_literal_with_expressions);
        RUN_TEST(test_empty_hash_literal);
        RUN_TEST(test_hash_literal_with_constants);
        RUN_TEST(test_hash_literal_with_expressions);
        RUN_TEST(test_array_index_expression);
        RUN_TEST(test_hash_index_expression);
        RUN_TEST(test_compiler_scopes);
        RUN_TEST(test_simple_recursive_function);
        RUN_TEST(test_nested_recursive_function_wrapper);
        RUN_TEST(test_closure_with_two_nested_functions);
        RUN_TEST(test_closure_with_three_nested_functions);
        RUN_TEST(test_closures_with_global_variables);
        RUN_TEST(test_function_with_return);
        RUN_TEST(test_function_with_implicit_return);
        RUN_TEST(test_function_with_multiple_statements);
        RUN_TEST(test_empty_function);
        RUN_TEST(test_function_call_no_arguments);
        RUN_TEST(test_function_call_with_noarg_variable);
        RUN_TEST(test_function_call_one_argument);
        RUN_TEST(test_function_call_many_arguments);
        RUN_TEST(test_global_let_statement);
        RUN_TEST(test_global_let_and_usage);
        RUN_TEST(test_local_let_statement);
        RUN_TEST(test_multiple_local_let_statements);
        RUN_TEST(test_builtin_function_calls);
        RUN_TEST(test_builtin_function_in_closure);
    }

    return UNITY_END();
}
