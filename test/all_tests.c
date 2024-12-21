//
// Created by dgood on 12/17/24.
//
#include "../Unity/unity_fixture.h"


static void RunAllTests(void)
{
    RUN_TEST_GROUP(lexer);
    RUN_TEST_GROUP(parser);
}


int main(int argc, char * argv[]) {
    return UnityMain(argc, argv, RunAllTests);
}