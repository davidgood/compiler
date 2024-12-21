//
// Created by dgood on 12/18/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../Unity/src/unity.h"
#include "../src/datastructures/arraylist.h"

void setUp(void) {
    // Set up code if needed
}

void tearDown(void) {
    // Tear down code if needed
}

void test_arraylist_create(void) {
    arraylist *list = arraylist_create(ARRAYLIST_INITIAL_CAPACITY, free);
    TEST_ASSERT_NOT_NULL(list);
    TEST_ASSERT_EQUAL_UINT(0, list->size);
    TEST_ASSERT_EQUAL_UINT(ARRAYLIST_INITIAL_CAPACITY, list->capacity);
    arraylist_destroy(list);
}

void test_arraylist_add(void) {
    arraylist *list = arraylist_create(ARRAYLIST_INITIAL_CAPACITY, free);
    int *value = malloc(sizeof(int));
    *value = 42;
    arraylist_add(list, value);
    TEST_ASSERT_EQUAL_UINT(1, list->size);
    TEST_ASSERT_EQUAL_INT(42, *(int *)arraylist_get(list, 0));
    arraylist_destroy(list);
}

void test_arraylist_get(void) {
    arraylist *list = arraylist_create(ARRAYLIST_INITIAL_CAPACITY, free);
    int *value = malloc(sizeof(int));
    *value = 42;
    arraylist_add(list, value);
    int *retrieved_value = (int *)arraylist_get(list, 0);
    TEST_ASSERT_NOT_NULL(retrieved_value);
    TEST_ASSERT_EQUAL_INT(42, *retrieved_value);
    arraylist_destroy(list);
}

void test_arraylist_set(void) {
    arraylist *list = arraylist_create(ARRAYLIST_INITIAL_CAPACITY, free);
    int *value1 = malloc(sizeof(int));
    *value1 = 42;
    arraylist_add(list, value1);
    int *value2 = malloc(sizeof(int));
    *value2 = 84;
    arraylist_set(list, 0, value2);
    TEST_ASSERT_EQUAL_INT(84, *(int *)arraylist_get(list, 0));
    arraylist_destroy(list);
}

void test_arraylist_remove(void) {
    arraylist *list = arraylist_create(ARRAYLIST_INITIAL_CAPACITY, free);
    int *value = malloc(sizeof(int));
    *value = 42;
    arraylist_add(list, value);
    int *removed_value = (int *)arraylist_remove(list, 0);
    TEST_ASSERT_EQUAL_INT(42, *removed_value);
    free(removed_value);
    TEST_ASSERT_EQUAL_UINT(0, list->size);
    arraylist_destroy(list);
}

void test_arraylist_clear(void) {
    arraylist *list = arraylist_create(ARRAYLIST_INITIAL_CAPACITY, free);
    for (int i = 0; i < 10; i++) {
        int *value = malloc(sizeof(int));
        *value = i;
        arraylist_add(list, value);
    }
    arraylist_clear(list);
    TEST_ASSERT_EQUAL_UINT(0, list->size);
    arraylist_destroy(list);
}

static int compare_integers(const void *v1, const void *v2) {
    const int i1 = **(int **) v1;
    const int i2 = **(int **)v2;
    return i1 - i2;
}

void test_arraylist_sort(void) {
    arraylist *list = arraylist_create(10, free);

    // Add unsorted integers to the arraylist
    int *a = malloc(sizeof(int));
    int *b = malloc(sizeof(int));
    int *c = malloc(sizeof(int));
    *a = 3;
    *b = 1;
    *c = 2;

    arraylist_add(list, a);
    arraylist_add(list, b);
    arraylist_add(list, c);

    // Sort using a comparison function
    arraylist_sort(list, compare_integers);

    // Verify sorted order
    TEST_ASSERT_EQUAL_INT(1, *(int *)list->body[0]);
    TEST_ASSERT_EQUAL_INT(2, *(int *)list->body[1]);
    TEST_ASSERT_EQUAL_INT(3, *(int *)list->body[2]);

    // Clean up
    arraylist_destroy(list);
}


int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_arraylist_create);
    RUN_TEST(test_arraylist_add);
    RUN_TEST(test_arraylist_get);
    RUN_TEST(test_arraylist_set);
    RUN_TEST(test_arraylist_remove);
    RUN_TEST(test_arraylist_clear);
    RUN_TEST(test_arraylist_sort);
    return UNITY_END();
}