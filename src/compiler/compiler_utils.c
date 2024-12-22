//
// Created by dgood on 12/19/24.
//

#include "compiler_utils.h"

#include <err.h>
#include <string.h>
#include <stdarg.h>
#include "../object/object.h"
#include "symbol_table.h"

void *_strdup(void *s) {
    return strdup(s);
}

void *_copy_object(void *obj) {
    return object_copy_object(obj);
}

void *_copy_symbol(void *obj) {
    const symbol *src = obj;
    symbol *new_symbol = symbol_init(src->name, src->scope, src->index);
    return new_symbol;
}

char *get_err_msg(const char *s, ...) {
    char *msg = NULL;
    va_list ap;
    va_start(ap, s);
    const int retval = vasprintf(&msg, s, ap);
    va_end(ap);
    if (retval == -1) {
        err(EXIT_FAILURE, "malloc failed");
    }
    return msg;
}

int compare_object_hash_keys(const void *v1, const void *v2) {
    // Cast v1 and v2 to pointers to ast_node *
    ast_node *n1 = *(ast_node **)v1;
    ast_node *n2 = *(ast_node **)v2;

    // Check if function pointers are valid
    if (n1 == NULL || n2 == NULL || n1->string == NULL || n2->string == NULL) {
        err(EXIT_FAILURE, "Null pointer encountered in comparator");
    }

    // Get the strings from the nodes
    char *s1 = n1->string((ast_hash_literal *)n1);
    char *s2 = n2->string((ast_hash_literal *)n2);

    // Compare the strings
    const int ret = strcmp(s1, s2);

    // Free the strings
    free(s1);
    free(s2);

    return ret;
}