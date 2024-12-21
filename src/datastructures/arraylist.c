/**
 * Arraylist implementation
 * (c) 2011 @marekweb
 *
 * Uses dynamic extensible arrays.
 */
#include "arraylist.h"
#include <assert.h>
#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
 * Interface section used for `makeheaders`.
 */
#if INTERFACE

struct arraylist {
	unsigned int size; // Count of items currently in list
	unsigned int capacity; // Allocated memory size, in items
	void** body; // Pointer to allocated memory for items (of size capacity * sizeof(void*))
};

/**
 * Iterates over a list, using the provided `unsigned int index` and `void* item`
 * variables as containers.
 */
#define arraylist_iterate(l, index, item) \
	for (index = 0, item = l->body[0]; index < l->size; item = l->body[++index])

#endif

/**
 * Macro to shift a section of memory by an offset, used when inserting or removing items.
 */
#define arraylist_memshift(s, offset, length) memmove((s) + (offset), (s), (length)* sizeof(s));

/**
 * Create a new, empty arraylist.
 */
arraylist *var_create(create_args args) {
    arraylist *new_list = malloc(sizeof(arraylist));
    if (new_list == NULL) {
        errx(EXIT_FAILURE, "Failed to create arraylist");
    }
    new_list->size      = 0;

    // Allocate the array
    new_list->body = malloc(sizeof(void *) * args.size);
    if (new_list->body == NULL) {
        free(new_list);
        errx(EXIT_FAILURE, "Failed to allocate memory for arraylist");
    }
    new_list->capacity = args.size;
    new_list->free_func = args.free_func;

    return new_list;
}

/**
 * Allocate sufficient array capacity for at least `size` elements.
 */
void arraylist_allocate(arraylist *l, const unsigned int size) {
    if (size == 0) {
        return; // No allocation needed for size 0
    }

    if (size > l->capacity) {
        unsigned int new_capacity = (l->capacity > 0) ? l->capacity : 1;
        while (new_capacity < size) {
            new_capacity *= 2; // Double capacity until it meets or exceeds size
        }

        void **new_body = realloc(l->body, sizeof(void *) * new_capacity);
        if (!new_body) {
            err(EXIT_FAILURE, "Failed to allocate memory for arraylist");
        }

        l->body = new_body;
        l->capacity = new_capacity;
    }
}


/**
 * Return the number of items contained in the list.
 */
extern inline unsigned int arraylist_size(const arraylist *l) {
    return l->size;
}

/**
 * Add item at the end of the list.
 */
void arraylist_add(arraylist *l, void *item) {
    arraylist_allocate(l, l->size + 1);
    // add new item to the end of the list
    l->body[l->size] = item;
    l->size++;
}

/**
 * Pop (remove and return) an item off the end of the list.
 */
void *arraylist_pop(arraylist *l) {
    if (l->size == 0) {
        fprintf(stderr, "Arraylist is empty: (size: %u)\n", l->size);
        return NULL; // Sentinel value
    }
    return l->body[--l->size];
}

/**
 * Return item located at index.
 */
void *arraylist_get(const arraylist *l, unsigned int index) {
    if (index >= l->size) {
        fprintf(stderr, "Index out of bounds: %u (size: %u)\n", index, l->size);
        return NULL; // Sentinel value
    }
    return l->body[index];
}


/**
 * Replace item at index with given value.
 */
void arraylist_set(arraylist *l, const unsigned int index, void *value) {
    assert(index < l->size);
    if (l->free_func != NULL) {
        l->free_func(l->body[index]);
    }
    l->body[index] = value;
}

/**
 * Insert item at index, shifting the following items by one spot.
 */
void arraylist_insert(arraylist *l, const unsigned int index, void *value) {
    // Reallocate, if needed
    arraylist_allocate(l, l->size + 1);

    // Move data to create a spot for the new value
    arraylist_memshift(l->body + index, 1, l->size - index);
    l->body[index] = value;
    l->size++;
}

/**
 * Remove the item at index, shifting the following items back by one spot.
 */
void *arraylist_remove(arraylist *l, const unsigned int index) {
    void *value = l->body[index];
    arraylist_memshift(l->body + index + 1, -1, l->size - index);
    l->size--;
    return value;
}

/**
 * Clear list of all items.
 */
void arraylist_clear(arraylist *l) {
    for (size_t i=0; i < l->size; i++) {
        if (l->free_func != NULL) {
            l->free_func(l->body[i]);
        }
    }
    l->size = 0;
}

/**
 * Return a slice of the list (of given length starting at index) as a new arraylist.
 */
arraylist *arraylist_slice(const arraylist *  l, const unsigned int index,
                           const unsigned int length) {
    assert(index + length <= l->size);
    arraylist *new_list = arraylist_create(ARRAYLIST_INITIAL_CAPACITY);
    arraylist_allocate(new_list, length);
    memmove(new_list->body, l->body + index, length * sizeof(void *));
    new_list->size = length;
    return new_list;
}

/**
 * Return a slice of the list (from index to the end) as a new arraylist.
 */
arraylist *arraylist_slice_end(const arraylist *l, const unsigned int index) {
    return arraylist_slice(l, index, l->size - index);
}

/**
 *  Clone the arraylist.
 */
arraylist *var_clone(const clone_args args) {
    arraylist *new_list = arraylist_create(ARRAYLIST_INITIAL_CAPACITY, args.l->free_func);
    if (args.l->size == 0) {
        return new_list;
    }
    arraylist_allocate(new_list, args.l->size);
    for (unsigned int i = 0; i < args.l->size; i++) {
        if (args.copy_func != NULL) {
            void *item = args.copy_func(args.l->body[i]);
            arraylist_add(new_list, item);
        }
    }
    new_list->size = args.l->size;
    return new_list;
}

char *arraylist_zip(const arraylist *l, const char *delim) {
    const size_t delim_len = strlen(delim);
    size_t       total_len = 0;
    char *       result    = NULL;
    unsigned int i;

    if (l == NULL || l->size == 0) {
        return NULL;
    }

    char *item;
    arraylist_iterate(l, i, item) {
        total_len += strlen(item) + delim_len;
    }

    // Allocate the result string
    result = malloc(total_len + 1);
    if (result == NULL) {
        return NULL;
    }

    // Copy the strings into the result
    char *p = result;
    arraylist_iterate(l, i, item) {
        const size_t len = strlen(item);
        memcpy(p, item, len);
        p += len;
        if (delim != NULL) {
            memcpy(p, delim, delim_len);
            p += delim_len;
        }
    }
    // Replace the last delimiter with a null terminator
    p -= delim_len;
    *p = '\0';

    return result;
}

char *arraylist_to_string(const arraylist *l) {
    return arraylist_zip(l, NULL);
}

void *arraylist_to_array(const arraylist *l) {
    void *array = malloc(l->size * sizeof(void *));
    if (array == NULL) {
        return NULL;
    }
    memcpy(array, l->body, l->size * sizeof(void *));
    return array;
}

/**
 * Append a list onto another, in-place.
 */
void arraylist_join(arraylist *l, const arraylist *source) {
    arraylist_splice(l, source, l->size);
}

/**
 * Insert a list into another at the given index, in-place.
 */
void arraylist_splice(arraylist *        l, const arraylist *source,
                      const unsigned int index) {
    // Reallocate, if needed
    arraylist_allocate(l, l->size + source->size);

    // Move data to the right
    arraylist_memshift(l->body + index, source->size, l->size - index);

    // Copy the data over
    memmove(l->body + index, source->body, source->size * sizeof(void *));
    l->size += source->size;
}

void arraylist_sort(const arraylist *l, int (*cmp_func)(const void *, const void *)) {
    if (!l || l->size == 1) {
        return;
    }
    for (size_t i = 0; i < l->size; i++) {
        if (!l->body[i]) {
            fprintf(stderr, "Null element in arraylist at index %zu\n", i);
            return;
        }
    }
    qsort(l->body, l->size, sizeof(void *), cmp_func);
}

void var_destroy(const destory_args args) {
    if (args.l == NULL) {
        return;
    }
    if (args.l->free_func != NULL) {
        for (unsigned i = 0; i < args.l->size; i++) {
            if (args.l->body[i] != NULL) {
                args.l->free_func(args.l->body[i]);
            }
        }
    }
    free(args.l->body);
    free(args.l);
}
