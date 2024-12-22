//
// Created by dgood on 12/18/24.
//

#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "../Unity/src/unity.h"
#include "../src/datastructures/conversions.h"
#include "../src/datastructures/hashmap.h"
#include "../src/object/environment.h"
#include "../src/evaluator/evaluator.h"
#include "../src/lexer/lexer.h"
#include "../src/object/object.h"
#include "object_test_utils.h"
#include "../src/parser/parser.h"
#include "test_utils.h"

void setUp(void) {
    // Optional setup before each test
}

void tearDown(void) {
    // Optional cleanup after each test
}

static object_object *test_eval(const char *input, environment *env) {
    lexer *        lexer   = lexer_init(input);
    parser *       parser  = parser_init(lexer);
    ast_program *  program = parse_program(parser);
    object_object *obj     = evaluator_eval((ast_node *) program, env);
    program_free(program);
    parser_free(parser);
    return obj;
}

static void test_eval_integer_expression(void) {
    environment *env;
    typedef struct {
        const char *input;
        long        expected_value;
    } test_input;

    test_input tests[] = {
            {"5", 5},
            {"10", 10},
            {"0", 0},
            {"-5", -5},
            {"-10", -10},
            {"5 + 5 + 5 + 5 - 10", 10},
            {"2 * 2 * 2 * 2 * 2", 32},
            {"-50 + 100 - 50", 0},
            {"5 * 2 + 10", 20},
            {"5 + 2 * 10", 25},
            {"20 + 2 * -10", 0},
            {"50 / 2 * 2 + 10", 60},
            {"2 * (5 + 10)", 30},
            {"3 * 3 * 3 + 10", 37},
            {"3 * (3 * 3) + 10", 37},
            {"(5 + 10 * 2 + 15 / 3) * 2 + -10", 50},
            {"5 % 2", 1},
            {"(3 + 2) % 5", 0},
            {"(10 % 3) + 1", 2}
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing eval for integer expression: %s\n", test.input);
        env                = environment_create();
        object_object *obj = test_eval(test.input, env);
        test_integer_object(obj, test.expected_value);
        object_free(obj);
        environment_free(env);
    }
    printf("integer expression eval test passed\n");
}

static void test_eval_bool_expression(void) {
    environment *env;
    typedef struct {
        const char *input;
        _Bool       expected_value;
    } test_input;

    test_input tests[] = {
            {"true", true},
            {"false", false},
            {"1 < 2", true},
            {"1 > 2", false},
            {"1 < 1", false},
            {"1 > 1", false},
            {"1 == 1", true},
            {"1 != 1", false},
            {"1 == 2", false},
            {"1 != 2", true},
            {"true == true", true},
            {"true == false", false},
            {"false == false", true},
            {"false == true", false},
            {"true != true", false},
            {"true != false", true},
            {"false != false", false},
            {"false != true", true},
            {"(1 < 2) == true", true},
            {"(1 < 2) == false", false},
            {"(1 > 2) == true", false},
            {"(1 > 2) == false", true},
            {"5 > 10 || 5 > 1", true},
            {"2 + 1 > 1 && 3 - 1 != 3", true},
            {"2 != 1 && false", false},
            {"2 != 1 || false", true}
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing eval for boolean expression: %s\n", test.input);
        env                = environment_create();
        object_object *obj = test_eval(test.input, env);
        test_boolean_object(obj, test.expected_value);
        object_free(obj);
        environment_free(env);
    }
    printf("boolean expression eval test passed\n");
}

static void test_bang_operator(void) {
    environment *env;

    typedef struct {
        const char *input;
        _Bool       expected;
    } test_input;

    test_input tests[] = {
            {"!true", false},
            {"!false", true},
            {"!5", false},
            {"!!true", true},
            {"!!false", false},
            {"!!5", true},
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing bang operator for expression: %s\n", test.input);
        env                = environment_create();
        object_object *obj = test_eval(test.input, env);
        test_boolean_object(obj, test.expected);
        object_free(obj);
        environment_free(env);
    }
}

static void test_while_expressions(void) {
    environment *env;
    typedef struct {
        const char *   input;
        object_object *expected;
    } test_input;

    test_input tests[] = {
            {
                    "let x = 10;\n"\
                    "while (x > 1) {\n"\
                    "   let x = x -1;\n"\
                    "   x;\n"\
                    "};",
                    (object_object *) object_create_int(1)
            },
            {
                    "let x = 10;\n"\
                    "while (x > 10) {\n"\
                    "   let x = x -1;\n"\
                    "   x;\n"\
                    "};",
                    (object_object *) object_create_null()
            }
    };
    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing while expression evaluation for %s\n", test.input);
        env                      = environment_create();
        object_object *evaluated = test_eval(test.input, env);
        TEST_ASSERT_EQUAL_INT(evaluated->type, test.expected->type);
        if (evaluated->type == OBJECT_INT) {
            object_int *actual       = (object_int *) evaluated;
            object_int *expected_int = (object_int *) test.expected;
            TEST_ASSERT_EQUAL_INT64(actual->value, expected_int->value);
        }
        environment_free(env);
        object_free(evaluated);
        object_free(test.expected);
    }
}

static void test_if_else_expressions(void) {
    environment *env;
    typedef struct {
        const char *   input;
        object_object *expected;
    } test_input;

    test_input tests[] = {
            {"if (true) { 10 }", (object_object *) object_create_int(10)},
            {"if (false) { 10 }", (object_object *) object_create_null()},
            {"if (1) { 10 }", (object_object *) object_create_int(10)},
            {"if (1 < 2) { 10 }", (object_object *) object_create_int(10)},
            {"if (1 > 2) { 10 }", (object_object *) object_create_null()},
            {"if (1 > 2) { 10 } else { 20 }", (object_object *) object_create_int(20)},
            {"if (1 < 2) { 10 } else { 20 }", (object_object *) object_create_int(10)}
    };
    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing if else expression evaluation for \"%s\"\n", test.input);
        env                      = environment_create();
        object_object *evaluated = test_eval(test.input, env);
        if (test.expected->type == OBJECT_INT) {
            object_int *expected_int = (object_int *) test.expected;
            test_integer_object(evaluated, expected_int->value);
        } else
            test_null_object(evaluated);
        object_free(test.expected);
        environment_free(env);
        object_free(evaluated);
    }
}

static void
test_return_statements(void) {
    environment *env;
    typedef struct {
        const char *   input;
        object_object *expected;
    } test_input;

    test_input tests[] = {
            {"return 10;", (object_object *) object_create_int(10)},
            {"return 10; 9;", (object_object *) object_create_int(10)},
            {"return 2 * 5; 9;", (object_object *) object_create_int(10)},
            {"9; return 2 * 5; 9", (object_object *) object_create_int(10)},
            {"if (10 > 1) {\n"\
             "   if (10 > 1) {\n"\
             "return 10;\n"\
             "}\n"\
             "return 1;\n"\
             "}\n",
             (object_object *) object_create_int(10)}
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing return statement evaluation for \"%s\"\n", test.input);
        env                      = environment_create();
        object_object *evaluated = test_eval(test.input, env);
        test_object_object(evaluated, test.expected);
        object_free(test.expected);
        object_free(evaluated);
        environment_free(env);
    }
}

static void
test_error_handling(void) {
    typedef struct {
        const char *input;
        const char *message;
    }          test_input;
    test_input tests[] = {
            {
                    "5 + true;",
                    "type mismatch: INTEGER + BOOLEAN"
            },
            {
                    "5 + true; 5;",
                    "type mismatch: INTEGER + BOOLEAN"
            },
            {
                    "-true",
                    "unknown operator: -BOOLEAN"
            },
            {
                    "true + false;",
                    "unknown operator: BOOLEAN + BOOLEAN"
            },
            {
                    "5; true + false; 5",
                    "unknown operator: BOOLEAN + BOOLEAN"
            },
            {
                    "if (10 > 1) { true + false;}",
                    "unknown operator: BOOLEAN + BOOLEAN"
            },
            {
                    "if (10 > 1) {\n"\
                    "   if (10 > 1) {\n"\
                    "       return true + false;\n"\
                    "   }\n"\
                    "   return 1;\n"\
                    "}",
                    "unknown operator: BOOLEAN + BOOLEAN"
            },
            {
                    "foobar;",
                    "identifier not found: foobar"
            },
            {
                    "\"Hello\" - \"World\"",
                    "unknown operator: STRING - STRING"
            },
            {
                    "{\"name\": \"Monkey\"}[fn(x) {x}]",
                    "unusable as a hash key: FUNCTION"
            },
            {
                    "let x = 10;\n"\
                    "let y = if (x == 10) {\n"\
                    "   let x = x + 1;\n"\
                    "} else {\n"\
                    "   let x = x * 2;\n"\
                    "};\n"\
                    "y + 1",
                    "type mismatch: NULL + INTEGER"
            },
            {
                    "1 / 0",
                    "division by 0 not allowed"
            },
            {
                    "5 % 0",
                    "division by 0 not allowed"
            }
    };

    print_test_separator_line();
    environment *env;
    size_t       ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Test error handling for %s\n", test.input);
        env                      = environment_create();
        object_object *evaluated = test_eval(test.input, env);
        TEST_ASSERT_EQUAL_INT(evaluated->type, OBJECT_ERROR);
        object_error *err = (object_error *) evaluated;
        TEST_ASSERT_EQUAL_STRING(err->message, test.message);
        object_free(evaluated);
        environment_free(env);
    }
}

static void
test_let_statements(void) {
    environment *env;
    typedef struct {
        const char *input;
        long        expected;
    } test_input;

    test_input tests[] = {
            {
                    "let a = 5; a;", 5
            },
            {
                    "let a = 5 * 5; a;", 25
            },
            {
                    "let a = 5; let b = a; b;", 5
            },
            {
                    "let a = 5; let b = a; let c = a + b + 5; c;", 15
            }
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        env             = environment_create();
        printf("Testing let statement for %s\n", test.input);
        object_object *evaluated = test_eval(test.input, env);
        test_integer_object(evaluated, test.expected);
        object_free(evaluated);
        environment_free(env);
    }
}

static void test_function_object(void) {
    const char * input = "fn(x) { x + 2;};";
    environment *env   = environment_create();
    print_test_separator_line();
    printf("Testing function object\n");
    object_object *evaluated = test_eval(input, env);
    TEST_ASSERT_EQUAL_INT(evaluated->type, OBJECT_FUNCTION);
    object_function *function_obj = (object_function *) evaluated;
    TEST_ASSERT_EQUAL_INT(function_obj->parameters->size, 1);
    ast_identifier *first_param = (ast_identifier *) function_obj->parameters->head->data;
    TEST_ASSERT_EQUAL_STRING(first_param->value, "x");
    const char *expected_body = "(x + 2)";
    char *      actual_body   = function_obj->body->statement.node.string(function_obj->body);
    TEST_ASSERT_EQUAL_STRING(expected_body, actual_body);
    free(actual_body);
    environment_free(env);
    object_free(evaluated);
}

static void
test_function_application(void) {
    typedef struct {
        const char *input;
        long        expected;
    } test_input;

    test_input tests[] = {
            {
                    "let identity = fn(x) {x;}; identity(5);",
                    5
            },
            {
                    "let identity = fn(x) { return x;}; identity(5);",
                    5
            },
            {
                    "let double = fn(x) { x * 2;} double(5);",
                    10
            },
            {
                    "let add = fn(x, y) {x + y;}; add(5, 5);",
                    10
            },
            {
                    "let add = fn(x, y) {x + y;}; add(5 + 5, add(5, 5));", 20
            },
            {
                    "fn(x) { x; }(5)",
                    5
            }
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing function application for %s\n", test.input);
        environment *  env       = environment_create();
        object_object *evaluated = test_eval(test.input, env);
        test_integer_object(evaluated, test.expected);
        environment_free(env);
        object_free(evaluated);
    }
}

static void test_string_literal(void) {
    const char *   input     = "\"Hello, world!\"";
    environment *  env       = environment_create();
    object_object *evaluated = test_eval(input, env);
    print_test_separator_line();
    printf("Testing string literal evaluation\n");
    TEST_ASSERT_EQUAL_INT(evaluated->type, OBJECT_STRING);
    object_string *str = (object_string *) evaluated;
    TEST_ASSERT_EQUAL_STRING(str->value, "Hello, world!");
    object_free(str);
    environment_free(env);
}

static void test_string_concatenation(void) {
    const char *   input     = "\"Hello,\" + \" \" + \"world!\"";
    environment *  env       = environment_create();
    object_object *evaluated = test_eval(input, env);
    print_test_separator_line();
    printf("Testing string concatenation evaluation\n");
    TEST_ASSERT_EQUAL_INT(evaluated->type, OBJECT_STRING);
    object_string *str = (object_string *) evaluated;
    TEST_ASSERT_EQUAL_STRING(str->value, "Hello, world!");
    object_free(str);
    environment_free(env);
}

static void test_int_array(object_array *actual, object_array *expected) {
    TEST_ASSERT_EQUAL_UINT(expected->elements->size, actual->elements->size);
    for (size_t i = 0; i < expected->elements->size; i++) {
        object_object *obj = (object_object *) expected->elements->body[i];
        TEST_ASSERT_EQUAL_INT(obj->type, OBJECT_INT);
        object_int *act_int = (object_int *) obj;
        object_int *exp_int = (object_int *) expected->elements->body[i];
        TEST_ASSERT_EQUAL_INT(act_int->value, exp_int->value);
    }
}

static object_array *create_int_array(int *int_arr, size_t length) {
    arraylist *array_list = arraylist_create(length, NULL);
    for (size_t i = 0; i < length; i++) {
        arraylist_add(array_list, (void *) object_create_int(int_arr[i]));
    }
    return object_create_array(array_list);
}

static void test_builtins(void) {
    typedef struct {
        const char *   input;
        object_object *expected;
    } test_input;

    test_input tests[] = {
            {"len(\"\")", (object_object *) object_create_int(0)},
            {"len(\"four\")", (object_object *) object_create_int(4)},
            {"len(\"hello world\")", (object_object *) object_create_int(11)},
            {"len(1)", (object_object *) object_create_error("argument to `len` not supported, got INTEGER")},
            {"len(\"one\", \"two\")",
             (object_object *) object_create_error("wrong number of arguments. got=2, want=1")},
            {"len([1, 2, 3])", (object_object *) object_create_int(3)},
            {"len([])", (object_object *) object_create_int(0)},
            {"first([1, 2, 3])", (object_object *) object_create_int(1)},
            {"first([])", (object_object *) object_create_null()},
            {"first(1)", (object_object *) object_create_error("argument to `first` must be ARRAY, got INTEGER")},
            {"last([1, 2, 3])", (object_object *) object_create_int(3)},
            {"last([])", (object_object *) object_create_null()},
            {"last(1)", (object_object *) object_create_error("argument to `last` must be ARRAY, got INTEGER")},
            {"rest([1, 2, 3])", (object_object *) create_int_array((int[]){2, 3}, 2)},
            {"rest([])", (object_object *) object_create_null()},
            {"push([], 1)", (object_object *) create_int_array((int[]){1}, 1)},
            {"push(1, 1)", (object_object *) object_create_error("argument to `push` must be ARRAY, got INTEGER")},
            {"type(10)", (object_object *) object_create_string("INTEGER", 7)},
            {"type(10, 1)", (object_object *) object_create_error("wrong number of arguments. got=2, want=1")}
    };

    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    print_test_separator_line();
    object_int *   actual_int;
    object_array * actual_array;
    object_array * expected_array;
    object_error * actual_err;
    object_error * expected_err;
    object_string *expected_str;
    object_string *actual_str;
    object_object *obj;
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing builtin function %s\n", test.input);
        environment *  env       = environment_create();
        object_object *evaluated = test_eval(test.input, env);
        switch (test.expected->type) {
            case OBJECT_INT:
                actual_int = (object_int *) evaluated;
                test_integer_object(evaluated, actual_int->value);
                object_free(test.expected);
                object_free(evaluated);
                break;
            case OBJECT_ERROR:
                actual_err = (object_error *) evaluated;
                expected_err = (object_error *) test.expected;
                TEST_ASSERT_EQUAL_STRING(actual_err->message, expected_err->message);
                object_free(test.expected);
                object_free(evaluated);
                break;
            case OBJECT_ARRAY:
                actual_array = (object_array *) evaluated;
                expected_array = (object_array *) test.expected;
                test_int_array(actual_array, expected_array);
                object_free(test.expected);
                object_free(evaluated);
                break;
            case OBJECT_NULL:
                obj = (object_object *) evaluated;
                TEST_ASSERT_EQUAL_INT(obj->type, OBJECT_NULL);
                object_free(evaluated);
                break;
            case OBJECT_STRING:
                expected_str = (object_string *) test.expected;
                actual_str = (object_string *) evaluated;
                TEST_ASSERT_EQUAL_STRING(expected_str->value, actual_str->value);
                object_free(test.expected);
                object_free(evaluated);
                break;
            default:
                object_free(evaluated);
                err(EXIT_FAILURE, "Unknown type for expected");
        }
        environment_free(env);
    }
}

static void test_array_literals(void) {
    const char *input = "[1, 2 * 2, 3 + 3]";
    print_test_separator_line();
    printf("Testing array literal evaluation\n");
    environment *  env       = environment_create();
    object_object *evaluated = test_eval(input, env);
    TEST_ASSERT_EQUAL_INT(evaluated->type, OBJECT_ARRAY);
    object_array *array = (object_array *) evaluated;
    TEST_ASSERT_EQUAL_INT(array->elements->size, 3);
    test_integer_object(array->elements->body[0], 1);
    test_integer_object(array->elements->body[1], 4);
    test_integer_object(array->elements->body[2], 6);
    object_free(evaluated);
    environment_free(env);
}

static void test_array_index_expressions(void) {
    typedef struct {
        const char *   input;
        object_object *expected;
    } test_input;

    test_input tests[] = {
            {"[1, 2, 3][0]", (object_object *) object_create_int(1)},
            {"[1, 2, 3][1]", (object_object *) object_create_int(2)},
            {"[1, 2, 3][2]", (object_object *) object_create_int(3)},
            {"let i = 0; [1][i]", (object_object *) object_create_int(1)},
            {"[1, 2, 3][1 + 1]", (object_object *) object_create_int(3)},
            {"let my_array = [1, 2, 3]; my_array[2];", (object_object *) object_create_int(3)},
            {"let my_array = [1, 2, 3]; my_array[0] + my_array[1] + my_array[2]",
             (object_object *) object_create_int(6)},
            {"let my_array = [1, 2, 3]; let i = my_array[0]; my_array[i]",
             (object_object *) object_create_int(2)},
            {"[1, 2, 3][3]", (object_object *) object_create_null()},
            {"[1, 2, 3][-1]", (object_object *) object_create_null()},
            {"\"apple\"[0]", (object_object *) object_create_string("a", 1)},
            {"\"apple\"[3]", (object_object *) object_create_string("l", 1)}
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing index expression evaluation for %s\n", test.input);
        environment *  env       = environment_create();
        object_object *evaluated = test_eval(test.input, env);
        TEST_ASSERT_EQUAL_INT(evaluated->type, test.expected->type);
        if (test.expected->type == OBJECT_INT) {
            test_integer_object(evaluated, ((object_int *) test.expected)->value);
            object_free(test.expected);
            object_free(evaluated);
        } else if (test.expected->type == OBJECT_STRING) {
            TEST_ASSERT_EQUAL_INT(evaluated->type, OBJECT_STRING);
            object_string *actual_string   = (object_string *) evaluated;
            object_string *expected_string = (object_string *) test.expected;
            TEST_ASSERT_EQUAL_STRING(expected_string->value, actual_string->value);
            object_free(test.expected);
            object_free(evaluated);
        } else {
            test_null_object(evaluated);
            object_free(evaluated);
        }
        environment_free(env);
    }
}

static void test_enclosing_env(void) {
    const char *input = "let first = 10;\n"\
            "let second = 10;\n"\
            "let third = 10;\n\n"\
            "let ourfunction = fn(first) {\n"\
            "   let second = 20;\n"\
            "   first + second + third;\n"\
            "};\n"\
            "ourfunction(20) + first + second;";
    print_test_separator_line();
    printf("Testing enclosed environment\n");
    environment *  env       = environment_create();
    object_object *evaluated = test_eval(input, env);
    printf("Enclosed environment test passed\n");
    test_integer_object(evaluated, 70);
    object_free(evaluated);
    environment_free(env);
}

static void test_hash_literals(void) {
    const char *input = "let two = \"two\";\n"\
            "{\"one\": 10 - 9,\n"\
            "two: 1 + 1,\n"\
            "\"thr\" + \"ee\": 6 / 2,\n"\
            "4: 4,\n"\
            "true: 5,\n"\
            "false: 6\n"\
            "}";

    typedef struct kv {
        object_object *key;
        object_object *value;
    }  kv;
    kv expected[] = {
            {
                    (object_object *) object_create_string("one", 3),
                    (object_object *) object_create_int(1)
            },
            {
                    (object_object *) object_create_string("two", 3),
                    (object_object *) object_create_int(2)
            },
            {
                    (object_object *) object_create_string("three", 5),
                    (object_object *) object_create_int(3)
            },
            {
                    (object_object *) object_create_int(4),
                    (object_object *) object_create_int(4)
            },
            {
                    (object_object *) object_create_bool(true),
                    (object_object *) object_create_int(5)
            },
            {
                    (object_object *) object_create_bool(false),
                    (object_object *) object_create_int(6)
            }
    };
    print_test_separator_line();
    printf("Testing hash literal evaluation for %s\n", input);
    environment *  env       = environment_create();
    object_object *evaluated = test_eval(input, env);
    TEST_ASSERT_EQUAL_INT(evaluated->type, OBJECT_HASH);
    object_hash *hash_obj            = (object_hash *) evaluated;
    size_t       expected_objs_count = sizeof(expected) / sizeof(expected[0]);
    TEST_ASSERT_EQUAL_INT(hash_obj->pairs->key_count, expected_objs_count);

    for (size_t i = 0; i < expected_objs_count; i++) {
        object_object *key            = expected[i].key;
        char *         key_string     = key->inspect(key);
        object_object *expected_value = expected[i].value;
        object_object *actual_value   = (object_object *) hashtable_get(hash_obj->pairs, key);
        TEST_ASSERT_NOT_NULL(actual_value);
        test_object_object(actual_value, expected_value);
        free(key_string);
        object_free(key);
        object_free(expected_value);
    }
    object_free(evaluated);
    environment_free(env);
}

static void test_hash_index_expressions(void) {
    typedef struct {
        const char *   input;
        object_object *expected;
    } test_input;

    test_input tests[] = {
            {
                    "{\"foo\": 5}[\"foo\"]",
                    (object_object *) object_create_int(5)
            },
            {
                    "{\"foo\": 5}[\"bar\"]",
                    (object_object *) object_create_null()
            },
            {
                    "let key = \"foo\"; {\"foo\": 5}[key]",
                    (object_object *) object_create_int(5)
            },
            {
                    "{}[\"foo\"]",
                    (object_object *) object_create_null()
            },
            {
                    "{5: 5}[5]",
                    (object_object *) object_create_int(5)
            },
            {
                    "{true: 5}[true]",
                    (object_object *) object_create_int(5)
            },
            {
                    "{false: 5}[false]",
                    (object_object *) object_create_int(5)
            }
    };

    print_test_separator_line();
    size_t      ntests = sizeof(tests) / sizeof(tests[0]);
    object_int *expected_int;
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing hash index expression for %s\n", test.input);
        environment *  env       = environment_create();
        object_object *evaluated = test_eval(test.input, env);
        switch (test.expected->type) {
            case OBJECT_INT:
                expected_int = (object_int *) test.expected;
                test_integer_object(evaluated, expected_int->value);
                break;
            case OBJECT_NULL:
                TEST_ASSERT_EQUAL_INT(evaluated->type, OBJECT_NULL);
                break;
            default:
                err(EXIT_FAILURE, "Unknown type: %s", get_type_name(test.expected->type));
        }
        object_free(test.expected);
        object_free(evaluated);
        environment_free(env);
    }
}

static void test_string_comparison(void) {
    typedef struct {
        const char *input;
        _Bool       expected;
    } test_input;

    test_input tests[] = {
            {
                    "let s1 = \"apple\";\n"\
                    "let s2 = \"apple\";\n"\
                    "s1 == s2;",
                    true
            },
            {
                    "let s1 = \"apple\";\n"\
                    "let s2 = \"apples\";\n"\
                    "s1 == s2;",
                    false
            }
    };
    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing string comparison for %s\n", test.input);
        environment *  env       = environment_create();
        object_object *evaluated = test_eval(test.input, env);
        TEST_ASSERT_EQUAL_INT(evaluated->type, OBJECT_BOOL);
        object_bool *actual = (object_bool *) evaluated;
        TEST_ASSERT_EQUAL_INT(actual->value, test.expected);
        object_free(evaluated);
        environment_free(env);
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_eval_integer_expression);
    RUN_TEST(test_eval_bool_expression);
    RUN_TEST(test_bang_operator);
    RUN_TEST(test_if_else_expressions);
    RUN_TEST(test_return_statements);
    RUN_TEST(test_error_handling);
    RUN_TEST(test_let_statements);
    RUN_TEST(test_function_object);
    RUN_TEST(test_function_application);
    RUN_TEST(test_string_literal);
    RUN_TEST(test_string_concatenation);
    RUN_TEST(test_builtins);
    RUN_TEST(test_array_literals);
    RUN_TEST(test_array_index_expressions);
    RUN_TEST(test_enclosing_env);
    RUN_TEST(test_hash_literals);
    RUN_TEST(test_hash_index_expressions);
    RUN_TEST(test_while_expressions);
    RUN_TEST(test_string_comparison);
    return UNITY_END();
}
