/**
 * Macro to shift a section of memory by an offset, used when inserting or removing items.
 */
#define arraylist_memshift(s, offset, length) memmove((s) + (offset), (s), (length) * sizeof(*(s)))

#include <arraylist.h>
#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include "../logging/log.h"

static arraylist *active_lists[1024];
static size_t     active_count = 0;

void track_arraylist(arraylist *list) {
    active_lists[active_count++] = list;
}

void untrack_arraylist(arraylist *list) {
    for (size_t i = 0; i < active_count; i++) {
        if (active_lists[i] == list) {
            active_lists[i] = active_lists[--active_count];
            return;
        }
    }
}

void log_active_arraylists(void) {
    log_debug("Active Arraylists (%zu):", active_count);
    for (size_t i = 0; i < active_count; i++) {
        log_debug("  %p", active_lists[i]);
    }
}

/**
 * Create a new, empty arraylist.
 */
arraylist *arraylist_create(size_t capacity, void (*free_func)(void *)) {
    arraylist *new_list = malloc(sizeof(arraylist));
    if (!new_list) {
        perror("Error: Failed to allocate memory for arraylist");
        exit(EXIT_FAILURE);
    }

    new_list->size      = 0;
    new_list->capacity  = capacity;
    new_list->free_func = free_func;

    new_list->body = malloc(sizeof(void *) * capacity);
    if (!new_list->body) {
        free(new_list);
        perror("Error: Failed to allocate memory for arraylist body");
        exit(EXIT_FAILURE);
    }
    //log_debug("Created Arraylist %p", new_list);
    track_arraylist(new_list);
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
            perror("Failed to allocate memory for arraylist");
            exit(EXIT_FAILURE);
        }

        l->body     = new_body;
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
    //log_debug("Arraylist %p, adding %p", l, item);
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
void arraylist_remove_and_free(arraylist *l, const unsigned int index) {
    // Check bounds
    if (index >= l->size) {
        fprintf(stderr, "Error: Index out of bounds\n");
        return;
    }

    // Store the value to return
    void *value = l->body[index];

    if (l->free_func != NULL) {
        l->free_func(value);
    }

    // Shift elements left to fill the gap
    if (index < l->size - 1) {
        arraylist_memshift(l->body + index + 1, -1, l->size - index - 1);
    }

    // Decrement size
    l->size--;
}


/**
 * Clear list of all items.
 */
void arraylist_clear(arraylist *l) {
    for (size_t i = 0; i < l->size; i++) {
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
    arraylist *new_list = arraylist_create(ARRAYLIST_INITIAL_CAPACITY, NULL);
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
arraylist *arraylist_clone(const arraylist *l, void *(*copy_func)(void *), void (*free_func)(void *)) {
    arraylist *new_list = arraylist_create(ARRAYLIST_INITIAL_CAPACITY, free_func);
    if (l->size == 0) {
        return new_list;
    }
    if (!copy_func) {
        err(EXIT_FAILURE, "Copy function must be provided.");
    }
    arraylist_allocate(new_list, l->size);
    for (unsigned int i = 0; i < l->size; i++) {
        void *item = copy_func(l->body[i]);
        arraylist_add(new_list, item);
    }
    new_list->size = l->size;
    return new_list;
}

char *arraylist_zip(const arraylist *l, const char *delim) {
    const size_t delim_len = strlen(delim);
    size_t       total_len = 0;
    char *       result    = nullptr;
    unsigned int i;

    if (l == NULL || l->size == 0) {
        return nullptr;
    }

    char *item;
    arraylist_iterate(l, i, item) {
        total_len += strlen(item) + delim_len;
    }

    // Allocate the result string
    result = malloc(total_len + 1);
    if (result == NULL) {
        return nullptr;
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

void var_destroy(destory_args args) {
    if (args.l == NULL || args.l->body == NULL) {
        return;
    }
    if (args.l->free_func != NULL) {
        for (unsigned i = 0; i < args.l->size; i++) {
            if (args.l->body[i] != NULL) {
                //log_debug("Arraylist %p calling free_func on %p", args.l, args.l->body[i]);
                args.l->free_func(args.l->body[i]);
            }
        }
    }
    untrack_arraylist(args.l);
    //log_debug("Freeing arraylist %p, size %u, free_func %p", args.l, args.l->size, args.l->free_func);
    free(args.l->body);
    free(args.l);
}
