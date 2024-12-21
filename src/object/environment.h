//
// Created by dgood on 12/4/24.
//

#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "../datastructures/hashmap.h"

typedef struct environment {
    hashtable *table;
    struct environment *outer;
} environment;

void *environment_get(const environment *, char *);
void environment_put(environment *, char *, void *);
environment *environment_create(void);
environment *environment_create_enclosed(environment *);
void environment_free(environment *);
environment *environment_copy(environment *);
#endif //ENVIRONMENT_H
