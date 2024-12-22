//
// Created by dgood on 12/18/24.
//
#include <err.h>
#include <string.h>

#include "../Unity/src/unity.h"
#include "../src/object/object.h"
#include "test_utils.h"

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
    arraylist * elements = arraylist_create(ARRAYLIST_INITIAL_CAPACITY, NULL); // Mocked arraylist
    object_int *int_obj  = object_create_int(42);                              // Mocked object
    arraylist_add(elements, int_obj);
    object_array *array_obj = object_create_array(elements);

    TEST_ASSERT_NOT_NULL(array_obj);
    TEST_ASSERT_EQUAL(OBJECT_ARRAY, array_obj->object.type);
    TEST_ASSERT_EQUAL(elements, array_obj->elements);

    // Free the array object and its elements
    object_free(array_obj);
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

// Test for object_create_function
// void test_object_create_function_creates_function_object(void) {
//     ast_block_statement *block_stmt = malloc(sizeof(*block_stmt));
//     if (block_stmt == NULL) {
//         err(EXIT_FAILURE, "malloc failed");
//     }
//     block_stmt->statement.node.string        = block_statement_string;
//     block_stmt->statement.node.token_literal = block_statement_token_literal;
//     block_stmt->statement.node.type          = STATEMENT;
//     block_stmt->statement.statement_type     = BLOCK_STATEMENT;
//     block_stmt->array_size                   = 8;
//     block_stmt->statements                   = calloc(block_stmt->array_size, sizeof(*block_stmt->statements));
//     if (block_stmt->statements == NULL) {
//         free(block_stmt);
//         err(EXIT_FAILURE, "malloc failed");
//     }
//     block_stmt->statement_count = 0;
//     block_stmt->token           = token_copy(parser->cur_tok);
//     return block_stmt;
//
//     linked_list *        parameters = linked_list_create();     // Mocked linked list
//     ast_block_statement *body       = ();                       // Mocked AST node
//     environment *        env        = Mockenvironment_Create(); // Mocked environment
//
//     object_function *func_obj = object_create_function(parameters, body, env);
//
//     TEST_ASSERT_NOT_NULL(func_obj);
//     TEST_ASSERT_EQUAL(OBJECT_FUNCTION, func_obj->object.type);
//     TEST_ASSERT_EQUAL(parameters, func_obj->parameters);
//     TEST_ASSERT_EQUAL(body, func_obj->body);
//     TEST_ASSERT_EQUAL(env, func_obj->env);
//
//     object_free(func_obj);
// }

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
    RUN_TEST(test_object_create_error_creates_error_object);
    return UNITY_END();
}
