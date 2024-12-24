//
// Created by dgood on 12/4/24.
//

#include "environment.h"

#include "object.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void free_value(void *value) {
    object_object *obj = value;
    object_free(obj);
}

environment *environment_create(void) {
    hashtable *table = hashtable_create(
            string_hash_function,
            string_equals,
            free,
            free_value);
    environment *env = malloc(sizeof(*env));
    assert(env != NULL);
    env->table = table;
    env->outer = nullptr;
    return env;
}

environment *environment_create_enclosed(environment *outer) {
    environment *env = environment_create();
    env->outer       = outer;
    return env;
}

void environment_put(environment *env, char *name, void *value) {
    hashtable_set(env->table, name, value);
}

void *environment_get(const environment *env, char *name) {
    void *value = hashtable_get(env->table, (void *) name);
    if (value != NULL)
        return value;

    if (env->outer != NULL)
        return environment_get(env->outer, name);
    return NULL;
}

void environment_free(environment *env) {
    hashtable_destroy(env->table);
    free(env);
}

environment *copy_env(const environment *env) {
    fprintf(stdout, "Copying Environment\n");
    environment *new_env = environment_create();
    for (size_t i = 0; i < env->table->table_size; i++) {
        const linked_list *entry_list = env->table->table[i];
        if (entry_list == NULL) {
            continue;
        }
        list_node *node = entry_list->head;
        while (node != NULL) {
            const hashtable_entry *entry = (hashtable_entry *) node->data;
            const char *           key   = (char *) entry->key;
            object_object *        value = (object_object *) entry->value;
            environment_put(new_env, strdup(key), object_copy_object(value));
            node = node->next;
        }
    }
    return new_env;
}
