//
// Created by dgood on 12/18/24.
//
#include <err.h>
#include <lexer.h>
#include <parser.h>
#include <string.h>

#include "../Unity/src/unity.h"
#include "../src/object/object.h"
#include "test_utils.h"
#include "../datastructures/linked_list.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

static void test_string_hash_key(void) {
    print_test_separator_line();
    printf("Testing hash key generation for string objects\n");

    object_string *hello  = object_create_string("hello world", 11);
    object_string *hello2 = object_create_string("hello world", 11);
    object_string *lorem  = object_create_string("Lorem ipsum dolor sit amet", 26);
    object_string *lorem2 = object_create_string("Lorem ipsum dolor sit amet", 26);

    size_t hello_hash  = hello->object.hash((object_string *) hello);
    size_t hello2_hash = hello2->object.hash((object_string *) hello2);
    size_t lorem_hash  = lorem->object.hash((object_string *) lorem);
    size_t lorem2_hash = lorem2->object.hash((object_string *) lorem2);

    TEST_ASSERT_EQUAL_size_t(hello_hash, hello2_hash);

    TEST_ASSERT_EQUAL_size_t(lorem_hash, lorem2_hash);

    TEST_ASSERT_NOT_EQUAL_size_t(hello_hash, lorem_hash);

    object_free(hello);
    object_free(hello2);
    object_free(lorem);
    object_free(lorem2);
}

// Test for object_create_int
void test_object_create_int_creates_integer_object(void) {
    long        value   = 42;
    object_int *int_obj = object_create_int(value);

    TEST_ASSERT_NOT_NULL(int_obj);
    TEST_ASSERT_EQUAL(OBJECT_INT, int_obj->object.type);
    TEST_ASSERT_EQUAL(value, int_obj->value);

    object_free(int_obj);
}

// Test for object_create_bool
void test_object_create_bool_returns_correct_boolean_objects(void) {
    object_bool *true_obj  = object_create_bool(true);
    object_bool *false_obj = object_create_bool(false);

    TEST_ASSERT_NOT_NULL(true_obj);
    TEST_ASSERT_NOT_NULL(false_obj);

    TEST_ASSERT_EQUAL(OBJECT_BOOL, true_obj->object.type);
    TEST_ASSERT_EQUAL(OBJECT_BOOL, false_obj->object.type);
    TEST_ASSERT_EQUAL(true, true_obj->value);
    TEST_ASSERT_EQUAL(false, false_obj->value);

    // No need to free TRUE_OBJ and FALSE_OBJ (global constants)
}

// Test for object_create_null
void test_object_create_null_returns_null_object(void) {
    const object_null *null_obj = object_create_null();

    TEST_ASSERT_NOT_NULL(null_obj);
    TEST_ASSERT_EQUAL(OBJECT_NULL, null_obj->object.type);
}

// Test for object_create_string
void test_object_create_string_creates_string_object(void) {
    const char *   value   = "Hello, World!";
    size_t         length  = strlen(value);
    object_string *str_obj = object_create_string(value, length);

    TEST_ASSERT_NOT_NULL(str_obj);
    TEST_ASSERT_EQUAL(OBJECT_STRING, str_obj->object.type);
    TEST_ASSERT_EQUAL_STRING(value, str_obj->value);
    TEST_ASSERT_EQUAL(length, str_obj->length);

    object_free(str_obj);
}

// Test for object_create_array
void test_object_create_array_creates_array_object(void) {
    arraylist * elements = arraylist_create(ARRAYLIST_INITIAL_CAPACITY, nullptr); // Mocked arraylist
    object_int *int_obj  = object_create_int(42);                                 // Mocked object
    arraylist_add(elements, int_obj);
    object_array *array_obj = object_create_array(elements);

    TEST_ASSERT_NOT_NULL(array_obj);
    TEST_ASSERT_EQUAL(OBJECT_ARRAY, array_obj->object.type);
    TEST_ASSERT_EQUAL(elements, array_obj->elements);

    // Free the array object and its elements
    object_free(array_obj);
}

// Test for object_create_hash
void test_object_create_hash_creates_hash_object(void) {
    hashtable *table = hashtable_create(string_hash_function, string_equals, object_free, object_free);
    // Mocked arraylist
    object_string *key = object_create_string("key", 3);
    hashtable_set(table, key, object_create_int(42));
    object_hash *hash_obj = object_create_hash(table);

    TEST_ASSERT_NOT_NULL(table);
    TEST_ASSERT_EQUAL(OBJECT_HASH, hash_obj->object.type);
    TEST_ASSERT_EQUAL(table, hash_obj->pairs);

    // Free the array object and its elements
    object_free(hash_obj);
}

void test_create_function_with_parameters() {
    const char *input = "fn(x, y) { x + y; }";
    print_test_separator_line();
    lexer *      lexer   = lexer_init(input);
    parser *     parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);

    // Mock environment
    environment *env = environment_create();

    ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
    ast_function_literal *    function = (ast_function_literal *) exp_stmt->expression;
    // Create function
    object_function *function_obj = object_create_function(function->parameters, function->body, env);

    // Assertions
    TEST_ASSERT_NOT_NULL(function);
    TEST_ASSERT_EQUAL(2, function->parameters->size);
    TEST_ASSERT_EQUAL_STRING("x", ((ast_identifier *)function->parameters->head->data)->value);
    TEST_ASSERT_EQUAL_STRING("y", ((ast_identifier *)function->parameters->head->next->data)->value);
    TEST_ASSERT_NOT_EQUAL(function->body, function_obj->body);
    TEST_ASSERT_NOT_EQUAL(function->parameters, function_obj->parameters);

    // Cleanup
    object_free(function_obj);
    environment_free(env);
    program_free(program);
    parser_free(parser);
}


/*void test_create_function_without_parameters() {
    ast_block_statement *body = create_mock_block_statement();
    environment *        env  = environment_create();

    object_function *function = object_create_function(nullptr, body, env);

    TEST_ASSERT_NOT_NULL(function);
    TEST_ASSERT_NULL(function->parameters);
    TEST_ASSERT_EQUAL_PTR(body, function->body);
    TEST_ASSERT_EQUAL_PTR(env, function->env);

    object_free((object_object *) function);
    environment_free(env);
}


void test_create_compiled_fn() {
    // Mock instructions
    instructions *mock_ins = create_mock_instructions(10, 20);

    // Create compiled function
    object_compiled_fn *compiled_fn = object_create_compiled_fn(mock_ins, 2, 3);

    // Assertions
    TEST_ASSERT_NOT_NULL(compiled_fn);
    TEST_ASSERT_EQUAL_UINT(10, compiled_fn->instructions->length);
    TEST_ASSERT_EQUAL_UINT(20, compiled_fn->instructions->capacity);
    TEST_ASSERT_EQUAL_UINT(2, compiled_fn->num_locals);
    TEST_ASSERT_EQUAL_UINT(3, compiled_fn->num_args);

    for (size_t i = 0; i < compiled_fn->instructions->length; i++) {
        TEST_ASSERT_EQUAL(mock_ins->bytes[i], compiled_fn->instructions->bytes[i]);
    }

    // Cleanup
    object_free((object_object *) compiled_fn);
}*/


void test_create_compiled_fn_invalid_instructions() {
    instructions invalid_ins = {.bytes = nullptr, .length = 0, .capacity = 0};

    TEST_ASSERT_NULL(object_create_compiled_fn(&invalid_ins, 2, 1));
}


// Test for object_create_error
void test_object_create_error_creates_error_object(void) {
    const char *  message   = "This is an error!";
    object_error *error_obj = object_create_error(message);

    TEST_ASSERT_NOT_NULL(error_obj);
    TEST_ASSERT_EQUAL(OBJECT_ERROR, error_obj->object.type);
    TEST_ASSERT_EQUAL_STRING(message, error_obj->message);

    object_free(error_obj);
}


// Test for object_equals
void test_object_equals_returns_true_for_identical_objects(void) {
    object_int *obj1 = object_create_int(42);
    object_int *obj2 = object_create_int(42);

    TEST_ASSERT_NOT_NULL(obj1);
    TEST_ASSERT_NOT_NULL(obj2);

    TEST_ASSERT_TRUE(object_equals(obj1, obj2));

    object_free(obj1);
    object_free(obj2);
}

// Test for inspect function
void test_inspect_returns_correct_string_representation(void) {
    object_int *int_obj = object_create_int(42);

    TEST_ASSERT_NOT_NULL(int_obj);
    char *result = int_obj->object.inspect((object_object *) int_obj);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("42", result);

    free(result); // Assuming inspect dynamically allocates memory
    object_free(int_obj);
}


int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_string_hash_key);
    RUN_TEST(test_object_create_int_creates_integer_object);
    RUN_TEST(test_object_create_bool_returns_correct_boolean_objects);
    RUN_TEST(test_object_create_null_returns_null_object);
    RUN_TEST(test_object_create_string_creates_string_object);
    RUN_TEST(test_object_create_array_creates_array_object);
    RUN_TEST(test_object_create_hash_creates_hash_object);
    RUN_TEST(test_object_create_error_creates_error_object);
    RUN_TEST(test_create_function_with_parameters);
    return UNITY_END();
}
