//
// Created by dgood on 12/18/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../Unity/src/unity.h"
#include "../src/datastructures/linked_list.h"

void setUp(void) {
    // Set up code if needed
}

void tearDown(void) {
    // Tear down code if needed
}

void test_linked_list_create(void) {
    linked_list *list = linked_list_create();
    TEST_ASSERT_NOT_NULL(list);
    TEST_ASSERT_NULL(list->head);
    TEST_ASSERT_NULL(list->tail);
    TEST_ASSERT_EQUAL_UINT(0, list->size);
    linked_list_free(list, free);
}

void test_linked_list_addNode(void) {
    linked_list *list = linked_list_create();
    int *value = malloc(sizeof(int));
    *value = 42;
    linked_list_addNode(list, value);
    TEST_ASSERT_EQUAL_UINT(1, list->size);
    TEST_ASSERT_EQUAL_INT(42, *(int *)list->head->data);
    linked_list_free(list, free);
}

void test_linked_list_getNode(void) {
    linked_list *list = linked_list_create();
    int *value = malloc(sizeof(int));
    *value = 42;
    linked_list_addNode(list, value);
    list_node *node = linked_list_get_at(list, 0);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(42, *(int *)node->data);
    linked_list_free(list, free);
}

void test_linked_list_removeNode(void) {
    linked_list *list = linked_list_create();
    int *value = malloc(sizeof(int));
    *value = 42;
    linked_list_addNode(list, value);
    bool deleted = linked_list_delete_at(list, 0, free);
    TEST_ASSERT(deleted);
    TEST_ASSERT_EQUAL_UINT(0, list->size);
    linked_list_free(list, free);
}
/*
void test_linked_list_clear(void) {
    linked_list *list = linked_list_create();
    for (int i = 0; i < 10; i++) {
        int *value = malloc(sizeof(int));
        *value = i;
        linked_list_addNode(list, value);
    }
    linked_list_clear(list, free);
    TEST_ASSERT_EQUAL_UINT(0, list->size);
    linked_list_free(list, free);
}
*/
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_linked_list_create);
    RUN_TEST(test_linked_list_addNode);
    RUN_TEST(test_linked_list_getNode);
    RUN_TEST(test_linked_list_removeNode);
    //RUN_TEST(test_linked_list_clear);
    return UNITY_END();
}