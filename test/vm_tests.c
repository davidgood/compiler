//
// Created by dgood on 12/19/24.
//
#include <err.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../src/object/object.h"
#include "../Unity/src/unity.h"
#include "../src/compiler/compiler_core.h"
#include "../src/lexer/lexer.h"
#include "object_test_utils.h"
#include "../src/parser/parser.h"
#include "test_utils.h"
#include "../src/vm/virtual_machine.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

typedef struct vm_testcase {
    const char *   input;
    object_object *expected;
} vm_testcase;

static void run_vm_tests(size_t test_count, vm_testcase test_cases[test_count]);

static void dump_bytecode(bytecode *bytecode) {
    object_compiled_fn *fn;
    object_int *        int_obj;
    char *              instructions = instructions_to_string(bytecode->instructions);
    printf(" Instructions:\n%s\n", instructions);
    free(instructions);
    if (bytecode->constants_pool == NULL)
        return;
    for (size_t i = 0; i < bytecode->constants_pool->size; i++) {
        object_object *constant = arraylist_get(bytecode->constants_pool, i);
        printf("CONSTANT %zu %p %s:\n", i, constant, get_type_name(constant->type));
        switch (constant->type) {
            case OBJECT_COMPILED_FUNCTION:
                fn = (object_compiled_fn *) constant;
                char *ins = instructions_to_string(fn->instructions);
                printf(" Instructions:\n%s", ins);
                free(ins);
                break;
            case OBJECT_INT:
                int_obj = (object_int *) constant;
                printf(" Value: %ld\n", int_obj->value);
                break;
            default:
                break;
        }
        printf("\n");
    }
}


static void test_recursive_closures(void) {
    vm_testcase tests[] = {
            {
                    "let countDown = fn(x) {\n"
                    "   if (x == 0) {\n"
                    "       return 0\n"
                    "   } else {\n"
                    "       return countDown(x - 1);\n"
                    "   }\n"
                    "}\n"
                    "countDown(1);\n",
                    (object_object *) object_create_int(0)
            },
            {
                    "let countDown = fn(x) {\n"
                    "   if (x == 0) {\n"
                    "       return 0\n"
                    "   } else {\n"
                    "       return countDown(x - 1);\n"
                    "   }\n"
                    "};\n"
                    "let wrapper = fn() {\n"
                    "   countDown(1);\n"
                    "};\n"
                    "wrapper();",
                    (object_object *) object_create_int(0)
            },
            {
                    "let wrapper = fn() {\n"
                    "   let countDown = fn(x) {\n"
                    "       if (x == 0) {\n"
                    "           return 0;\n"
                    "       } else {\n"
                    "           return countDown(x - 1);\n"
                    "       }\n"
                    "   };\n"
                    "   countDown(1);\n"
                    "};\n"
                    "wrapper();",
                    (object_object *) object_create_int(0)
            }
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        object_free(tests[i].expected);

}

static void
test_recursive_fibonacci(void) {
    vm_testcase tests[] = {
            {
                    "let fibonacci = fn(x) {\n"
                    "   if (x == 0) {\n"
                    "       return 0;\n"
                    "   } else {\n"
                    "       if (x == 1) {\n"
                    "           return 1;\n"
                    "       } else {\n"
                    "           fibonacci(x - 1) + fibonacci(x - 2);\n"
                    "       }\n"
                    "   }\n"
                    "};\n"
                    "fibonacci(15);",
                    (object_object *) object_create_int(610)
            }
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++) {
        object_free(tests[i].expected);
    }

}


static void test_basic_boolean_expressions(void) {
    vm_testcase tests[] = {
            {"true", (object_object *) object_create_bool(true)},
            {"false", (object_object *) object_create_bool(false)},
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++) {
        object_free(tests[i].expected);
    }
}

static void test_comparison_expressions(void) {
    vm_testcase tests[] = {
            {"1 < 2", (object_object *) object_create_bool(true)},
            {"1 > 2", (object_object *) object_create_bool(false)},
            {"1 < 1", (object_object *) object_create_bool(false)},
            {"1 > 1", (object_object *) object_create_bool(false)},
            {"1 == 1", (object_object *) object_create_bool(true)},
            {"1 != 1", (object_object *) object_create_bool(false)},
            {"1 == 2", (object_object *) object_create_bool(false)},
            {"1 != 2", (object_object *) object_create_bool(true)},
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++) {
        object_free(tests[i].expected);
    }
}

static void test_boolean_comparisons(void) {
    vm_testcase tests[] = {
            {"true == true", (object_object *) object_create_bool(true)},
            {"false == false", (object_object *) object_create_bool(true)},
            {"true == false", (object_object *) object_create_bool(false)},
            {"false != true", (object_object *) object_create_bool(true)},
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++) {
        object_free(tests[i].expected);
    }
}

static void test_parenthesized_boolean_expressions(void) {
    vm_testcase tests[] = {
            {"(1 < 2) == true", (object_object *) object_create_bool(true)},
            {"(1 < 2) == false", (object_object *) object_create_bool(false)},
            {"(1 > 2) == false", (object_object *) object_create_bool(true)},
            {"(1 > 2) == true", (object_object *) object_create_bool(false)},
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++) {
        object_free(tests[i].expected);
    }
}

static void test_nested_and_complex_expressions(void) {
    vm_testcase tests[] = {
            {"!(if (false) {5;})", (object_object *) object_create_bool(true)},
            {"if (if (false) {10}) {10} else {20}", (object_object *) object_create_int(20)},
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++) {
        object_free(tests[i].expected);
    }
}


static void test_vm_integer_single_value(void) {
    vm_testcase test = {"1", (object_object *) object_create_int(1)};
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_vm_integer_addition(void) {
    vm_testcase test = {"1 + 2", (object_object *) object_create_int(3)};
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_vm_integer_subtraction(void) {
    vm_testcase test = {"1 - 2", (object_object *) object_create_int(-1)};
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_vm_integer_multiplication(void) {
    vm_testcase test = {"1 * 2", (object_object *) object_create_int(2)};
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_vm_integer_division(void) {
    vm_testcase test = {"4 / 2", (object_object *) object_create_int(2)};
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_vm_integer_complex_expression_1(void) {
    vm_testcase test = {"50 / 2 * 2 + 10 - 5", (object_object *) object_create_int(55)};
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_vm_integer_complex_expression_2(void) {
    vm_testcase test = {"5 + 5 + 5 + 5 - 10", (object_object *) object_create_int(10)};
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_vm_integer_exponentiation(void) {
    vm_testcase test = {"2 * 2 * 2 * 2 * 2", (object_object *) object_create_int(32)};
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_vm_integer_mixed_operations_1(void) {
    vm_testcase test = {"5 * 2 + 10", (object_object *) object_create_int(20)};
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_vm_integer_mixed_operations_2(void) {
    vm_testcase test = {"5 + 2 * 10", (object_object *) object_create_int(25)};
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_vm_integer_parentheses(void) {
    vm_testcase test = {"5 * (2 + 10)", (object_object *) object_create_int(60)};
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_vm_integer_negative_value_1(void) {
    vm_testcase test = {"-5", (object_object *) object_create_int(-5)};
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_vm_integer_negative_value_2(void) {
    vm_testcase test = {"-10", (object_object *) object_create_int(-10)};
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_vm_integer_negative_addition(void) {
    vm_testcase test = {"-50 + 100 + -50", (object_object *) object_create_int(0)};
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_vm_integer_complex_parentheses(void) {
    vm_testcase test = {"(5 + 10 * 2 + 15 / 3) * 2 + -10", (object_object *) object_create_int(50)};
    run_vm_tests(1, &test);
    object_free(test.expected);
}


static void test_if_true(void) {
    vm_testcase test = {"if (true) {10}", (object_object *) object_create_int(10)};
    print_test_separator_line();
    printf("Testing: if (true) {10}\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_if_true_else(void) {
    vm_testcase test = {"if (true) {10} else {20}", (object_object *) object_create_int(10)};
    print_test_separator_line();
    printf("Testing: if (true) {10} else {20}\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_if_false_else(void) {
    vm_testcase test = {"if (false) {10} else {20}", (object_object *) object_create_int(20)};
    print_test_separator_line();
    printf("Testing: if (false) {10} else {20}\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_if_1(void) {
    vm_testcase test = {"if (1) {10}", (object_object *) object_create_int(10)};
    print_test_separator_line();
    printf("Testing: if (1) {10}\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_if_1_less_2(void) {
    vm_testcase test = {"if (1 < 2) {10}", (object_object *) object_create_int(10)};
    print_test_separator_line();
    printf("Testing: if (1 < 2) {10}\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_if_1_less_2_else(void) {
    vm_testcase test = {"if (1 < 2) {10} else {20}", (object_object *) object_create_int(10)};
    print_test_separator_line();
    printf("Testing: if (1 < 2) {10} else {20}\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_if_1_greater_2_else(void) {
    vm_testcase test = {"if (1 > 2) {10} else {20}", (object_object *) object_create_int(20)};
    print_test_separator_line();
    printf("Testing: if (1 > 2) {10} else {20}\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_if_false(void) {
    vm_testcase test = {"if (false) {10}", (object_object *) object_create_null()};
    print_test_separator_line();
    printf("Testing: if (false) {10}\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_if_1_greater_2(void) {
    vm_testcase test = {"if (1 > 2) {10}", (object_object *) object_create_null()};
    print_test_separator_line();
    printf("Testing: if (1 > 2) {10}\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}


static void test_global_let_stmts(void) {
    vm_testcase tests[] = {
            {"let one = 1; one", (object_object *) object_create_int(1)},
            {"let one = 1; let two = 2; one + two", (object_object *) object_create_int(3)},
            {"let one = 1; let two = one + one; one + two", (object_object *) object_create_int(3)}
    };
    print_test_separator_line();
    printf("Testing global let statements\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++) {
        object_free(tests[i].expected);
    }
}

static void test_string_expressions(void) {
    vm_testcase tests[] = {
            {"\"monkey\"", (object_object *) object_create_string("monkey", 6)},
            {"\"mon\" + \"key\"", (object_object *) object_create_string("monkey", 6)},
            {"\"mon\" + \"key\" + \"banana\"", (object_object *) object_create_string("monkeybanana", 12)}
    };
    print_test_separator_line();
    printf("Testing string expressions\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++) {
        object_free(tests[i].expected);
    }
}

static object_array *create_monkey_int_array(size_t count, ...) {
    va_list ap;
    va_start(ap, count);
    arraylist *list = arraylist_create(count, NULL);
    for (size_t i = 0; i < count; i++) {
        int val = va_arg(ap, int);
        arraylist_add(list, object_create_int(val));
    };
    va_end(ap);
    return object_create_array(list);
}

static void test_empty_array_literal(void) {
    vm_testcase test = {"[]", (object_object *) create_monkey_int_array(0)};
    print_test_separator_line();
    printf("Testing: []\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
    log_active_arraylists();
}

static void test_simple_array_literal(void) {
    vm_testcase test = {"[1, 2, 3]", (object_object *) create_monkey_int_array(3, 1, 2, 3)};
    print_test_separator_line();
    printf("Testing: [1, 2, 3]\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
    log_active_arraylists();
}

static void test_array_with_expressions(void) {
    vm_testcase test = {"[1 + 2, 3 * 4, 5 + 6]", (object_object *) create_monkey_int_array(3, 3, 12, 11)};
    print_test_separator_line();
    printf("Testing: [1 + 2, 3 * 4, 5 + 6]\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
    log_active_arraylists();
}


static object_hash *create_hash_table(size_t n, object_object *objects[n]) {
    hashtable *table = hashtable_create(object_get_hash, object_equals, object_free, object_free);
    for (size_t i = 0; i < n; i += 2) {
        object_object *key   = objects[i];
        object_object *value = objects[i + 1];
        hashtable_set(table, key, value);
    }
    return object_create_hash(table);
}

static void test_hash_empty_literal(void) {
    vm_testcase test = {"{}", (object_object *) create_hash_table(0, nullptr)};
    print_test_separator_line();
    printf("Testing empty hash literal\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_hash_simple_literal(void) {
    vm_testcase test = {
            "{1: 2, 3: 4}",
            (object_object *) create_hash_table(
                    (size_t) 4,
                    (object_object *[4]){
                            (object_object *) object_create_int(1),
                            (object_object *) object_create_int(2),
                            (object_object *) object_create_int(3),
                            (object_object *) object_create_int(4)
                    }
                    )
    };
    print_test_separator_line();
    printf("Testing simple hash literal\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_hash_expression_literal(void) {
    vm_testcase test = {
            "{1 + 1: 2 * 2, 3 + 3: 4 * 4}",
            (object_object *) create_hash_table(
                    (size_t) 4,
                    (object_object *[4]){
                            (object_object *) object_create_int(2),
                            (object_object *) object_create_int(4),
                            (object_object *) object_create_int(6),
                            (object_object *) object_create_int(16)
                    }
                    )
    };
    print_test_separator_line();
    printf("Testing hash literal with expressions\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}


static void test_index_array_access(void) {
    vm_testcase test = {"[1, 2, 3][1]", (object_object *) object_create_int(2)};
    print_test_separator_line();
    printf("Testing: [1, 2, 3][1]\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
    log_active_arraylists();
}

static void test_index_array_expression(void) {
    vm_testcase test = {"[1, 2, 3][0 + 2]", (object_object *) object_create_int(3)};
    print_test_separator_line();
    printf("Testing: [1, 2, 3][0 + 2]\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_nested_array_access(void) {
    vm_testcase test = {"[[1, 1, 1]][0][0]", (object_object *) object_create_int(1)};
    print_test_separator_line();
    printf("Testing: [[1, 1, 1]][0][0]\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_empty_array_index(void) {
    vm_testcase test = {"[][0]", (object_object *) object_create_null()};
    print_test_separator_line();
    printf("Testing: [][0]\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_out_of_bounds_array_index(void) {
    vm_testcase test = {"[1, 2, 3][99]", (object_object *) object_create_null()};
    print_test_separator_line();
    printf("Testing: [1, 2, 3][99]\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_negative_array_index(void) {
    vm_testcase test = {"[1][-1]", (object_object *) object_create_null()};
    print_test_separator_line();
    printf("Testing: [1][-1]\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_dict_index_existing_key_1(void) {
    vm_testcase test = {"{1: 1, 2: 2}[1]", (object_object *) object_create_int(1)};
    print_test_separator_line();
    printf("Testing: {1: 1, 2: 2}[1]\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_dict_index_existing_key_2(void) {
    vm_testcase test = {"{1: 1, 2: 2}[2]", (object_object *) object_create_int(2)};
    print_test_separator_line();
    printf("Testing: {1: 1, 2: 2}[2]\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_dict_index_non_existing_key(void) {
    vm_testcase test = {"{1: 1}[0]", (object_object *) object_create_null()};
    print_test_separator_line();
    printf("Testing: {1: 1}[0]\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_empty_dict_index(void) {
    vm_testcase test = {"{}[0]", (object_object *) object_create_null()};
    print_test_separator_line();
    printf("Testing: {}[0]\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}


static void test_function_no_arguments_single_function(void) {
    vm_testcase test = {"let fivePlusTen = fn() {5 + 10;}; fivePlusTen();",
                        (object_object *) object_create_int(15)};
    print_test_separator_line();
    printf("Testing single function without arguments\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_function_no_arguments_multiple_functions(void) {
    vm_testcase test = {"let one = fn() {1;}\n let two = fn() {2;}\n one() + two();",
                        (object_object *) object_create_int(3)};
    print_test_separator_line();
    printf("Testing multiple functions without arguments\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_function_no_arguments_nested_calls(void) {
    vm_testcase test = {"let a = fn() {1};\n let b = fn() {a() + 1};\n let c = fn() {b() + 1;};\n c();",
                        (object_object *) object_create_int(3)};
    print_test_separator_line();
    printf("Testing nested function calls without arguments\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}


static void
test_function_with_return_statement(void) {
    vm_testcase tests[] = {
            {"let earlyExit = fn() {return 99; 100;};\n earlyExit();", (object_object *) object_create_int(99)},
            {"let earlyExit = fn() {return 99; return 100;};\n earlyExit();", (object_object *) object_create_int(99)}
    };
    print_test_separator_line();
    printf("Testing functions with return statement\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        object_free(tests[i].expected);
}

static void
test_functions_without_return_value(void) {
    vm_testcase tests[] = {
            {"let noReturn = fn() {};\n noReturn();", (object_object *) object_create_null()},
            {"let noReturn = fn() {};\n let noReturnTwo = fn() {noReturn();}\n noReturn();\n noReturnTwo();",
             (object_object *) object_create_null()}
    };
    print_test_separator_line();
    printf("Testing functions without return value\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        object_free(tests[i].expected);
}

static void
test_first_class_functions(void) {
    vm_testcase tests[] = {
            {"let returnOne = fn() {1;};\n let returnOneReturner = fn() {returnOne;};\n returnOneReturner()();",
             (object_object *) object_create_int(1)}
    };
    print_test_separator_line();
    printf("Testing first class functions\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        object_free(tests[i].expected);
}

static void test_function_with_single_argument(void) {
    vm_testcase test = {
            "let identity = fn(a) {a};\nidentity(4);",
            (object_object *) object_create_int(4)
    };
    print_test_separator_line();
    printf("Testing function with a single argument\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_function_with_two_arguments(void) {
    vm_testcase test = {
            "let sum = fn(a, b) { a + b;};\nsum(1, 2);",
            (object_object *) object_create_int(3)
    };
    print_test_separator_line();
    printf("Testing function with two arguments\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_function_with_local_binding(void) {
    vm_testcase test = {
            "let sum = fn(a, b) {\n  let c = a + b;\n  c;\n};\nsum(1, 2);",
            (object_object *) object_create_int(3)
    };
    print_test_separator_line();
    printf("Testing function with local bindings\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_function_with_multiple_calls(void) {
    vm_testcase test = {
            "let sum = fn(a, b) {\n  let c = a + b;\n  c;\n};\nsum(1, 2) + sum(3, 4);",
            (object_object *) object_create_int(10)
    };
    print_test_separator_line();
    printf("Testing function with multiple calls\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_function_nested_calls(void) {
    vm_testcase test = {
            "let sum = fn(a, b) {\n  let c = a + b;\n  c;\n};\nlet outer = fn() {\n  sum(1, 2) + sum(3, 4);\n};\nouter();",
            (object_object *) object_create_int(10)
    };
    print_test_separator_line();
    printf("Testing nested function calls\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_function_with_global_variable(void) {
    vm_testcase test = {
            "let globalNum = 10;\nlet sum = fn(a, b) {\n  let c = a + b;\n  c + globalNum;\n};\nlet outer = fn() {\n  sum(1, 2) + sum(3, 4) + globalNum;\n};\nouter() + globalNum;",
            (object_object *) object_create_int(50)
    };
    print_test_separator_line();
    printf("Testing function with global variables\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}


static object_array *create_int_array(int *int_arr, size_t length) {
    arraylist *array_list = arraylist_create(length, NULL);
    for (size_t i = 0; i < length; i++) {
        arraylist_add(array_list, (void *) object_create_int(int_arr[i]));
    }
    return object_create_array(array_list);
}

static void
test_calling_functions_with_wrong_arguments(void) {
    typedef struct testcase {
        const char *input;
        const char *expected_errmsg;
    } testcase;
    print_test_separator_line();
    printf("Testing functions with wrong arguments\n");

    testcase tests[] = {
            {
                    "fn() {1;}(1);",
                    "wrong number of arguments: want=0, got=1"
            },
            {
                    "fn(a) {a;}();",
                    "wrong number of arguments: want=1, got=0"
            },
            {
                    "fn(a, b) {a + b;}(1);",
                    "wrong number of arguments: want=2, got=1"
            }
    };

    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        testcase t = tests[i];
        printf("Testing %s\n", t.input);
        lexer *        lexer    = lexer_init(t.input);
        parser *       parser   = parser_init(lexer);
        ast_program *  program  = parse_program(parser);
        compiler *     compiler = compiler_init();
        compiler_error error    = compile(compiler, (ast_node *) program);
        if (error.error_code != COMPILER_ERROR_NONE)
            err(EXIT_FAILURE, "compilation failed for input %s with error %s\n",
                t.input, error.msg);
        bytecode *bytecode = get_bytecode(compiler);
        // dump_bytecode(bytecode);
        virtual_machine *vm       = vm_init(bytecode);
        vm_error         vm_error = vm_run(vm);

        TEST_ASSERT_NOT_EQUAL_INT(vm_error.code, VM_ERROR_NONE);
        TEST_ASSERT_EQUAL_STRING(vm_error.msg, t.expected_errmsg);

        free(vm_error.msg);
        parser_free(parser);
        program_free(program);
        vm_free(vm);
        compiler_free(compiler);
        bytecode_free(bytecode);
    }
}

static void test_len_with_empty_string(void) {
    vm_testcase test = {
            "len(\"\")",
            (object_object *) object_create_int(0)
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_len_with_non_empty_string(void) {
    vm_testcase test = {
            "len(\"four\")",
            (object_object *) object_create_int(4)
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_len_with_long_string(void) {
    vm_testcase test = {
            "len(\"hello world\")",
            (object_object *) object_create_int(11)
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_len_with_integer(void) {
    vm_testcase test = {
            "len(1)",
            (object_object *) object_create_error("argument to `len` not supported, got INTEGER")
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_len_with_extra_arguments(void) {
    vm_testcase test = {
            "len(\"one\", \"two\")",
            (object_object *) object_create_error("wrong number of arguments. got=2, want=1")
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_len_with_array(void) {
    vm_testcase test = {
            "len([1, 2, 3])",
            (object_object *) object_create_int(3)
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_len_with_empty_array(void) {
    vm_testcase test = {
            "len([])",
            (object_object *) object_create_int(0)
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_puts_function(void) {
    vm_testcase test = {
            "puts(\"hello\", \"world!\")",
            (object_object *) object_create_null()
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_first_with_array(void) {
    vm_testcase test = {
            "first([1, 2, 3])",
            (object_object *) object_create_int(1)
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_first_with_empty_array(void) {
    vm_testcase test = {
            "first([])",
            (object_object *) object_create_null()
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_first_with_integer(void) {
    vm_testcase test = {
            "first(1)",
            (object_object *) object_create_error("argument to `first` must be ARRAY, got INTEGER")
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_last_with_array(void) {
    vm_testcase test = {
            "last([1, 2, 3])",
            (object_object *) object_create_int(3)
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_last_with_empty_array(void) {
    vm_testcase test = {
            "last([])",
            (object_object *) object_create_null()
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_last_with_integer(void) {
    vm_testcase test = {
            "last(1)",
            (object_object *) object_create_error("argument to `last` must be ARRAY, got INTEGER")
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_rest_with_array(void) {
    vm_testcase test = {
            "rest([1, 2, 3])",
            (object_object *) create_int_array((int[]){2, 3}, 2)
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_rest_with_empty_array(void) {
    vm_testcase test = {
            "rest([])",
            (object_object *) object_create_null()
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_push_to_empty_array(void) {
    vm_testcase test = {
            "push([], 1)",
            (object_object *) create_int_array((int[]){1}, 1)
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_push_to_integer(void) {
    vm_testcase test = {
            "push(1, 1)",
            (object_object *) object_create_error("argument to `push` must be ARRAY, got INTEGER")
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}


static void test_calling_functions_with_bindings_simple() {
    vm_testcase test = {
            .input = "let one = fn() {let one = 1; one;}; one();",
            .expected = (object_object *) object_create_int(1)
    };

    print_test_separator_line();
    printf("Testing function calls with local bindings\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_calling_functions_with_bindings_complex() {
    vm_testcase test = {
            .input = "let oneAndTwo = fn() {let one = 1; let two = 2; one + two;}; oneAndTwo();",
            .expected = (object_object *) object_create_int(3)
    };

    print_test_separator_line();
    printf("Testing function calls with local bindings\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_calling_functions_with_bindings_two_functions() {
    vm_testcase test = {
            .input = "let oneAndTwo = fn() {let one = 1; let two = 2; one + two;}\n"
            "let threeAndFour = fn() {let three = 3; let four = 4; three + four;}\n"
            "oneAndTwo() + threeAndFour();",
            .expected = (object_object *) object_create_int(10)
    };

    print_test_separator_line();
    printf("Testing function calls with local bindings\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_calling_functions_with_bindings_two_function_calls() {
    vm_testcase test = {
            .input = "let firstFooBar = fn() {let foobar = 50; foobar;}\n"
            "let secondFooBar = fn() { let foobar = 100; foobar;};\n"
            "firstFooBar() + secondFooBar();",
            .expected = (object_object *) object_create_int(150)
    };

    print_test_separator_line();
    printf("Testing function calls with local bindings\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_calling_functions_with_bindings_global_seed(void) {
    vm_testcase test = {
            .input = "let globalSeed = 50;\n"
            "let minusOne = fn() {\n"
            "  let num = 1;\n"
            "  globalSeed - num;\n"
            "}\n"
            "let minusTwo = fn() {\n"
            "  let num = 2;\n"
            "  globalSeed - num;\n"
            "}\n"
            "minusOne() + minusTwo();",
            .expected = (object_object *) object_create_int(97)
    };
    print_test_separator_line();
    printf("Testing function calls with local bindings\n");
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_closure_with_single_argument(void) {
    vm_testcase test = {
            "let newClosure = fn(a) {\n"
            "   fn() {a;}\n"
            "};\n"
            "let closure = newClosure(99);\n"
            "closure();",
            (object_object *) object_create_int(99)
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_closure_with_two_arguments(void) {
    vm_testcase test = {
            "let newAdder = fn(a, b) {\n"
            "   fn(c) {a + b + c;};\n"
            "}\n"
            "let adder = newAdder(1, 2);\n"
            "adder(8);",
            (object_object *) object_create_int(11)
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_closure_with_inner_variable(void) {
    vm_testcase test = {
            "let newAdder = fn(a, b) {\n"
            "   let c = a + b;\n"
            "   fn(d) { c + d };\n"
            "}\n"
            "let adder = newAdder(1, 2);\n"
            "adder(8);",
            (object_object *) object_create_int(11)
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_nested_closures(void) {
    vm_testcase test = {
            "let newAdderOuter = fn(a, b) {\n"
            "   let c = a + b;\n"
            "   fn(d) {\n"
            "       let e = d + c;\n"
            "       fn(f) {\n"
            "           e + f;\n"
            "       }\n"
            "   }\n"
            "}\n"
            "let newAdderInner = newAdderOuter(1, 2);\n"
            "let adder = newAdderInner(3);\n"
            "adder(8);\n",
            (object_object *) object_create_int(14)
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_closure_with_outer_variable(void) {
    vm_testcase test = {
            "let a = 1;\n"
            "let newAdderOuter = fn(b) {\n"
            "   fn(c) {\n"
            "       fn(d) { a + b + c + d;}\n"
            "   }\n"
            "};\n"
            "let newAdderInner = newAdderOuter(2);\n"
            "let adder = newAdderInner(3);\n"
            "adder(8);",
            (object_object *) object_create_int(14)
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void test_closure_with_multiple_nested_functions(void) {
    vm_testcase test = {
            "let newClosure = fn(a, b) {\n"
            "   let one = fn() {a;}\n"
            "   let two = fn() {b;}\n"
            "   fn() {one() + two();};\n"
            "};\n"
            "let closure = newClosure(9, 90);\n"
            "closure();",
            (object_object *) object_create_int(99)
    };
    run_vm_tests(1, &test);
    object_free(test.expected);
}

static void run_vm_tests(size_t test_count, vm_testcase test_cases[test_count]) {
    for (size_t i = 0; i < test_count; i++) {
        vm_testcase t = test_cases[i];
        printf("Testing vm test for input %s\n", t.input);
        lexer *        lexer    = lexer_init(t.input);
        parser *       parser   = parser_init(lexer);
        ast_program *  program  = parse_program(parser);
        compiler *     compiler = compiler_init();
        compiler_error error    = compile(compiler, (ast_node *) program);
        if (error.error_code != COMPILER_ERROR_NONE) {
            err(EXIT_FAILURE, "compilation failed for input %s with error %s\n",
                t.input, error.msg);
        }
        bytecode *bytecode = get_bytecode(compiler);

        dump_bytecode(bytecode);

        virtual_machine *vm       = vm_init(bytecode);
        vm_error         vm_error = vm_run(vm);


        if (vm_error.code != VM_ERROR_NONE)
            err(EXIT_FAILURE, "vm error: %s\n", vm_error.msg);
        object_object *top = vm_last_popped_stack_elem(vm);
        TEST_ASSERT_NOT_NULL(top);
        test_object_object(top, t.expected);
        parser_free(parser);
        program_free(program);
        compiler_free(compiler);
        bytecode_free(bytecode);
        vm_free(vm);
    }
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_vm_integer_single_value);
    RUN_TEST(test_vm_integer_addition);
    RUN_TEST(test_vm_integer_subtraction);
    RUN_TEST(test_vm_integer_multiplication);
    RUN_TEST(test_vm_integer_division);
    RUN_TEST(test_vm_integer_complex_expression_1);
    RUN_TEST(test_vm_integer_complex_expression_2);
    RUN_TEST(test_vm_integer_exponentiation);
    RUN_TEST(test_vm_integer_mixed_operations_1);
    RUN_TEST(test_vm_integer_mixed_operations_2);
    RUN_TEST(test_vm_integer_parentheses);
    RUN_TEST(test_vm_integer_negative_value_1);
    RUN_TEST(test_vm_integer_negative_value_2);
    RUN_TEST(test_vm_integer_negative_addition);
    RUN_TEST(test_vm_integer_complex_parentheses);
    RUN_TEST(test_boolean_comparisons);
    RUN_TEST(test_basic_boolean_expressions);
    RUN_TEST(test_comparison_expressions);
    RUN_TEST(test_parenthesized_boolean_expressions);
    RUN_TEST(test_nested_and_complex_expressions);
    RUN_TEST(test_if_true);
    RUN_TEST(test_if_false);
    RUN_TEST(test_if_1);
    RUN_TEST(test_if_1_less_2);
    RUN_TEST(test_if_1_less_2_else);
    RUN_TEST(test_if_1_greater_2_else);
    RUN_TEST(test_if_true_else);
    RUN_TEST(test_if_false_else);
    RUN_TEST(test_if_1_greater_2);
    RUN_TEST(test_global_let_stmts);
    RUN_TEST(test_string_expressions);
    RUN_TEST(test_empty_array_literal);
    RUN_TEST(test_simple_array_literal);
    RUN_TEST(test_array_with_expressions);
    RUN_TEST(test_hash_empty_literal);
    RUN_TEST(test_hash_simple_literal);
    RUN_TEST(test_hash_expression_literal);
    RUN_TEST(test_index_array_access);
    RUN_TEST(test_index_array_expression);
    RUN_TEST(test_nested_array_access);
    RUN_TEST(test_empty_array_index);
    RUN_TEST(test_out_of_bounds_array_index);
    RUN_TEST(test_negative_array_index);
    RUN_TEST(test_dict_index_existing_key_1);
    RUN_TEST(test_dict_index_existing_key_2);
    RUN_TEST(test_dict_index_non_existing_key);
    RUN_TEST(test_empty_dict_index);
    RUN_TEST(test_function_no_arguments_single_function);
    RUN_TEST(test_function_no_arguments_multiple_functions);
    RUN_TEST(test_function_no_arguments_nested_calls);
    RUN_TEST(test_function_with_return_statement);
    RUN_TEST(test_functions_without_return_value);
    RUN_TEST(test_first_class_functions);
    RUN_TEST(test_calling_functions_with_bindings_simple);
    RUN_TEST(test_calling_functions_with_bindings_complex);
    RUN_TEST(test_calling_functions_with_bindings_two_functions);
    RUN_TEST(test_calling_functions_with_bindings_two_function_calls);
    RUN_TEST(test_function_with_single_argument);
    RUN_TEST(test_function_with_two_arguments);
    RUN_TEST(test_function_with_local_binding);
    RUN_TEST(test_function_with_multiple_calls);
    RUN_TEST(test_function_nested_calls);
    RUN_TEST(test_function_with_global_variable);
    RUN_TEST(test_calling_functions_with_wrong_arguments);
    RUN_TEST(test_len_with_empty_string);
    RUN_TEST(test_len_with_non_empty_string);
    RUN_TEST(test_len_with_long_string);
    RUN_TEST(test_len_with_integer);
    RUN_TEST(test_len_with_extra_arguments);
    RUN_TEST(test_len_with_array);
    RUN_TEST(test_len_with_empty_array);
    RUN_TEST(test_puts_function);
    RUN_TEST(test_first_with_array);
    RUN_TEST(test_first_with_empty_array);
    RUN_TEST(test_first_with_integer);
    RUN_TEST(test_last_with_array);
    RUN_TEST(test_last_with_empty_array);
    RUN_TEST(test_last_with_integer);
    RUN_TEST(test_rest_with_array);
    RUN_TEST(test_rest_with_empty_array);
    RUN_TEST(test_push_to_empty_array);
    RUN_TEST(test_push_to_integer);
    RUN_TEST(test_calling_functions_with_bindings_global_seed);
    RUN_TEST(test_recursive_closures);
    RUN_TEST(test_recursive_fibonacci);
    RUN_TEST(test_closure_with_single_argument);
    RUN_TEST(test_closure_with_two_arguments);
    RUN_TEST(test_closure_with_inner_variable);
    RUN_TEST(test_nested_closures);
    RUN_TEST(test_closure_with_outer_variable);
    RUN_TEST(test_closure_with_multiple_nested_functions);


    return UNITY_END();
}
