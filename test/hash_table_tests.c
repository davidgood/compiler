//
// Created by dgood on 12/18/24.
//
#include <stdlib.h>
#include <string.h>
#include "../Unity/src/unity.h"
#include "../src/datastructures/hashmap.h"

// Helper functions for testing
static void free_string(void *data) { free((char *) data); }

static void free_noop(void *data) {
    // No operation
}

static void *copy_string(void *data) { return strdup((char *) data); }

static void *copy_int(void *data) {
    int *copy = malloc(sizeof(int));
    *copy     = *(int *) data;
    return copy;
}

void setUp(void) {
    // Optional setup before each test
}

void tearDown(void) {
    // Optional cleanup after each test
}

// Test cases

void test_hashtable_create_destroy(void) {
    hashtable *table = hashtable_create(string_hash_function, string_equals, free_string, free_string);
    TEST_ASSERT_NOT_NULL(table);
    TEST_ASSERT_EQUAL(0, table->key_count);
    TEST_ASSERT_EQUAL(HASHTABLE_INITIAL_CAPACITY, table->table_size);
    hashtable_destroy(table);
}

void test_hashtable_set_get(void) {
    hashtable *table = hashtable_create(string_hash_function, string_equals, free_string, free_string);

    char *key1   = strdup("key1");
    char *value1 = strdup("value1");

    hashtable_set(table, key1, value1);

    char *retrieved_value = (char *) hashtable_get(table, "key1");
    TEST_ASSERT_NOT_NULL(retrieved_value);
    TEST_ASSERT_EQUAL_STRING("value1", retrieved_value);

    hashtable_destroy(table);
}

void test_hashtable_remove(void) {
    hashtable *table = hashtable_create(string_hash_function, string_equals, free_string, free_string);

    char *key1   = strdup("key1");
    char *value1 = strdup("value1");

    hashtable_set(table, key1, value1);
    hashtable_remove(table, "key1");

    char *retrieved_value = (char *) hashtable_get(table, "key1");
    TEST_ASSERT_NULL(retrieved_value);

    hashtable_destroy(table);
}

void test_hashtable_multiple_entries(void) {
    hashtable *table = hashtable_create(string_hash_function, string_equals, free_string, free_string);

    char *key1   = strdup("key1");
    char *value1 = strdup("value1");
    char *key2   = strdup("key2");
    char *value2 = strdup("value2");

    hashtable_set(table, key1, value1);
    hashtable_set(table, key2, value2);

    TEST_ASSERT_EQUAL(2, table->key_count);

    char *retrieved_value1 = (char *) hashtable_get(table, "key1");
    char *retrieved_value2 = (char *) hashtable_get(table, "key2");

    TEST_ASSERT_NOT_NULL(retrieved_value1);
    TEST_ASSERT_NOT_NULL(retrieved_value2);
    TEST_ASSERT_EQUAL_STRING("value1", retrieved_value1);
    TEST_ASSERT_EQUAL_STRING("value2", retrieved_value2);

    hashtable_destroy(table);
}

void test_hashtable_clone(void) {
    hashtable *table = hashtable_create(string_hash_function, string_equals, free_string, free_string);

    char *key1   = strdup("key1");
    char *value1 = strdup("value1");
    hashtable_set(table, key1, value1);

    hashtable *clone = hashtable_clone(table, copy_string, copy_string);

    TEST_ASSERT_EQUAL(1, clone->key_count);
    char *retrieved_value = (char *) hashtable_get(clone, "key1");
    TEST_ASSERT_NOT_NULL(retrieved_value);
    TEST_ASSERT_EQUAL_STRING("value1", retrieved_value);

    hashtable_destroy(clone);
    hashtable_destroy(table);
}

void test_hashtable_collision_handling(void) {
    hashtable *table = hashtable_create(int_hash_function, int_equals, free_noop, free_noop);

    size_t key1 = 1, key2 = HASHTABLE_INITIAL_CAPACITY + 1;
    size_t value1 = 42, value2 = 84;

    hashtable_set(table, &key1, &value1);
    hashtable_set(table, &key2, &value2);

    TEST_ASSERT_EQUAL(2, table->key_count);

    int *retrieved_value1 = (int *) hashtable_get(table, &key1);
    int *retrieved_value2 = (int *) hashtable_get(table, &key2);

    TEST_ASSERT_NOT_NULL(retrieved_value1);
    TEST_ASSERT_NOT_NULL(retrieved_value2);
    TEST_ASSERT_EQUAL(42, *retrieved_value1);
    TEST_ASSERT_EQUAL(84, *retrieved_value2);

    hashtable_destroy(table);
}

void test_hashtable_get_keys_values(void) {
    hashtable *table = hashtable_create(string_hash_function, string_equals, free_string, free_string);

    char *key1   = strdup("key1");
    char *value1 = strdup("value1");
    char *key2   = strdup("key2");
    char *value2 = strdup("value2");

    hashtable_set(table, key1, value1);
    hashtable_set(table, key2, value2);

    arraylist *keys   = hashtable_get_keys(table);
    arraylist *values = hashtable_get_values(table);

    TEST_ASSERT_EQUAL(2, keys->size);
    TEST_ASSERT_EQUAL(2, values->size);

    hashtable_destroy(table);
    arraylist_destroy(keys);
    arraylist_destroy(values);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_hashtable_create_destroy);
    RUN_TEST(test_hashtable_set_get);
    RUN_TEST(test_hashtable_remove);
    RUN_TEST(test_hashtable_multiple_entries);
    RUN_TEST(test_hashtable_clone);
    RUN_TEST(test_hashtable_collision_handling);
    RUN_TEST(test_hashtable_get_keys_values);

    return UNITY_END();
}
