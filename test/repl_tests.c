//
// Created by dgood on 12/27/24.
//
#include "../src/repl/repl.h"
#include "../Unity/src/unity.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}


// void test_repl() {
//     repl();
// }

void test_execute() {
    execute_file("/home/dgood/Projects/C/compiler/test2.txt");
}

int main(const int argc, char **argv) {
    UNITY_BEGIN();

    //RUN_TEST(test_repl);
    RUN_TEST(test_execute);

    return UNITY_END();
}
