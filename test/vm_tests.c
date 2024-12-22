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
    const char *input;
    object_object *expected;
} vm_testcase;

static void run_vm_tests(size_t test_count, vm_testcase test_cases[test_count]);


static void dump_bytecode(bytecode *bytecode) {
    object_compiled_fn *fn;
    object_int *int_obj;
    printf(" Instructions:\n%s\n", instructions_to_string(bytecode->instructions));
    if (bytecode->constants_pool == NULL)
        return;
    for (size_t i = 0; i < bytecode->constants_pool->size; i++) {
        object_object *constant = (object_object *) arraylist_get(bytecode->constants_pool, i);
        printf("CONSTANT %zu %p %s:\n", i, constant, get_type_name(constant->type));
        switch (constant->type) {
        case OBJECT_COMPILED_FUNCTION:
            fn = (object_compiled_fn *) constant;
            printf(" Instructions:\n%s", instructions_to_string(fn->instructions));
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
test_recursive_fibonacci(void)
{
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

static void test_closures(void) {
    vm_testcase tests[] = {
        {
            "let newClosure = fn(a) {\n"
            "   fn() {a;}\n"
            "};\n"
            "let closure = newClosure(99);\n"
            "closure();",
            (object_object *) object_create_int(99)
        },
        {
            "let newAdder = fn(a, b) {\n"
            "   fn(c) {a + b + c;};\n"
            "}\n"
            "let adder = newAdder(1, 2);\n"
            "adder(8);",
            (object_object *) object_create_int(11)
        },
        {
            "let newAdder = fn(a, b) {\n"
            "   let c = a + b;\n"
            "   fn(d) { c + d };\n"
            "}\n"
            "let adder = newAdder(1, 2);\n"
            "adder(8);",
            (object_object *) object_create_int(11)
        },
        {
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
        },
        {
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
        },
        {
            "let newClosure = fn(a, b) {\n"
            "   let one = fn() {a;}\n"
            "   let two = fn() {b;}\n"
            "   fn() {one() + two();};\n"
            "};\n"
            "let closure = newClosure(9, 90);\n"
            "closure();",
            (object_object *) object_create_int(99)
        }
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++) {
        object_free(tests[i].expected);
    }
}

static void test_boolean_expressions(void) {
    vm_testcase tests[] = {
        {"true", (object_object *) object_create_bool(true)},
        {"false", (object_object *) object_create_bool(false)},
        {"1 < 2", (object_object *) object_create_bool(true)},
        {"1 > 2", (object_object *) object_create_bool(false)},
        {"1 < 1", (object_object *) object_create_bool(false)},
        {"1 > 1", (object_object *) object_create_bool(false)},
        {"1 == 1", (object_object *) object_create_bool(true)},
        {"1 != 1", (object_object *) object_create_bool(false)},
        {"1 == 2", (object_object *) object_create_bool(false)},
        {"1 != 2", (object_object *) object_create_bool(true)},
        {"true == true", (object_object *) object_create_bool(true)},
        {"false == false", (object_object *) object_create_bool(true)},
        {"true == false", (object_object *) object_create_bool(false)},
        {"false != true", (object_object *) object_create_bool(true)},
        {"(1 < 2) == true", (object_object *) object_create_bool(true)},
        {"(1 < 2) == false", (object_object *) object_create_bool(false)},
        {"(1 > 2) == false", (object_object *) object_create_bool(true)},
        {"(1 > 2) == true", (object_object *) object_create_bool(false)},
        {"!(if (false) {5;})", (object_object *) object_create_bool(true)},
        {"if (if (false) {10}) {10} else {20}", (object_object *) object_create_int(20)}
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++) {
        object_free(tests[i].expected);
    }
}

static void test_integer_aritmetic(void) {
    vm_testcase tests[] = {
        {"1", (object_object *) object_create_int(1)},
        {"2", (object_object *) object_create_int(2)},
        {"1 + 2", (object_object *) object_create_int(3)},
        {"1 - 2", (object_object *) object_create_int(-1)},
        {"1 * 2", (object_object *) object_create_int(2)},
        {"4 / 2", (object_object *) object_create_int(2)},
        {"50 / 2 * 2 + 10 - 5", (object_object *) object_create_int(55)},
        {"5 + 5 + 5 + 5 - 10", (object_object *) object_create_int(10)},
        {"2 * 2 * 2 * 2 * 2", (object_object *) object_create_int(32)},
        {"5 * 2 + 10", (object_object *) object_create_int(20)},
        {"5 + 2 * 10", (object_object *) object_create_int(25)},
        {"5 * (2 + 10)", (object_object *) object_create_int(60)},
        {"-5", (object_object *) object_create_int(-5)},
        {"-10", (object_object *) object_create_int(-10)},
        {"-50 + 100 + -50", (object_object *) object_create_int(0)},
        {"(5 + 10 * 2 + 15 / 3) * 2 + -10", (object_object *) object_create_int(50)}
    };

    print_test_separator_line();
    printf("Testing vm for integer arithmetic\n");
    size_t ntests = sizeof(tests)/ sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++) {
        object_free(tests[i].expected);
    }
}

static void test_conditionals(void) {
    vm_testcase tests[] = {
        {"if (true) {10}", (object_object *) object_create_int(10)},
        {"if (true) {10} else {20}", (object_object *) object_create_int(10)},
        {"if (false) {10} else {20}", (object_object *) object_create_int(20)},
        {"if (1) {10}", (object_object *) object_create_int(10)},
        {"if (1 < 2) {10}", (object_object *) object_create_int(10)},
        {"if (1 < 2) {10} else {20}", (object_object *) object_create_int(10)},
        {"if (1 > 2) {10} else {20}", (object_object *) object_create_int(20)},
        {"if (false) {10}", (object_object *) object_create_null()},
        {"if (1 > 2) {10}", (object_object *) object_create_null()}
    };
    print_test_separator_line();
    printf("Testing conditionals\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++) {
        object_free(tests[i].expected);
    }
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

static object_array * create_monkey_int_array(size_t count, ...) {
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

static void test_array_literals(void) {
    vm_testcase tests[] = {
        {"[]", (object_object *) create_monkey_int_array(0)},
        {"[1, 2, 3]", (object_object *) create_monkey_int_array(3, 1, 2, 3)},
        {"[1 + 2, 3  * 4, 5 + 6]", (object_object *) create_monkey_int_array(3, 3, 12, 11)}
    };
    print_test_separator_line();
    printf("Testing array literals\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        object_free(tests[i].expected);
}

static object_hash *create_hash_table(size_t n, object_object *objects[n]) {
    hashtable *table = hashtable_create(object_get_hash, object_equals, NULL, NULL);
    for (size_t i = 0; i < n; i += 2) {
        object_object *key = objects[i];
        object_object *value = objects[i + 1];
        hashtable_set(table, key, value);
    }
    return object_create_hash(table);
}

static void
test_hash_literals(void)
{
    vm_testcase tests[] = {
        {"{}", (object_object *) create_hash_table(0, NULL)},
        {"{1: 2, 3: 4}", (object_object *) create_hash_table((size_t) 4, (object_object *[4])
            {
                (object_object *) object_create_int(1),
                (object_object *) object_create_int(2),
                (object_object *) object_create_int(3),
                (object_object *) object_create_int(4)
            })
        },
        {"{1 + 1: 2 * 2, 3 + 3: 4 * 4}", (object_object *) create_hash_table((size_t) 4, (object_object *[4])
            {
                (object_object *) object_create_int(2),
                (object_object *) object_create_int(4),
                (object_object *) object_create_int(6),
                (object_object *) object_create_int(16)
            })
        }
    };
    print_test_separator_line();
    printf("Testing hash literals\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        object_free(tests[i].expected);
}

static void
test_index_expresions(void)
{
    vm_testcase tests[] = {
        {"[1, 2, 3][1]", (object_object *) object_create_int(2)},
        {"[1, 2, 3][0 + 2]", (object_object *) object_create_int(3)},
        {"[[1, 1, 1]][0][0]", (object_object *) object_create_int(1)},
        {"[][0]", (object_object *) object_create_null()},
        {"[1, 2, 3][99]", (object_object *) object_create_null()},
        {"[1][-1]", (object_object *) object_create_null()},
        {"{1: 1, 2: 2}[1]", (object_object *) object_create_int(1)},
        {"{1: 1, 2: 2}[2]", (object_object *) object_create_int(2)},
        {"{1: 1}[0]", (object_object *) object_create_null()},
        {"{}[0]", (object_object *) object_create_null()}
    };
    print_test_separator_line();
    printf("Testing index expressions\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        object_free(tests[i].expected);
}

static void
test_functions_without_arguments(void)
{
    vm_testcase tests[] = {
        {"let fivePlusTen = fn() {5 + 10;}; fivePlusTen();", (object_object *) object_create_int(15)},
        {"let one = fn() {1;}\n let two = fn() {2;}\n one() + two();", (object_object *) object_create_int(3)},
        {"let a = fn() {1};\n let b = fn() {a() + 1};\n let c = fn() {b() + 1;};\n c();", (object_object *) object_create_int(3)}
    };
    print_test_separator_line();
    printf("Testing functions without arguments\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        object_free(tests[i].expected);
}

static void
test_function_with_return_statement(void)
{
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
test_functions_without_return_value(void)
{
    vm_testcase tests[] = {
        {"let noReturn = fn() {};\n noReturn();", (object_object *) object_create_null()},
        {"let noReturn = fn() {};\n let noReturnTwo = fn() {noReturn();}\n noReturn();\n noReturnTwo();", (object_object *) object_create_null()}
    };
    print_test_separator_line();
    printf("Testing functions without return value\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        object_free(tests[i].expected);
}

static void
test_first_class_functions(void)
{
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

static void
test_calling_functions_with_wrong_arguments(void)
{
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
        lexer *lexer = lexer_init(t.input);
        parser *parser = parser_init(lexer);
        ast_program *program = parse_program(parser);
        compiler *compiler = compiler_init();
        compiler_error error = compile(compiler, (ast_node *) program);
        if (error.error_code != COMPILER_ERROR_NONE)
            err(EXIT_FAILURE, "compilation failed for input %s with error %s\n",
                t.input, error.msg);
        bytecode *bytecode = get_bytecode(compiler);
        // dump_bytecode(bytecode);
        virtual_machine *vm = vm_init(bytecode);
        vm_error vm_error = vm_run(vm);

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

static void test_calling_functions_with_bindings_and_arguments(void) {
    vm_testcase tests[] = {
        {
            "let identity = fn(a) {a};\n"
            "identity(4);",
            (object_object *) object_create_int(4)
        },
        {
            "let sum = fn(a, b) { a + b;};\n"
            "sum(1, 2);",
            (object_object *) object_create_int(3)
        },
        {
            "let sum = fn(a, b) {\n"
            "  let c = a + b;\n"
            "  c;\n"
            "};\n"
            "sum(1, 2);",
            (object_object *) object_create_int(3)
        },
        {
            "let sum = fn(a, b) {\n"
            "  let c = a + b;\n"
            "  c;\n"
            "};\n"
            "sum(1, 2) + sum(3, 4);",
            (object_object *) object_create_int(10)
        },
        {
            "let sum = fn(a, b) {\n"
            "  let c = a + b;\n"
            "  c;\n"
            "};\n"
            "let outer = fn() {\n"
            "  sum(1, 2) + sum(3, 4);\n"
            "};\n"
            "outer();",
            (object_object *) object_create_int(10)
        },
        {
            "let globalNum = 10;\n"
            "let sum = fn(a, b) {\n"
            "  let c = a + b;\n"
            "  c + globalNum;\n"
            "};\n"
            "let outer = fn() {\n"
            "  sum(1, 2) + sum(3, 4) + globalNum;\n"
            "};\n"
            "outer() + globalNum;",
            (object_object *) object_create_int(50)
        }
    };
    print_test_separator_line();
    printf("Testing functions with bindings and arguments\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++) {
        object_free(tests[i].expected);
    }
}

static object_array *create_int_array(int *int_arr, size_t length) {
    arraylist *array_list = arraylist_create(length, NULL);
    for (size_t i = 0; i < length; i++) {
        arraylist_add(array_list, (void *) object_create_int(int_arr[i]));
    }
    return object_create_array(array_list);
}

static void test_builtin_functions(void) {
    vm_testcase tests[] = {
        {
            "len(\"\")",
            (object_object *) object_create_int(0)
        },
        {
            "len(\"four\")",
            (object_object *) object_create_int(4)
        },
        {
            "len(\"hello world\")",
            (object_object *) object_create_int(11)
        },
        {
            "len(1)",
            (object_object *) object_create_error("argument to `len` not supported, got INTEGER")
        },
        {
            "len(\"one\", \"two\")",
            (object_object *) object_create_error("wrong number of arguments. got=2, want=1")
        },
        {
            "len([1, 2, 3])",
            (object_object *) object_create_int(3)
        },
        {
            "len([])",
            (object_object *) object_create_int(0)
        },
        {
            "puts(\"hello\", \"world!\")",
            (object_object *) object_create_null()
        },
        {
            "first([1, 2, 3])",
            (object_object *) object_create_int(1)
        },
        {
            "first([])",
            (object_object *) object_create_null()
        },
        {
            "first(1)",
            (object_object *) object_create_error("argument to `first` must be ARRAY, got INTEGER")
        },
        {
            "last([1, 2, 3])",
            (object_object *) object_create_int(3)
        },
        {
            "last([])",
            (object_object *) object_create_null()
        },
        {
            "last(1)",
            (object_object *) object_create_error("argument to `last` must be ARRAY, got INTEGER")
        },
        {
            "rest([1, 2, 3])",
            (object_object *) create_int_array((int[]) {2, 3}, 2)
        },
        {
            "rest([])",
            (object_object *) object_create_null()
        },
        {
            "push([], 1)",
            (object_object *) create_int_array((int[]) {1}, 1)
        },
        {
            "push(1, 1)",
            (object_object *) object_create_error("argument to `push` must be ARRAY, got INTEGER")
        }
    };
    print_test_separator_line();
    printf("Testing builtin functions\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++) {
        object_free(tests[i].expected);
    }
}

static void test_calling_functions_with_bindings(void) {
    vm_testcase tests[] = {
        {
            "let one = fn() {let one = 1; one;}; one();",
            (object_object *) object_create_int(1)
        },
        {
            "let oneAndTwo = fn() {let one = 1; let two = 2; one + two;}; oneAndTwo();",
            (object_object *) object_create_int(3)
        },
        {
            "let oneAndTwo = fn() {let one = 1; let two = 2; one + two;}\n"
            "let threeAndFour = fn() {let three = 3; let four = 4; three + four;}\n"
            "oneAndTwo() + threeAndFour();",
            (object_object *) object_create_int(10)
        },
        {
            "let firstFooBar = fn() {let foobar = 50; foobar;}\n"
            "let secondFooBar = fn() { let foobar = 100; foobar;};\n"
            "firstFooBar() + secondFooBar();",
            (object_object *) object_create_int(150)
        },
        {
            "let globalSeed = 50;\n"
            "let minusOne = fn() {\n"
            "  let num = 1;\n"
            "  globalSeed - num;\n"
            "}\n"
            "let minusTwo = fn() {\n"
            "  let num = 2;\n"
            "  globalSeed - num;\n"
            "}\n"
            "minusOne() + minusTwo();",
            (object_object *) object_create_int(97)
        }
    };
    print_test_separator_line();
    printf("Testing function calls with local bindings\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++) {
        object_free(tests[i].expected);
    }

}

static void run_vm_tests(size_t test_count, vm_testcase test_cases[test_count]) {
    for (size_t i = 0; i < test_count; i++) {
        vm_testcase t = test_cases[i];
        printf("Testing vm test for input %s\n", t.input);
        lexer *lexer = lexer_init(t.input);
        parser *parser = parser_init(lexer);
        ast_program *program = parse_program(parser);
        compiler *compiler = compiler_init();
        compiler_error error = compile(compiler, (ast_node *) program);
        if (error.error_code != COMPILER_ERROR_NONE) {
            err(EXIT_FAILURE, "compilation failed for input %s with error %s\n",
                t.input, error.msg);
        }
        bytecode *bytecode = get_bytecode(compiler);
        virtual_machine *vm = vm_init(bytecode);
        vm_error vm_error = vm_run(vm);
        if (vm_error.code != VM_ERROR_NONE)
            err(EXIT_FAILURE, "vm error: %s\n", vm_error.msg);
        object_object *top = vm_last_popped_stack_elem(vm);
        test_object_object(top, t.expected);
        object_free(top);
        parser_free(parser);
        program_free(program);
        compiler_free(compiler);
        bytecode_free(bytecode);
        vm_free(vm);
    }
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_integer_aritmetic);
    RUN_TEST(test_boolean_expressions);
    RUN_TEST(test_conditionals);
    RUN_TEST(test_global_let_stmts);
    RUN_TEST(test_string_expressions);
    RUN_TEST(test_array_literals);
    RUN_TEST(test_hash_literals);
    RUN_TEST(test_index_expresions);
    RUN_TEST(test_functions_without_arguments);
    RUN_TEST(test_function_with_return_statement);
    RUN_TEST(test_functions_without_return_value);
    RUN_TEST(test_first_class_functions);
    RUN_TEST(test_calling_functions_with_bindings);
    RUN_TEST(test_calling_functions_with_bindings_and_arguments);
    RUN_TEST(test_calling_functions_with_wrong_arguments);
    RUN_TEST(test_builtin_functions);
    RUN_TEST(test_closures);
    RUN_TEST(test_recursive_closures);
    RUN_TEST(test_recursive_fibonacci);

    return UNITY_END();
}
