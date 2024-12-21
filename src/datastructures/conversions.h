//
// Created by dgood on 12/6/24.
//

#ifndef CONVERSIONS_H
#define CONVERSIONS_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

char *long_to_string(long);
const char *bool_to_string(bool);

size_t *create_size_t_array(size_t, ...);

uint8_t *create_uint8_array(size_t, ...);

uint8_t *size_t_to_uint8_be(size_t, size_t);

size_t be_to_size_t(const uint8_t *bytes);

#define be_to_size_t_1(bytes) ((size_t) (bytes)[0])

#endif //CONVERSIONS_H
