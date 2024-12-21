//
// Created by dgood on 12/18/24.
//

#include <string.h>
#include "../Unity/src/unity.h"
#include "../datastructures/hashmap.h"
#include "../src/compiler/symbol_table.h"
#include "test_utils.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}
static void compare_symbols(symbol *expected, symbol *actual) {
    TEST_ASSERT_EQUAL_STRING(expected->name, actual->name);
    TEST_ASSERT_EQUAL_INT(expected->index, actual->index);
    TEST_ASSERT_EQUAL_INT(expected->scope, actual->scope);
}

static void test_define(void) {
    print_test_separator_line();
    printf("Testing symbol define\n");
    hashtable *expected = hashtable_create(string_hash_function, string_equals, NULL, symbol_free);
    hashtable_set(expected, "a", symbol_init("a", GLOBAL, 0));
    hashtable_set(expected, "b", symbol_init("b", GLOBAL, 1));
    hashtable_set(expected, "c", symbol_init("c", LOCAL, 0));
    hashtable_set(expected, "d", symbol_init("d", LOCAL, 1));
    hashtable_set(expected, "e", symbol_init("e", LOCAL, 0));
    hashtable_set(expected, "f", symbol_init("f", LOCAL, 1));

    symbol_table *global = symbol_table_init();
    symbol *s = symbol_define(global, "a");
    compare_symbols((symbol *) hashtable_get(expected, "a"), s);

    s = symbol_define(global, "b");
    compare_symbols((symbol *) hashtable_get(expected, "b"), s);

    symbol_table *first_local = enclosed_symbol_table_init(global);
    s = symbol_define(first_local, "c");
    compare_symbols((symbol *) hashtable_get(expected, "c"), s);
    s = symbol_define(first_local, "d");
    compare_symbols((symbol *) hashtable_get(expected, "d"), s);

    symbol_table *second_local = enclosed_symbol_table_init(first_local);
    s = symbol_define(second_local, "e");
    compare_symbols((symbol *) hashtable_get(expected, "e"), s);
    s = symbol_define(second_local, "f");
    compare_symbols((symbol *) hashtable_get(expected, "f"), s);


    hashtable_destroy(expected);
    symbol_table_free(global);
    symbol_table_free(first_local);
    symbol_table_free(second_local);
}

static void test_resolve_global(void) {
    print_test_separator_line();
    printf("Testing global symbols resolution\n");
    symbol_table *global = symbol_table_init();
    symbol_define(global, "a");
    symbol_define(global, "b");

    symbol *expected[] = {
        symbol_init("a", GLOBAL, 0),
        symbol_init("b", GLOBAL, 1)
    };

    for (size_t i = 0; i < 2; i++) {
        symbol *sym = expected[i];
        symbol *result = symbol_resolve(global, sym->name);
        TEST_ASSERT_NOT_NULL(result);
        compare_symbols(sym, result);
        symbol_free(sym);
    }
    symbol_table_free(global);
}

static void test_define_and_resolve_function_name(void) {
    print_test_separator_line();
    printf("Testing function name resolution\n");
    symbol_table *global = symbol_table_init();
    symbol_define_function(global, "a");
    symbol *expected = symbol_init("a", FUNCTION_SCOPE, 0);
    symbol *actual = symbol_resolve(global, "a");
    compare_symbols(expected, actual);
    symbol_table_free(global);
    symbol_free(expected);
}

static void test_shadowing_function_name(void) {
    print_test_separator_line();
    printf("Testing shadowing of function name\n");
    symbol_table *global = symbol_table_init();
    symbol_define_function(global, "a");
    symbol_define(global, "a");
    symbol *expected = symbol_init("a", GLOBAL, 0);
    symbol *actual = symbol_resolve(global, "a");
    compare_symbols(expected, actual);
    symbol_table_free(global);
    symbol_free(expected);
}

static void test_resolve_unresolvable_free(void) {
    print_test_separator_line();
    printf("Testing unresolvable free symbols\n");
    symbol_table *global = symbol_table_init();
    symbol_define(global, "a");

    symbol_table *first_local = enclosed_symbol_table_init(global);
    symbol_define(first_local, "c");

    symbol_table *second_local = enclosed_symbol_table_init(first_local);
    symbol_define(second_local, "e");
    symbol_define(second_local, "f");

    symbol *expected[] = {
        symbol_init("a", GLOBAL, 0),
        symbol_init("c", FREE, 0),
        symbol_init("e", LOCAL, 0),
        symbol_init("f", LOCAL, 1)
    };

    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        symbol *expected_sym = expected[i];
        symbol *actual = symbol_resolve(second_local, expected_sym->name);
        TEST_ASSERT_NOT_NULL(actual);
        compare_symbols(expected_sym, actual);
        symbol_free(expected_sym);
    }

    const char *unresolvable[] = {
        "b",
        "d"
    };

    for (size_t i = 0; i < sizeof(unresolvable) / sizeof(unresolvable[0]); i++) {
        symbol *resolved = symbol_resolve(second_local, unresolvable[i]);
        TEST_ASSERT_NULL(resolved);
    }

    symbol_table_free(second_local);
    symbol_table_free(first_local);
    symbol_table_free(global);
}

static void test_resolve_free(void) {
    print_test_separator_line();
    printf("Testing free symbols resolution\n");
    symbol_table *global = symbol_table_init();
    symbol_define(global, "a");
    symbol_define(global, "b");

    symbol_table *first_local = enclosed_symbol_table_init(global);
    symbol_define(first_local, "c");
    symbol_define(first_local, "d");

    symbol_table *second_local = enclosed_symbol_table_init(first_local);
    symbol_define(second_local, "e");
    symbol_define(second_local, "f");

    typedef struct {
        symbol_table *table;
        symbol *expected_symbols[10];
        symbol *expected_free[10];
        size_t expected_free_count;
    } test;

    test tests[] = {
        {
            first_local,
            {
                symbol_init("a", GLOBAL, 0),
                symbol_init("b", GLOBAL, 1),
                symbol_init("c", LOCAL, 0),
                symbol_init("d", LOCAL, 1)
            },
            {},
            0
        },
        {
            second_local,
            {
                symbol_init("a", GLOBAL, 0),
                symbol_init("b", GLOBAL, 1),
                symbol_init("c", FREE, 0),
                symbol_init("d", FREE, 1),
                symbol_init("e", LOCAL, 0),
                symbol_init("f", LOCAL, 1)
            },
            {
                symbol_init("c", LOCAL, 0),
                symbol_init("d", LOCAL, 1)
            },
            2
        }
    };

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        test t = tests[i];
        for (size_t j = 0; j < sizeof(t.expected_symbols) / sizeof(t.expected_symbols[0]); j++) {
            symbol *expected_sym = t.expected_symbols[j];
            if (expected_sym == NULL)
                break;
            symbol *actual_sym = symbol_resolve(t.table, expected_sym->name);
            TEST_ASSERT_NOT_NULL(actual_sym);
            compare_symbols(expected_sym, actual_sym);
            symbol_free(expected_sym);
        }
        TEST_ASSERT_EQUAL_INT(t.expected_free_count, t.table->free_symbols->size);
       for (size_t j = 0; j < t.expected_free_count; j++) {
           symbol *expected_sym = t.expected_free[j];
           if (expected_sym == NULL) {
               break;
           }
           symbol *actual_sym = arraylist_get(t.table->free_symbols, j);
           compare_symbols(expected_sym, actual_sym);
           symbol_free(expected_sym);
       }
    }
    symbol_table_free(global);
    symbol_table_free(first_local);
    symbol_table_free(second_local);

}

static void test_resolve_local(void) {
    print_test_separator_line();
    printf("Testing local symbol tables\n");
    symbol_table *global = symbol_table_init();
    symbol_define(global, "a");
    symbol_define(global, "b");
    symbol_table *first_local = enclosed_symbol_table_init(global);
    symbol_define(first_local, "c");
    symbol_define(first_local, "d");

    symbol *expected[] = {
        symbol_init("a", GLOBAL, 0),
        symbol_init("b", GLOBAL, 1),
        symbol_init("c", LOCAL, 0),
        symbol_init("d", LOCAL, 1)
    };

    for (size_t i = 0; i < 4; i++) {
        symbol *sym = expected[i];
        symbol *result = symbol_resolve(first_local, sym->name);
        TEST_ASSERT_NOT_NULL(result);
        compare_symbols(sym, result);
        symbol_free(sym);
    }
    symbol_table_free(global);
    symbol_table_free(first_local);
}

static void test_define_resolve_builtins(void) {
    print_test_separator_line();
    printf("Testing symbol resolution for builtins\n");
    symbol_table *global = symbol_table_init();
    symbol_table *first_local = enclosed_symbol_table_init(global);
    symbol_table *second_local = enclosed_symbol_table_init(first_local);
    symbol *expected[] = {
        symbol_init("a", BUILTIN, 0),
        symbol_init("c", BUILTIN, 1),
        symbol_init("e", BUILTIN, 2),
        symbol_init("f", BUILTIN, 3)
    };
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        symbol *s = expected[i];
        symbol_define_builtin(global, i, s->name);
    }

    symbol_table *tables[] = {global, first_local, second_local};

    for (size_t i = 0; i < sizeof(tables) / sizeof(tables[0]); i++) {
        for (size_t j = 0; j < sizeof(expected) / sizeof(expected[0]); j++) {
            symbol_table *table = tables[i];
            symbol *sym = expected[j];
            symbol *resolved = symbol_resolve(table, sym->name);
            TEST_ASSERT_NOT_NULL(resolved);
            compare_symbols(sym, resolved);
        }
    }
    symbol_table_free(global);
    symbol_table_free(first_local);
    symbol_table_free(second_local);
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
        symbol_free(expected[i]);
}

static void test_resolve_nested_local(void) {
    print_test_separator_line();
    printf("Testing nested local symbol tables\n");
    symbol_table *global = symbol_table_init();
    symbol_define(global, "a");
    symbol_define(global, "b");

    symbol_table *first_local = enclosed_symbol_table_init(global);
    symbol_define(first_local, "c");
    symbol_define(first_local, "d");

    symbol_table *second_local = enclosed_symbol_table_init(first_local);
    symbol_define(second_local, "e");
    symbol_define(second_local, "f");

    typedef struct testcase {
        symbol_table *table;
        symbol *expected_symbols[4];
    } testcase;

    testcase tests[] = {
        {
            first_local,
            {
                symbol_init("a", GLOBAL, 0),
                symbol_init("b", GLOBAL, 1),
                symbol_init("c", LOCAL, 0),
                symbol_init("d", LOCAL, 1)
            }
        },
        {
            second_local,
            {
                symbol_init("a", GLOBAL, 0),
                symbol_init("b", GLOBAL, 1),
                symbol_init("e", LOCAL, 0),
                symbol_init("f", LOCAL, 1)
            }
        }
    };
    size_t ntests = sizeof(tests)/sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        testcase t = tests[i];
        for (size_t j = 0; j < 4; j++) {
            symbol *resolved = symbol_resolve(t.table, t.expected_symbols[j]->name);
            TEST_ASSERT_NOT_NULL(resolved);
            compare_symbols(t.expected_symbols[j], resolved);
            symbol_free(t.expected_symbols[j]);
        }
    }
    symbol_table_free(global);
    symbol_table_free(first_local);
    symbol_table_free(second_local);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_define);
    RUN_TEST(test_resolve_global);
    RUN_TEST(test_resolve_local);
    RUN_TEST(test_resolve_nested_local);
    RUN_TEST(test_define_resolve_builtins);
    RUN_TEST(test_resolve_free);
    RUN_TEST(test_resolve_unresolvable_free);
    RUN_TEST(test_define_and_resolve_function_name);
    RUN_TEST(test_shadowing_function_name);
    return UNITY_END();
}