//
// Created by dgood on 12/5/24.
//

#ifndef BUILTINS_H
#define BUILTINS_H
#include "../datastructures/hashmap.h"
#include "object.h"

#define MAX_BUILTINS 7

typedef hashtable *builtins_table;

object_builtin *get_builtins(const char *);

extern const char *         BUILTINS[MAX_BUILTINS];
extern const object_builtin BUILTIN_LEN;
extern const object_builtin BUILTIN_FIRST;
extern const object_builtin BUILTIN_LAST;
extern const object_builtin BUILTIN_REST;
extern const object_builtin BUILTIN_PUSH;
extern const object_builtin BUILTIN_PUTS;
extern const object_builtin BUILTIN_TYPE;


#define get_builtins_count() sizeof(BUILTINS)/sizeof(BUILTINS[0])
#define get_builtins_name(x) BUILTINS[x]
#endif //BUILTINS_H
