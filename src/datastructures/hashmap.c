#include "hashmap.h"
#include "arraylist.h"
#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


/**
 * Return the item associated with the given key, or NULL if not found.
 */
void *hashtable_get(const hashtable *table, void *key) {
    if (table->hash_func == NULL || table->key_equals == NULL) {
        err(EXIT_FAILURE, "hash_func and key_equals must be set. key=%s\n", (char *) key);
    }
    const size_t       index      = table->hash_func(key) % table->table_size;
    const linked_list *entry_list = table->table[index];
    if (entry_list == NULL) {
        return NULL;
    }
    const list_node *list_node = entry_list->head;
    while (list_node) {
        const hashtable_entry *entry = (hashtable_entry *) list_node->data;
        if (table->key_equals(entry->key, key))
            return entry->value;
        list_node = list_node->next;
    }
    return NULL;
}

static hashtable_entry *find_entry(const linked_list *list, const hashtable_entry *other_entry,
                                   _Bool (*           key_equals)(void *, void *)) {
    const list_node *node = list->head;
    while (node) {
        hashtable_entry *entry = (node->data);
        if (key_equals(entry->key, other_entry->key))
            return entry;
        node = node->next;
    }
    return NULL;
}

/**
 * Assign a value to the given key in the table.
 */
void hashtable_set(hashtable *hash_table, void *key, void *value) {
    const size_t index = hash_table->hash_func(key) % hash_table->table_size;

    linked_list *    entry_list = hash_table->table[index];
    hashtable_entry *entry      = NULL;

    if (entry_list == NULL) {
        entry_list               = linked_list_create();
        hash_table->table[index] = entry_list;

        size_t *used_slot = malloc(sizeof(*used_slot));
        if (used_slot == NULL) {
            err(EXIT_FAILURE, "malloc failed");
        }
        *used_slot = index;
        arraylist_add(hash_table->used_slots, used_slot);
    } else {
        const hashtable_entry temp_entry = {key, NULL, NULL, NULL};
        entry                            = find_entry(entry_list, &temp_entry, hash_table->key_equals);
    }

    if (entry == NULL) {
        entry = malloc(sizeof(*entry));
        if (entry == NULL) {
            err(EXIT_FAILURE, "malloc failed");
        }
        entry->key        = key;
        entry->value      = value;
        entry->free_key   = hash_table->free_key; // Assign free functions
        entry->free_value = hash_table->free_value;

        hash_table->key_count++;
        linked_list_addNode(entry_list, entry);
    } else {
        if (entry->free_key) {
            entry->free_key(entry->key);
        }
        if (entry->free_value) {
            entry->free_value(entry->value);
        }
        entry->key   = key;
        entry->value = value;
    }
}


arraylist *hashtable_get_keys(const hashtable *hash_table) {
    if (hash_table->key_count == 0) {
        return NULL;
    }
    arraylist *keys_list = arraylist_create(hash_table->key_count, NULL);
    for (size_t i = 0; i < hash_table->table_size; i++) {
        linked_list *bucket = hash_table->table[i];
        if (bucket == NULL) {
            continue;
        }
        list_node *node = bucket->head;
        while (node != NULL) {
            const hashtable_entry *entry = (hashtable_entry *) node->data;
            node                         = node->next;
            arraylist_add(keys_list, entry->key);
        }
    }
    return keys_list;
}

arraylist *hashtable_get_values(hashtable *hash_table) {
    arraylist *values_list = arraylist_create(hash_table->key_count, NULL);
    for (size_t i = 0; i < hash_table->table_size; i++) {
        linked_list *bucket = hash_table->table[i];
        if (bucket == NULL) {
            continue;
        }
        const list_node *node = bucket->head;
        while (node != NULL) {
            const hashtable_entry *entry = (hashtable_entry *) node->data;
            node                         = node->next;
            arraylist_add(values_list, entry->value);
        }
    }
    return values_list;
}

/**
 * Remove a key from the table
 */
void hashtable_remove(hashtable *t, void *key) {
    const size_t index      = t->hash_func(key) % t->table_size;
    linked_list *entry_list = t->table[index];

    if (entry_list == NULL) {
        return; // Key doesn't exist
    }

    const hashtable_entry temp_entry = {key, NULL};
    list_node *           prev       = NULL;
    list_node *           current    = entry_list->head;

    while (current != NULL) {
        hashtable_entry *entry = (hashtable_entry *) current->data;
        if (t->key_equals(entry->key, key)) {
            // Free the entry
            if (t->free_key) {
                t->free_key(entry->key);
            }
            if (t->free_value) {
                t->free_value(entry->value);
            }
            free(entry);

            // Remove the node from the list
            if (prev == NULL) {
                entry_list->head = current->next; // Remove head node
            } else {
                prev->next = current->next; // Bypass the current node
            }

            if (current == entry_list->tail) {
                entry_list->tail = prev; // Update tail if needed
            }

            arraylist *used_slots = t->used_slots;
            for (size_t i = 0; i < used_slots->size; i++) {
                size_t *slot = used_slots->body[i];
                if (*slot == index) {
                    arraylist_remove(used_slots, i);
                    free(slot);
                    break;
                }
            }

            list_node *to_free = current;       // Save current for freeing
            current            = current->next; // Move to the next node
            free(to_free);                      // Free the current node
            t->key_count--;
            t->table[index]->size--;
            return;
        }
        prev    = current;
        current = current->next;
    }
}


hashtable *hashtable_clone(const hashtable *src, void * (*key_copy)(void *), void * (*value_copy)(void *)) {
    hashtable *copy = hashtable_create(src->hash_func,
                                       src->key_equals, src->free_key, src->free_value);
    for (size_t i = 0; i < src->used_slots->size; i++) {
        const size_t *     index      = (size_t *) src->used_slots->body[i];
        const linked_list *entry_list = src->table[*index];
        const list_node *  entry_node = entry_list->head;
        while (entry_node != NULL) {
            const hashtable_entry *entry = (hashtable_entry *) entry_node->data;
            void *                 key   = entry->key;
            void *                 value = entry->value;
            hashtable_set(copy, key_copy(key), value_copy(value));
            entry_node = entry_node->next;
        }
    }
    return copy;
}


/**
 *Create a new, empty hashtable
 */
hashtable *hashtable_create(size_t (*hash_func)(void *),
                            bool (*  key_equals)(void *, void *),
                            void (*  free_key)(void *),
                            void (*  free_value)(void *)) {
    hashtable *table = malloc(sizeof(*table));
    if (!table) {
        fprintf(stderr, "Error: malloc failed for hashtable\n");
        exit(EXIT_FAILURE);
    }

    table->hash_func  = hash_func;
    table->key_equals = key_equals;
    table->free_key   = free_key;
    table->free_value = free_value;

    table->table_size = HASHTABLE_INITIAL_CAPACITY;

    table->table = calloc(table->table_size, sizeof(*table->table));
    if (!table->table) {
        fprintf(stderr, "Error: calloc failed for hashtable table\n");
        free(table);
        exit(EXIT_FAILURE);
    }

    table->used_slots = arraylist_create(table->table_size, free);
    if (!table->used_slots) {
        fprintf(stderr, "Error: arraylist_create failed\n");
        free(table->table);
        free(table);
        exit(EXIT_FAILURE);
    }

    table->key_count = 0;
    return table;
}


/**
 * Allocate a new memory block with the given capacity.
 */
hashtable_entry *hashtable_body_allocate(const unsigned int capacity) {
    return calloc(capacity, sizeof(hashtable_entry));
}

/*// Ensure proper memory management in free_entry_list
static void free_entry_list(const hashtable *table, linked_list *entry_list) {
    list_node *current = entry_list->head;

    while (current != NULL) {
        list_node *next = current->next;

        hashtable_entry *entry = current->data;
        if (table->free_key) {
            table->free_key(entry->key);
        }
        if (table->free_value) {
            table->free_value(entry->value);
        }

        free(entry);
        free(current);

        current = next;
    }
    free(entry_list);
}

// Ensure proper memory management in hashtable_destroy
void hashtable_destroy(hashtable *t) {
    for (size_t i = 0; i < t->table_size; i++) {
        linked_list *entry_list = t->table[i];
        if (entry_list != NULL) {
            free_entry_list(t, entry_list);
        }
    }
    free(t->table);
    arraylist_destroy(t->used_slots);
    free(t);
}*/

static void free_hashtable_entry(void *data) {
    hashtable_entry *entry = data;

    if (entry == NULL)
        return;

    if (entry->free_key) {
        entry->free_key(entry->key);
    }

    if (entry->free_value) {
        entry->free_value(entry->value);
    }

    free(entry); // Free the entry itself
}


static void free_entry_list(linked_list *entry_list) {
    if (entry_list == NULL)
        return;

    linked_list_free(entry_list, free_hashtable_entry);
}


void hashtable_destroy(hashtable *t) {
    if (t == NULL)
        return;

    for (size_t i = 0; i < t->table_size; i++) {
        linked_list *entry_list = t->table[i];
        if (entry_list != NULL) {
            linked_list_free(entry_list, free_hashtable_entry);
        }
    }

    free(t->table);
    arraylist_destroy(t->used_slots);
    free(t);
}


size_t string_hash_function(void *key) {
    unsigned long hash = 5381;
    char *        str  = (char *) key;
    int           c;
    while ((c = (unsigned char) *str++))
        hash  = ((hash << 5) + hash) + c;
    return hash;
}

bool string_equals(void *key1, void *key2) {
    char *strkey1 = (char *) key1;
    char *strkey2 = (char *) key2;
    return strcmp(strkey1, strkey2) == 0;
}

size_t int_hash_function(void *key) {
    const size_t *lkey = (size_t *) key;
    return *lkey * 2654435761 % 4294967296;
}


bool int_equals(void *key1, void *key2) {
    const long *lkey1 = (long *) key1;
    const long *lkey2 = (long *) key2;
    return *lkey1 == *lkey2;
}

size_t pointer_hash_function(void *data) {
    const long key = (long) data;
    return key * 2654435761 % (4294967296);
}

bool pointer_equals(void *key1, void *key2) {
    return key1 == key2;
}

void hashtable_visualize(const hashtable *table) {
    if (!table) {
        printf("Hashtable is NULL.\n");
        return;
    }

    printf("Hashtable Visualization:\n");
    printf("Table Size: %zu\n", table->table_size);
    printf("Key Count: %zu\n", table->key_count);
    printf("Used Slots (%zu): [", table->used_slots->size);
    for (size_t i = 0; i < table->used_slots->size; i++) {
        size_t *index = (size_t *) table->used_slots->body[i];
        printf("%zu", *index);
        if (i < table->used_slots->size - 1) {
            printf(", ");
        }
    }
    printf("]\n");

    printf("Buckets:\n");
    for (size_t i = 0; i < table->table_size; i++) {
        linked_list *bucket = table->table[i];
        if (bucket) {
            printf("  [%zu]: ", i);
            list_node *node = bucket->head;
            while (node) {
                hashtable_entry *entry = (hashtable_entry *) node->data;
                printf("(Key: %p, Value: %p) -> ", entry->key, entry->value);
                node = node->next;
            }
            printf("NULL\n");
        } else {
            printf("  [%zu]: NULL\n", i);
        }
    }
}

