//
// Created by dgood on 12/5/24.
//

#include "builtins.h"

#include <string.h>

#include "../datastructures/hashmap.h"
#include "../datastructures/linked_list.h"

#include <stdlib.h>

const char *BUILTINS[MAX_BUILTINS] = {
        "len",
        "puts",
        "first",
        "last",
        "rest",
        "push",
        "type",
};

static object_object *len(linked_list *);

static object_object *first(linked_list *);

static object_object *last(linked_list *);

static object_object *rest(linked_list *);

static object_object *push(linked_list *);

static object_object *_puts(linked_list *);

static object_object *type(linked_list *);

static char *builtin_inspect(object_object *);

const object_builtin BUILTIN_LEN   = {{OBJECT_BUILTIN, builtin_inspect}, len};
const object_builtin BUILTIN_FIRST = {{OBJECT_BUILTIN, builtin_inspect}, first};
const object_builtin BUILTIN_LAST  = {{OBJECT_BUILTIN, builtin_inspect}, last};
const object_builtin BUILTIN_REST  = {{OBJECT_BUILTIN, builtin_inspect}, rest};
const object_builtin BUILTIN_PUSH  = {{OBJECT_BUILTIN, builtin_inspect}, push};
const object_builtin BUILTIN_PUTS  = {{OBJECT_BUILTIN, builtin_inspect}, _puts};
const object_builtin BUILTIN_TYPE  = {{OBJECT_BUILTIN, builtin_inspect}, type};

static char *builtin_inspect(object_object *object) {
    return "builtin function";
}

static object_object *_puts(linked_list *arguments) {
    const list_node *node = arguments->tail;
    while (node != NULL) {
        object_object * arg = (object_object *) node->data;
        node     = node->next;
        char *s  = arg->inspect(arg);
        printf("%s\n", s);
        free(s);
    }
    return (object_object *) object_create_null();
}

static object_object *type(linked_list *arguments) {
    if (arguments->size != 1) {
        return (object_object *)
                object_create_error("wrong number of arguments. got=%zu, want=1",
                                    arguments->size);
    }

    const object_object *arg      = (object_object *) arguments->tail->data;
    const char *         typename = get_type_name(arg->type);
    return (object_object *) object_create_string(typename, strlen(typename));
}

static object_object *len(linked_list *arguments) {
    object_string *str;
    object_array * array;
    object_hash *  hash_obj;
    if (arguments->size != 1) {
        return (object_object *) object_create_error(
                "wrong number of arguments. got=%zu, want=1", arguments->size);
    }

    object_object *arg = (object_object *) arguments->tail->data;
    switch (arg->type) {
        case OBJECT_STRING:
            str = (object_string *) arg;
            return (object_object *) object_create_int(str->length);
        case OBJECT_ARRAY:
            array = (object_array *) arg;
            return (object_object *) object_create_int(array->elements->size);
        case OBJECT_HASH:
            hash_obj = (object_hash *) arg;
            return (object_object *) object_create_int(hash_obj->pairs->table_size);
        default:
            return (object_object *) object_create_error(
                    "argument to `len` not supported, got %s", get_type_name(arg->type));
    }
}

static object_object *first(linked_list *arguments) {
    if (arguments->size != 1) {
        return (object_object *)
                object_create_error("wrong number of arguments. got=%zu, want=1",
                                    arguments->size);
    }

    object_object *arg = (object_object *) arguments->head->data;
    if (arg->type != OBJECT_ARRAY) {
        return (object_object *) object_create_error(
                "argument to `first` must be ARRAY, got %s", get_type_name(arg->type));
    }
    object_array *array = (object_array *) arg;
    if (array->elements->size > 0)
        return object_copy_object(array->elements->body[0]);
    else
        return (object_object *) object_create_null();
}

static object_object *
last(linked_list *arguments) {
    if (arguments->size != 1) {
        return (object_object *)
                object_create_error("wrong number of arguments. got=%zu, want=1",
                                    arguments->size);
    }

    object_object *arg = (object_object *) arguments->head->data;
    if (arg->type != OBJECT_ARRAY) {
        return (object_object *) object_create_error(
                "argument to `last` must be ARRAY, got %s", get_type_name(arg->type)
                );
    }

    const object_array *array = (object_array *) arg;
    if (array->elements->size > 0) {
        return (object_object *) object_copy_object(
                arraylist_get(array->elements, array->elements->size - 1));
    }
    return (object_object *) object_create_null();

}

static object_object *rest(linked_list *arguments) {

    if (arguments->size != 1) {
        return (object_object *)
                object_create_error("wrong number of arguments. got=%zu, want=1",
                                    arguments->size);
    }

    object_object *arg = (object_object *) arguments->head->data;
    if (arg->type != OBJECT_ARRAY) {
        return (object_object *) object_create_error(
                "argument to `rest` must be ARRAY, got %s",
                get_type_name(arg->type));
    }

    object_array *array = (object_array *) arg;

    if (array->elements->size == 0) {
        return (object_object *) object_create_null();
    }

    arraylist *rest_elements = arraylist_create(array->elements->size - 1);
    for (size_t i = 1; i < array->elements->size; i++) {
        object_object *obj = object_copy_object(array->elements->body[i]);
        arraylist_add(rest_elements, obj);
    }
    object_array *rest_array = object_create_array(rest_elements);
    return (object_object *) rest_array;
}

static object_object *push(linked_list *arguments) {
    object_object *obj;

    if (arguments->size != 2) {
        return (object_object *)
                object_create_error("wrong number of arguments. got=%zu, want=2",
                                    arguments->size);
    }

    object_object *arg = (object_object *) arguments->head->data;
    if (arg->type != OBJECT_ARRAY) {
        return (object_object *)
                object_create_error("argument to `push` must be ARRAY, got %s",
                                    get_type_name(arg->type));
    }

    const object_array *array             = (object_array *) arg;
    arraylist *         new_list_elements = arraylist_create(arguments->size + 1);
    for (size_t i = 0; i < array->elements->size; i++) {
        obj = object_copy_object(array->elements->body[i]);
        arraylist_add(new_list_elements, obj);
    }

    obj = (object_object *) arguments->head->next->data;
    arraylist_add(new_list_elements, object_copy_object(obj));
    object_array *new_array = object_create_array(new_list_elements);
    return (object_object *) new_array;
}

object_builtin *get_builtins(const char *name) {
    if (strcmp(name, "len") == 0)
        return (object_builtin *) &BUILTIN_LEN;
    if (strcmp(name, "first") == 0)
        return (object_builtin *) &BUILTIN_FIRST;
    if (strcmp(name, "last") == 0)
        return (object_builtin *) &BUILTIN_LAST;
    if (strcmp(name, "rest") == 0)
        return (object_builtin *) &BUILTIN_REST;
    if (strcmp(name, "push") == 0)
        return (object_builtin *) &BUILTIN_PUSH;
    if (strcmp(name, "puts") == 0)
        return (object_builtin *) &BUILTIN_PUTS;
    if (strcmp(name, "type") == 0)
        return (object_builtin *) &BUILTIN_TYPE;
    return NULL;
}

