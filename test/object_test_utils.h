//
// Created by dgood on 12/18/24.
//

#ifndef OBJECT_TEST_UTILS_H
#define OBJECT_TEST_UTILS_H
#include "../src/object/object.h"

void test_object_object(object_object *, object_object *);
void test_null_object(object_object *);
void test_integer_object(object_object *, long);
void test_boolean_object(object_object *, bool);
void test_array_object(object_object *, object_object *);
void test_hash_object(object_object *, object_object *);

#endif //OBJECT_TEST_UTILS_H
