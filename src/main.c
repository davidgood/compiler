#include <err.h>
#include <stdlib.h>

#include "repl/repl.h"

int main(const int argc, char **argv) {
    if (argc == 1)
        return repl();
    if (argc == 2)
        return execute_file(argv[1]);
    err(EXIT_FAILURE, "Unsupported number of arguments %d", argc);
}
