//
// Created by dgood on 12/6/24.
//

#include "conversions.h"

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>

size_t *create_size_t_array(size_t count, ...) {
    va_list ap;
    va_start(ap, count);
    size_t *array = malloc(sizeof(*array) * count);
    if (array == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    for (size_t i = 0; i < count; i++) {
        array[i] = (size_t) va_arg(ap, size_t);
    }
    va_end(ap);
    return array;
}

uint8_t *create_uint8_array(const size_t count, ...) {
    va_list ap;
    va_start(ap, count);
    uint8_t *array = malloc(sizeof(*array) * count);
    if (array == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    for (size_t i = 0; i < count; i++) {
        array[i] = va_arg(ap, int);
    }
    va_end(ap);
    return array;
}

static size_t power_ceil(size_t x) {
    size_t power = 1;
    while (x >>= 1) {
        power++;
    }
    return power;
}

uint8_t *size_t_to_uint8_be(size_t value, size_t width) {
    uint8_t *array = malloc(sizeof(*array) * width);
    if (array == NULL) {
        err(EXIT_FAILURE, "malloc failed");
    }
    uint64_t newval = htobe16(value); // to make sure we work on both BE and LE archs
    uint8_t *x = (uint8_t *) &newval;
    int j = width - 1;
    int i = sizeof(value) - 1;
    while (j >= 0) {
        array[j--] = x[i--];
    }
    return array;
}


size_t be_to_size_t(const uint8_t *bytes) {
    if (bytes[0] == 0) {
        return (size_t) bytes[1] + bytes[0];
    }
    return (size_t) (bytes[0] << 8) + bytes[1];
}

static size_t calculate_string_size(long l) {
    size_t size = 0;
    if (l < 0) {
        size++;
        l = -1 * l;
    }
    while (l >= 10) {
        l /= 10;
        size++;
    }
    return size + 1;
}

char *long_to_string(long l) {
    const size_t str_size    = calculate_string_size(l) + 1;
    char *       str         = malloc(str_size + 1);
    size_t       index       = str_size;
    const bool   is_negative = l < 0;
    if (is_negative) {
        l = -1 * l;
    }
    str[--index] = 0;
    while (l >= 10) {
        const long rem = l % 10;
        l /= 10;
        str[--index] = 48 + rem;
    }
    str[--index] = 48 + l;
    if (is_negative)
        str[--index] = '-';
    return str;
}

const char *bool_to_string(const bool value) {
    if (value) {
        return "true";
    }
    return "false";
}
