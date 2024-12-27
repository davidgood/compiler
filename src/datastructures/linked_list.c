//
// Created by dgood on 12/6/24.
//

#include "linked_list.h"
#include <stdio.h>
#include <stdlib.h>
#include "../logging/log.h"

#define LOG_ALLOC(ptr, type) printf("[ALLOC] %s: %p\n", type, ptr)
#define LOG_FREE(ptr, type) printf("[FREE] %s: %p\n", type, ptr)


// Function to create a new list
linked_list *linked_list_create(void (*free_func)(void *)) {
    linked_list *list = malloc(sizeof(linked_list));
    if (!list) {
        perror("Failed to allocate memory for linked list");
        return nullptr;
    }
    list->head      = nullptr;
    list->tail      = nullptr;
    list->size      = 0;
    list->free_func = free_func;
    return list;
}

// Function to create a new node
list_node *linked_list_createNode(void *data) {
    list_node *node = malloc(sizeof(list_node));
    if (!node) {
        perror("Failed to allocate memory for list node");
        return nullptr;
    }
    node->data = data;
    node->next = nullptr;
    return node;
}

// Function to add an element to the list
void linked_list_addNode(linked_list *list, void *data) {
    list_node *node = malloc(sizeof(*node));
    if (node == NULL)
        return;
    node->data = data;
    node->next = nullptr;
    list->size++;
    if (list->head == NULL) {
        list->head = node;
        list->tail = node;
        return;
    }
    list->tail->next = node;
    list->tail       = node;
}

list_node *linked_list_get_at(const linked_list *list, size_t index) {
    if (list->head == NULL || index >= list->size) {
        fprintf(stderr, "Invalid index or NULL list\n");
        return nullptr;
    }
    list_node *current = list->head;
    for (size_t i = 0; i < index; i++) {
        current = current->next;
    }
    return current;
}

void *linked_list_get(linked_list *list, void *key, bool (*compareFunc)(void *, void *)) {
    list_node *node = list->head;
    while (node) {
        if (compareFunc(node->data, key)) {
            return node->data;
        }
        node = node->next;
    }
    return NULL;
}

// Function to display the list (example for integer data)
void linked_list_displayList(linked_list *list, void (*printFunc)(void *)) {
    if (list->tail == NULL) {
        printf("List is empty.\n");
        return;
    }
    const list_node *current = list->tail->next; // Start from the head
    do {
        printFunc(current->data);
        current = current->next;
    } while (current != list->tail->next); // Loop until we return to the head
    printf("\n");
}


// Function to delete a node
bool linked_list_delete_node(linked_list *list, const list_node *nodeToDelete) {
    if (!list || !nodeToDelete) {
        fprintf(stderr, "Invalid list or node\n");
        return false;
    }

    list_node *current = list->head;
    list_node *prev    = list->tail;
    for (size_t i = 0; i < list->size; i++) {
        if (current == nodeToDelete) {
            if (list->size == 1) {
                list->head = nullptr;
                list->tail = nullptr;
            } else {
                prev->next = current->next;
                if (current == list->head)
                    list->head = current->next;
                if (current == list->tail)
                    list->tail = prev;
            }
            free(current);
            list->size--;
            return true;
        }
        prev    = current;
        current = current->next;
    }
    return false;
}

// Function to delete a node at a given index
bool linked_list_delete_at(linked_list *list, const size_t index, void (*free_data)(void *)) {
    if (!list || index >= list->size) {
        fprintf(stderr, "Invalid index or NULL list\n");
        return false;
    }

    list_node *current = list->head;
    list_node *prev    = list->tail;

    for (size_t i = 0; i < index; i++) {
        prev    = current;
        current = current->next;
    }

    if (list->size == 1) {
        list->head = nullptr;
        list->tail = nullptr;
    } else {
        prev->next = current->next;
        if (current == list->head)
            list->head = current->next;
        if (current == list->tail)
            list->tail = prev;
    }

    if (free_data)
        free_data(current->data);

    free(current);
    list->size--;
    return true;
}


// Function to free the linked list
void linked_list_free(linked_list *list, void (*free_data)(void *)) {
    if (!list) {
        fprintf(stderr, "List is NULL\n");
        return;
    }

    list_node *current = list->head;
    for (size_t i = 0; i < list->size; i++) {
        list_node *next = current->next;
        if (list->free_func) {
            list->free_func(current->data);
        } else if (free_data) {
            free_data(current->data);
        }
        free(current);
        current = next;
    }

    free(list);
}

