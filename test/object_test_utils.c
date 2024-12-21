//
// Created by dgood on 12/18/24.
//
#include "object_test_utils.h"
#include <string.h>
#include "../src/datastructures/conversions.h"
#include "../src/object/object.h"
#include "object_test_utils.h"
#include "test_utils.h"
#include "../Unity/src/unity.h"

void test_boolean_object(object_object *object, _Bool expected_value) {
    TEST_ASSERT_TRUE(object->type);
    object_bool *bool_obj = (object_bool *) object;
    TEST_ASSERT_EQUAL(bool_obj->value, expected_value);
}

void test_integer_object(object_object *object, long expected_value) {
    printf("Testing Integer Object: %ld\n", expected_value);
    TEST_ASSERT_EQUAL_INT(object->type, OBJECT_INT);
    
    object_int *int_obj = (object_int *) object;
    TEST_ASSERT_EQUAL_INT64(int_obj->value, expected_value);
}

void test_null_object(object_object *object) {
    TEST_ASSERT_EQUAL_INT(object->type, OBJECT_NULL);
}

void test_string_object(object_object *obj, char *expected_value, size_t expected_length) {
    printf("Testing Integer Object: %s\n", expected_value);
    object_string *str_obj = (object_string *) obj;
    TEST_ASSERT_EQUAL_INT(str_obj->length, expected_length);
    TEST_ASSERT_EQUAL_STRING_LEN(str_obj->value, expected_value, expected_length);
}

void test_object_object(object_object *obj, object_object *expected) {
    TEST_ASSERT_EQUAL_INT(obj->type, expected->type);
    if (expected->type == OBJECT_INT)
        test_integer_object(obj, ((object_int *) expected)->value);
    else if (expected->type == OBJECT_BOOL)
        test_boolean_object(obj, ((object_bool *) expected)->value);
    else if (expected->type == OBJECT_NULL)
        test_null_object(expected);
    else if (expected->type == OBJECT_STRING) {
        object_string *expected_str_obj = (object_string *) expected;
        test_string_object(obj, expected_str_obj->value, expected_str_obj->length);
    } else if (expected->type == OBJECT_ARRAY)
        test_array_object(obj, expected);
    else if (expected->type == OBJECT_HASH)
        test_hash_object(obj, expected);
    else if (expected->type == OBJECT_ERROR) {
        object_error *actual_err = (object_error *) obj;
        object_error *expected_err = (object_error *) expected;
        TEST_ASSERT_EQUAL_STRING(actual_err->message, expected_err->message);
    } else if (expected->type == OBJECT_COMPILED_FUNCTION) {
        instructions *expected_ins = ((object_compiled_fn *) expected)->instructions;
        instructions *actual_ins = ((object_compiled_fn *) obj)->instructions;
        char *expected_ins_string = instructions_to_string(expected_ins);
        char *actual_ins_string = instructions_to_string(actual_ins);
        TEST_ASSERT_EQUAL_size_t(expected_ins->length, actual_ins->length);
        TEST_ASSERT_EQUAL_MEMORY(expected_ins->bytes, actual_ins->bytes, expected_ins->length);
        free(expected_ins_string);
        free(actual_ins_string);
    }
}

void test_array_object(object_object *actual, object_object *expected) {
    object_array *actual_arr = (object_array *) actual;
    object_array *expected_arr = (object_array *) expected;
    TEST_ASSERT_EQUAL_UINT(actual_arr->elements->size, expected_arr->elements->size);
    for (size_t i = 0; i < actual_arr->elements->size; i++) {
        object_object *actual_obj = (object_object *) arraylist_get(actual_arr->elements, i);
        object_object *expected_obj = (object_object *) arraylist_get(expected_arr->elements, i);
        test_object_object(actual_obj, expected_obj);
    }
}

void test_hash_object(object_object *actual, object_object *expected) {
    object_hash *actual_hash = (object_hash *) actual;
    object_hash *expected_hash = (object_hash *) expected;
    TEST_ASSERT_EQUAL_size_t(actual_hash->pairs->key_count, expected_hash->pairs->key_count);
    arraylist *expected_keys = hashtable_get_keys(expected_hash->pairs);
    arraylist *actual_keys = hashtable_get_keys(actual_hash->pairs);
    if (expected_keys == NULL) {
        TEST_ASSERT_NULL(actual_keys);
    } else {
        for (size_t i = 0; i < expected_keys->size; i++) {
            object_object *key = arraylist_get(expected_keys, i);
            object_object *expected_value = (object_object *) hashtable_get(expected_hash->pairs, key);
            object_object *actual_value = (object_object *) hashtable_get(actual_hash->pairs, key);
            char *key_string = key->inspect(key);
            TEST_ASSERT_NOT_NULL(actual_value);
            test_object_object(actual_value, expected_value);
            free(key_string);
        }
    }
    arraylist_destroy(expected_keys);
    arraylist_destroy(actual_keys);
}
