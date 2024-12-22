//
// Created by dgood on 12/3/24.
//

#ifndef ARRAYLIST_H
#define ARRAYLIST_H

#pragma once

#include <stdlib.h>

#define ARRAYLIST_INITIAL_CAPACITY 16


typedef struct {
    unsigned int size;     // Count of items currently in list
    unsigned int capacity; // Allocated memory size, in items
    void **      body;

    void (*free_func)(void *); // Function to free memory of items
    // Pointer to allocated memory for items (of size capacity * sizeof(void*))
} arraylist;

/**
 * Safe Variadic function to destroy an arraylist.
 */
typedef struct {
    arraylist *l;

    void (*free_func)(void *);
} destory_args;

void destroy_base(arraylist *l, ...);

void var_destroy(const destory_args args);

#define arraylist_destroy(...) var_destroy((destory_args){__VA_ARGS__});
/******************************************************************************/

/**
 * Safe Variadic function to copy an arraylist.
 */
typedef struct {
    arraylist *l;

    void * (*copy_func)(void *);
} clone_args;

void clone_base(arraylist *l, ...);

arraylist *var_clone(clone_args args);

#define arraylist_clone(...) var_clone((clone_args){__VA_ARGS__});
/******************************************************************************/


arraylist *arraylist_create(size_t capacity, void (*free_func)(void *));

void arraylist_sort(const arraylist *l, int (*cmp_func)(const void *, const void *));

void arraylist_splice(arraylist *  l, const arraylist *source,
                      unsigned int index);

void arraylist_join(arraylist *l, const arraylist *source);

arraylist *arraylist_slice_end(const arraylist *l, unsigned int index);

arraylist *arraylist_slice(const arraylist *l, unsigned int index,
                           unsigned int     length);

void arraylist_clear(arraylist *l);

void *arraylist_remove(arraylist *l, unsigned int index);

void arraylist_insert(arraylist *l, unsigned int index, void *value);

void arraylist_set(arraylist *l, unsigned int index, void *value);

void *arraylist_get(const arraylist *l, unsigned int index);

void *arraylist_pop(arraylist *l);

void arraylist_add(arraylist *l, void *item);

char *arraylist_zip(const arraylist *l, const char *delim);

char *arraylist_to_string(const arraylist *l);

void *arraylist_to_array(const arraylist *l);

void arraylist_allocate(arraylist *l, unsigned int size);

#define arraylist_iterate(l, index, item) \
for (index = 0, item = l->body[0]; index < l->size; item = l->body[++index])

#define INTERFACE 0

#endif //ARRAYLIST_H
