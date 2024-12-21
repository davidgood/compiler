//
// Created by dgood on 12/2/24.
//

#ifndef HASHMAP_H
#define HASHMAP_H

#pragma once

#include "arraylist.h"
#include "linked_list.h"
#include <stdbool.h>

#define HASHTABLE_INITIAL_CAPACITY 64

typedef struct {
    linked_list **table;
    arraylist *            used_slots;
    size_t                 table_size;
    size_t                 key_count; // actual number of keys stored
    size_t (*              hash_func)(void *);

    bool (*key_equals)(void *, void *);

    void (*free_key)(void *);

    void (*free_value)(void *);
} hashtable;

typedef struct {
    void *key;
    void *value;
    void (*free_key)(void *);

    void (*free_value)(void *);
} hashtable_entry;

void hashtable_destroy(hashtable *t);

hashtable_entry *hashtable_body_allocate(unsigned int capacity);

hashtable *hashtable_create(size_t (*hash_func)(void *),
                            bool (*  key_equals)(void *, void *),
                            void (*  free_key)(void *),
                            void (*  free_value)(void *));

void hashtable_remove(hashtable *, void *);

void hashtable_resize(hashtable *, unsigned int);

void hashtable_set(hashtable *, void *, void *);

void *hashtable_get(const hashtable *, void *);

unsigned int hashtable_find_slot(hashtable *, void *);

unsigned long hashtable_hash(char *);

arraylist *hashtable_get_keys(const hashtable *);

arraylist *hashtable_get_values(hashtable *);

hashtable *hashtable_clone(const hashtable *src, void * (*key_copy)(void *), void * (*value_copy)(void *));

size_t string_hash_function(void *key);

bool string_equals(void *, void *);

size_t int_hash_function(void *);

bool int_equals(void *, void *);

size_t pointer_hash_function(void *);

bool pointer_equals(void *, void *);

void hashtable_visualize(const hashtable *);

#define INTERFACE 0

#endif //HASHMAP_H
