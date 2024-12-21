//
// Created by dgood on 12/7/24.
//

#ifndef REPL_H
#define REPL_H

#include "../parser/parser.h"

int repl(void);

int execute_file(const char *);

static void print_parse_errors(const parser *parser);
#endif //REPL_H
