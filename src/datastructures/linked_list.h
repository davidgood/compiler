//
// Created by dgood on 12/6/24.
//

#ifndef LINKED_LIST_H
#define LINKED_LIST_H
#include <stddef.h>
#include <stdbool.h>

// Node structure
typedef struct list_node {
    void *            data; // Pointer to data
    struct list_node *next; // Pointer to the next node
} list_node;

// Circular Linked List structure
typedef struct {
    list_node *head; // Pointer to the tail node
    list_node *tail; // Pointer to the tail node
    size_t     size; // Number of elements in the list
} linked_list;

linked_list *linked_list_create();

list_node *linked_list_createNode(void *);

void linked_list_addNode(linked_list *, void *);

list_node *linked_list_get_at(const linked_list *, size_t);

void *linked_list_get(linked_list *, void *, bool (*)(void *, void *));

void linked_list_displayList(linked_list *list, void (*printFunc)(void *));

bool linked_list_delete_node(linked_list *list, const list_node *nodeToDelete);

bool linked_list_delete_at(linked_list *list, const size_t index, void (*free_data)(void *));

void linked_list_free(linked_list *list, void (*free_data)(void *));

#endif //LINKED_LIST_H
