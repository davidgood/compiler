//
// Created by dgood on 12/18/24.
//
#include "../Unity/src/unity.h"
#include "../src/object/object.h"
#include "test_utils.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

static void test_string_hash_key(void) {
    print_test_separator_line();
    printf("Testing hash key generation for string objects\n");

    object_string *hello  = object_create_string("hello world", 11);
    object_string *hello2 = object_create_string("hello world", 11);
    object_string *lorem  = object_create_string("Lorem ipsum dolor sit amet", 26);
    object_string *lorem2 = object_create_string("Lorem ipsum dolor sit amet", 26);

    size_t hello_hash  = hello->object.hash((object_string *) hello);
    size_t hello2_hash = hello2->object.hash((object_string *) hello2);
    size_t lorem_hash  = lorem->object.hash((object_string *) lorem);
    size_t lorem2_hash = lorem2->object.hash((object_string *) lorem2);

    TEST_ASSERT_EQUAL_size_t(hello_hash, hello2_hash);

    TEST_ASSERT_EQUAL_size_t(lorem_hash, lorem2_hash);

    TEST_ASSERT_NOT_EQUAL_size_t(hello_hash, lorem_hash);

    object_free(hello);
    object_free(hello2);
    object_free(lorem);
    object_free(lorem2);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_string_hash_key);
    return UNITY_END();
}
