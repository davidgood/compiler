//
// Created by dgood on 12/17/24.
//

#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "../../Unity/src/unity.h"
#include "../src/ast/ast.h"
#include "../src/datastructures/arraylist.h"
#include "../src/datastructures/conversions.h"
#include "../src/datastructures/linked_list.h"
#include "../src/lexer/lexer.h"
#include "../src/parser/parser.h"
#include "../src/token/token.h"
#include "test_utils.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void check_parser_errors(parser *parser) {
    if (parser->errors == NULL)
        return;
    list_node *errors_head = parser->errors->head;
    while (errors_head) {
        printf("parser error: %s\n", (char *) errors_head->data);
        errors_head = errors_head->next;
    }
    exit(1);
}

bool string_to_bool(const char *str) {
    if (strncmp("true", str, 4) == 0)
        return true;
    else
        return false;
}

void test_integer_literal_value(ast_expression *exp, long expected_value) {
    printf("\tTesting Integer Literal Value\n");

    TEST_ASSERT_EQUAL_INT(exp->expression_type, INTEGER_EXPRESSION);
    printf("\tThe expression is of type INTEGER_EXPRESSION\n");
    ast_integer *int_exp = (ast_integer *) exp;
    TEST_ASSERT_EQUAL_INT64(int_exp->value, expected_value);
    char *literal          = int_exp->expression.node.string(int_exp);
    char *expected_literal = long_to_string(expected_value);
    TEST_ASSERT_EQUAL_STRING(literal, expected_literal);
    free(expected_literal);
    free(literal);
}

void test_identifier(ast_expression *exp, const char *expected_value) {
    printf("\tTesting Identifier\n");
    TEST_ASSERT_EQUAL_INT(exp->expression_type, IDENTIFIER_EXPRESSION);

    ast_identifier *ident_exp = (ast_identifier *) exp;
    TEST_ASSERT_EQUAL_STRING(ident_exp->value, expected_value);

    char *tok_literal = ident_exp->expression.node.token_literal(ident_exp);
    TEST_ASSERT_EQUAL_STRING(tok_literal, expected_value);
}

void test_boolean_literal(ast_expression *exp, const char *expected_value) {
    printf("\tTesting Boolean Literal\n");

    TEST_ASSERT_EQUAL_INT(exp->expression_type, BOOLEAN_EXPRESSION);

    ast_boolean_expression *bool_exp = (ast_boolean_expression *) exp;
    TEST_ASSERT_EQUAL(bool_exp->value, string_to_bool(expected_value));

    char *tok_literal = bool_exp->expression.node.token_literal(bool_exp);
    TEST_ASSERT_EQUAL_STRING(tok_literal, expected_value);
}

void test_literal_expression(ast_expression *exp, const char *value) {
    printf("\tTesting Literal Expression\n");

    long expected_integer_value;
    switch (exp->expression_type) {
        case INTEGER_EXPRESSION:
            expected_integer_value = atol(value);
            test_integer_literal_value(exp, expected_integer_value);
            return;
        case IDENTIFIER_EXPRESSION:
            test_identifier(exp, value);
            return;
        case BOOLEAN_EXPRESSION:
            test_boolean_literal(exp, value);
            return;
        default:
            err(EXIT_FAILURE, "Unsupported expression type passed to test_literal_expression: %s",
                ast_get_expression_type_name(exp->expression_type));
    }
}

void test_infix_expression(ast_expression *exp, const char *operator, const char * left, const char *right) {
    printf("\tTesting Infix Expression\n");

    TEST_ASSERT_EQUAL_INT(exp->expression_type, INFIX_EXPRESSION);

    ast_infix_expression *infix_exp = (ast_infix_expression *) exp;

    test_literal_expression(infix_exp->left, left);

    TEST_ASSERT_EQUAL_STRING(infix_exp->operator, operator);
    test_literal_expression(infix_exp->left, left);
    test_literal_expression(infix_exp->right, right);
}

void test_parser_errors(void) {
    const char *input = "let x 5;\n"
                        "let = 10;\n"
                        "let 838383;\n";
    print_test_separator_line();
    printf("Testing parser errors\n");
    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_NOT_NULL(parser->errors);
    TEST_ASSERT_EQUAL_INT(parser->errors->size, 4);

    list_node *errors_head       = parser->errors->head;
    token_type expected_tokens[] = {ASSIGN, IDENT, IDENT};
    token_type actual_tokens[]   = {INT, ASSIGN, INT};
    int        i                 = 0;
    while (errors_head) {
        char *expected_error = NULL;
        asprintf(&expected_error, "expected next token to be %s, got %s instead",
                 token_get_name_from_type(expected_tokens[i]), token_get_name_from_type(actual_tokens[i]));
        TEST_ASSERT_EQUAL_STRING(errors_head->data, expected_error);
        free(expected_error);
        errors_head = errors_head->next;
        i++;
    }
    program_free(program);
    parser_free(parser);
}


void _test_let_stmt(ast_statement *stmt, const char *expected_identifier) {
    print_test_separator_line();
    printf("Testing Let Statement\n");

    char *tok_literal = stmt->node.token_literal(stmt);
    TEST_ASSERT_EQUAL_STRING(tok_literal, "let");

    ast_let_statement *let_stmt = (ast_let_statement *) stmt;

    TEST_ASSERT_EQUAL_STRING(let_stmt->name->value, expected_identifier);

    tok_literal = let_stmt->name->expression.node.token_literal(let_stmt->name);
    TEST_ASSERT_EQUAL_STRING(tok_literal, expected_identifier);
}

void test_let_stmt() {
    lexer         *lexer;
    parser        *parser;
    ast_program   *program;
    ast_statement *stmt;

    typedef struct {
        const char *input;
        const char *expected_identifier;
        const char *expected_value;
    } test_input;

    test_input tests[] = {
            {"let x = 5;\n", "x", "5"}, {"let y = 10;\n", "y", "10"}, {"let foobar = 838383;\n", "foobar", "838383"}};
    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    printf("Testing let statements\n");
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing let statement: %s\n", test.input);
        lexer   = lexer_init(test.input);
        parser  = parser_init(lexer);
        program = parse_program(parser);
        check_parser_errors(parser);
        TEST_ASSERT_NOT_NULL(program);
        TEST_ASSERT_EQUAL_INT(program->statement_count, 1);

        stmt = program->statements[0];
        TEST_ASSERT_EQUAL_INT(stmt->statement_type, LET_STATEMENT);
        _test_let_stmt(stmt, test.expected_identifier);

        ast_let_statement *let_stmt = (ast_let_statement *) stmt;
        test_literal_expression(let_stmt->value, test.expected_value);

        program_free(program);
        parser_free(parser);
    }
}

void test_identifier_expression() {
    const char *input = "foobar;\n";
    print_test_separator_line();
    printf("Testing identifier expression\n");
    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);
    check_parser_errors(parser);

    TEST_ASSERT_NOT_NULL(program);

    TEST_ASSERT_EQUAL_INT(program->statement_count, 1);

    TEST_ASSERT_EQUAL_INT(program->statements[0]->statement_type, EXPRESSION_STATEMENT);

    ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
    TEST_ASSERT_EQUAL_INT(exp_stmt->expression->expression_type, IDENTIFIER_EXPRESSION);

    ast_identifier *ident = (ast_identifier *) exp_stmt->expression;
    TEST_ASSERT_EQUAL_STRING(ident->value, "foobar");

    char *ident_token_literal = ident->expression.node.token_literal(ident);
    TEST_ASSERT_EQUAL_STRING(ident_token_literal, "foobar");

    program_free(program);
    parser_free(parser);
}

void test_parse_infix_expression() {
    typedef struct test_input {
        const char *input;
        const char *operator;
        const char *left;
        const char *right;
    } test_input;

    test_input tests[] = {{"5 + 5;", "+", "5", "5"},
                          {"5 - 5;", "-", "5", "5"},
                          {"5 * 5;", "*", "5", "5"},
                          {"5 / 5;", "/", "5", "5"},
                          {"5 > 5;", ">", "5", "5"},
                          {"5 < 5;", "<", "5", "5"},
                          {"5 == 5;", "==", "5", "5"},
                          {"5 != 5;", "!=", "5", "5"},
                          {"foobar + barfoo;", "+", "foobar", "barfoo"},
                          {"foobar - barfoo;", "-", "foobar", "barfoo"},
                          {"foobar / barfoo;", "/", "foobar", "barfoo"},
                          {"foobar * barfoo;", "*", "foobar", "barfoo"},
                          {"foobar > barfoo;", ">", "foobar", "barfoo"},
                          {"foobar < barfoo;", "<", "foobar", "barfoo"},
                          {"foobar == barfoo;", "==", "foobar", "barfoo"},
                          {"foobar != barfoo;", "!=", "foobar", "barfoo"},
                          {"true == true", "==", "true", "true"},
                          {"true != false", "!=", "true", "false"},
                          {"false == false", "==", "false", "false"},
                          {"true && false", "&&", "true", "false"},
                          {"true || false", "||", "true", "false"},
                          {"10 % 3", "%", "10", "3"}};
    print_test_separator_line();
    printf("Testing Parse Infix Expression\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input   test    = tests[i];
        lexer       *lexer   = lexer_init(test.input);
        parser      *parser  = parser_init(lexer);
        ast_program *program = parse_program(parser);
        check_parser_errors(parser);
        TEST_ASSERT_EQUAL_INT(program->statement_count, 1);

        TEST_ASSERT_EQUAL_INT(program->statements[0]->statement_type, EXPRESSION_STATEMENT);

        ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
        test_infix_expression(exp_stmt->expression, test.operator, test.left, test.right);

        program_free(program);
        parser_free(parser);
    }
}

void test_operator_precedence_parsing() {
    print_test_separator_line();
    printf("Testing operator precedence parsing\n");
    typedef struct {
        const char *input;
        const char *string;
    } test_input;

    test_input tests[] = {
            {"-a * b", "((-a) * b)"},
            {"!-a", "(!(-a))"},
            {"a + b + c", "((a + b) + c)"},
            {"a + b - c", "((a + b) - c)"},
            {"a * b * c", "((a * b) * c)"},
            {"a * b / c", "((a * b) / c)"},
            {"a + b / c", "(a + (b / c))"},
            {"a + b * c + d / e - f", "(((a + (b * c)) + (d / e)) - f)"},
            {"3 + 4; -5 * 5", "(3 + 4) ((-5) * 5)"},
            {"5 > 4 == 3 < 4", "((5 > 4) == (3 < 4))"},
            {"5 < 4 != 3 > 4", "((5 < 4) != (3 > 4))"},
            {"3 + 4 * 5 == 3 * 1 + 4 * 5", "((3 + (4 * 5)) == ((3 * 1) + (4 * 5)))"},
            {"true", "true"},
            {"false", "false"},
            {"3 > 5 == false", "((3 > 5) == false)"},
            {"3 < 5 == true", "((3 < 5) == true)"},
            {"1 + (2 + 3) + 4", "((1 + (2 + 3)) + 4)"},
            {"(5 + 5) * 2", "((5 + 5) * 2)"},
            {"2 / (5 + 5)", "(2 / (5 + 5))"},
            {"-(5 + 5)", "(-(5 + 5))"},
            {"!(true == true)", "(!(true == true))"},
            {"a + add(b * c) + d", "((a + add((b * c))) + d)"},
            {"add(a, b, 1, 2 * 3, 4 + 5, add(6, 7 *  8))", "add(a, b, 1, (2 * 3), (4 + 5), add(6, (7 * 8)))"},
            {"add(a + b + c * d / f + g)", "add((((a + b) + ((c * d) / f)) + g))"},
            {"a * [1, 2, 3, 4][b * c] * d", "((a * ([1, 2, 3, 4][(b * c)])) * d)"},
            {"add(a * b[2], b[1], 2 * [1, 2][1])", "add((a * (b[2])), (b[1]), (2 * ([1, 2][1])))"},
            {"5 > 4 && 3 > 2", "((5 > 4) && (3 > 2))"},
            {"4 < 5 || 3 > 2", "((4 < 5) || (3 > 2))"}};

    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing expression: %s\n", test.input);

        lexer  *lexer  = lexer_init(test.input);
        parser *parser = parser_init(lexer);

        ast_program *program = parse_program(parser);
        check_parser_errors(parser);
        char *actual_string = program->node.string(program);

        TEST_ASSERT_EQUAL_STRING(test.string, actual_string);

        program_free(program);
        parser_free(parser);
        if (actual_string)
            free(actual_string);
    }
}

void test_parse_prefix_expression() {
    print_test_separator_line();
    printf("Testing Parse Prefix Expression\n");

    typedef struct test_input {
        const char *input;
        const char *operator;
        const char *value;
    } test_input;

    test_input tests[] = {{"!5", "!", "5"},           {"-15", "-", "15"},     {"!foobar", "!", "foobar"},
                          {"-foobar", "-", "foobar"}, {"!true", "!", "true"}, {"!false", "!", "false"}};

    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing prefix expression: %s\n", test.input);
        lexer       *lexer   = lexer_init(test.input);
        parser      *parser  = parser_init(lexer);
        ast_program *program = parse_program(parser);
        check_parser_errors(parser);
        TEST_ASSERT_NOT_NULL(program);

        TEST_ASSERT_EQUAL_INT(program->statement_count, 1);

        TEST_ASSERT_EQUAL_INT(program->statements[0]->statement_type, EXPRESSION_STATEMENT);

        ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
        TEST_ASSERT_EQUAL_INT(exp_stmt->expression->expression_type, PREFIX_EXPRESSION);

        ast_prefix_expression *prefix_exp = (ast_prefix_expression *) exp_stmt->expression;
        TEST_ASSERT_EQUAL_STRING(prefix_exp->operator, test.operator);

        test_literal_expression(prefix_exp->right, test.value);

        program_free(program);
        parser_free(parser);
    }
    printf("Prefix expression parsing tests passed\n");
}

void test_integer_literal_expression() {
    print_test_separator_line();
    printf("Testing Integer Literal Expression\n");

    const char  *input   = "5;\n";
    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);
    check_parser_errors(parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_EQUAL_INT(program->statement_count, 1);
    TEST_ASSERT_EQUAL_INT(program->statements[0]->statement_type, EXPRESSION_STATEMENT);

    ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
    TEST_ASSERT_EQUAL_INT(exp_stmt->expression->expression_type, INTEGER_EXPRESSION);

    test_integer_literal_value(exp_stmt->expression, 5);

    program_free(program);
    parser_free(parser);
}

void test_return_statement() {
    print_test_separator_line();
    printf("Testing Return Statement\n");

    typedef struct {
        const char *input;
        const char *expected_value;
    } test_input;

    test_input tests[] = {{"return 5;", "5"}, {"return true;", "true"}, {"return foobar;", "foobar"}};

    size_t ntests = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing return statement: %s\n", test.input);
        lexer       *lexer   = lexer_init(test.input);
        parser      *parser  = parser_init(lexer);
        ast_program *program = parse_program(parser);
        check_parser_errors(parser);
        TEST_ASSERT_NOT_NULL(program);

        TEST_ASSERT_EQUAL_INT(program->statement_count, 1);

        ast_statement *stmt        = program->statements[0];
        ast_node       stmt_node   = stmt->node;
        char          *tok_literal = stmt_node.token_literal(stmt);
        TEST_ASSERT_EQUAL_STRING(tok_literal, "return");
        TEST_ASSERT_EQUAL_INT(stmt->statement_type, RETURN_STATEMENT);

        ast_return_statement *ret_stmt = (ast_return_statement *) stmt;
        test_literal_expression(ret_stmt->return_value, test.expected_value);

        program_free(program);
        parser_free(parser);
    }
}

void test_string() {
    print_test_separator_line();
    printf("Testing String\n");

    const char  *input   = "let myvar = someVar;";
    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);
    check_parser_errors(parser);
    char *program_string = program->node.string(program);

    TEST_ASSERT_EQUAL_STRING(input, program_string);

    program_free(program);
    parser_free(parser);
    free(program_string);
}

void test_boolean_expression() {
    print_test_separator_line();
    printf("Testing Boolean Expression\n");

    typedef struct {
        const char *input;
        const char *expected_value;
    } test_input;

    test_input tests[] = {{"true;", "true"}, {"false;", "false"}};

    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Parsing boolean expression: %s\n", test.input);
        lexer       *lexer   = lexer_init(test.input);
        parser      *parser  = parser_init(lexer);
        ast_program *program = parse_program(parser);
        check_parser_errors(parser);

        TEST_ASSERT_NOT_NULL(program);
        TEST_ASSERT_EQUAL_INT(program->statement_count, 1);


        TEST_ASSERT_EQUAL_INT(program->statements[0]->statement_type, EXPRESSION_STATEMENT);

        ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
        test_literal_expression(exp_stmt->expression, test.expected_value);

        program_free(program);
        parser_free(parser);
    }
}

void test_ifelse_expression(void) {
    print_test_separator_line();
    printf("Testing If/Else Expression\n");

    const char *input = "if (x < y) { x } else { y }";

    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);
    TEST_ASSERT_NOT_NULL(program);

    check_parser_errors(parser);
    TEST_ASSERT_EQUAL_INT(program->statement_count, 1);

    TEST_ASSERT_EQUAL_INT(program->statements[0]->statement_type, EXPRESSION_STATEMENT);

    ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
    ast_expression           *exp      = exp_stmt->expression;

    TEST_ASSERT_EQUAL_INT(exp->expression_type, IF_EXPRESSION);

    ast_if_expression *if_exp = (ast_if_expression *) exp;
    test_infix_expression(if_exp->condition, "<", "x", "y");


    TEST_ASSERT_EQUAL_INT(if_exp->consequence->statement_count, 1);

    TEST_ASSERT_EQUAL_INT(if_exp->consequence->statements[0]->statement_type, EXPRESSION_STATEMENT);

    ast_expression_statement *conseq_stmt = (ast_expression_statement *) if_exp->consequence->statements[0];
    test_identifier(conseq_stmt->expression, "x");


    TEST_ASSERT_EQUAL_INT(if_exp->alternative->statement_count, 1);
    TEST_ASSERT_EQUAL_INT(if_exp->alternative->statements[0]->statement_type, EXPRESSION_STATEMENT);

    ast_expression_statement *alternative_exp_stmt = (ast_expression_statement *) if_exp->alternative->statements[0];
    test_identifier(alternative_exp_stmt->expression, "y");

    program_free(program);
    parser_free(parser);
}


void test_if_expression(void) {
    print_test_separator_line();
    printf("Testing If Expression\n");

    const char *input = "if (x < y) { x }";
    printf("Testing if expression: %s\n", input);
    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);
    TEST_ASSERT_NOT_NULL(program);

    check_parser_errors(parser);
    TEST_ASSERT_EQUAL_INT(program->statement_count, 1);
    TEST_ASSERT_EQUAL_INT(program->statements[0]->statement_type, EXPRESSION_STATEMENT);

    ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
    ast_expression           *exp      = exp_stmt->expression;

    TEST_ASSERT_EQUAL_INT(exp->expression_type, IF_EXPRESSION);

    ast_if_expression *if_exp = (ast_if_expression *) exp;
    test_infix_expression(if_exp->condition, "<", "x", "y");

    TEST_ASSERT_EQUAL_INT(if_exp->consequence->statement_count, 1);

    TEST_ASSERT_EQUAL_INT(if_exp->consequence->statements[0]->statement_type, EXPRESSION_STATEMENT);

    ast_expression_statement *conseq_stmt = (ast_expression_statement *) if_exp->consequence->statements[0];
    test_identifier(conseq_stmt->expression, "x");

    TEST_ASSERT_NULL(if_exp->alternative);

    program_free(program);
    parser_free(parser);
}

void test_function_literal(void) {
    const char *input = "fn(x, y) { x + y; }";
    print_test_separator_line();
    printf("Testing Function Literal: %s\n", input);
    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);

    TEST_ASSERT_NOT_NULL(program);

    check_parser_errors(parser);

    TEST_ASSERT_EQUAL_INT(program->statement_count, 1);

    TEST_ASSERT_EQUAL_INT(program->statements[0]->statement_type, EXPRESSION_STATEMENT);

    ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
    TEST_ASSERT_EQUAL_INT(exp_stmt->expression->expression_type, FUNCTION_LITERAL);

    ast_function_literal *function = (ast_function_literal *) exp_stmt->expression;
    TEST_ASSERT_EQUAL_INT(function->parameters->size, 2);

    test_literal_expression((ast_expression *) function->parameters->head->data, "x");
    test_literal_expression((ast_expression *) function->parameters->head->next->data, "y");

    TEST_ASSERT_EQUAL_INT(function->body->statement_count, 1);

    TEST_ASSERT_EQUAL_INT(function->body->statements[0]->statement_type, EXPRESSION_STATEMENT);

    ast_expression_statement *body_exp_statement = (ast_expression_statement *) function->body->statements[0];
    test_infix_expression(body_exp_statement->expression, "+", "x", "y");

    program_free(program);
    parser_free(parser);
}

void test_function_parameter_parsing(void) {
    typedef struct {
        const char *input;
        size_t      nparams;
        const char *expected_params[3];
    } test_input;

    test_input tests[] = {{"fn () {};", 0, {NULL, NULL, NULL}},
                          {"fn (x) {};", 1, {"x", NULL, NULL}},
                          {"fn (x, y, z) {};", 3, {"x", "y", "z"}}};
    size_t     ntests  = sizeof(tests) / sizeof(tests[0]);
    print_test_separator_line();

    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing function parameter parsing for: %s\n", test.input);
        lexer       *lexer   = lexer_init(test.input);
        parser      *parser  = parser_init(lexer);
        ast_program *program = parse_program(parser);

        TEST_ASSERT_NOT_NULL(program);

        check_parser_errors(parser);

        ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
        ast_function_literal     *function = (ast_function_literal *) exp_stmt->expression;
        TEST_ASSERT_EQUAL_INT(function->parameters->size, test.nparams);

        list_node *list_node = function->parameters->head;
        for (size_t n = 0; n < test.nparams; n++) {
            ast_identifier *param = (ast_identifier *) list_node->data;
            test_literal_expression((ast_expression *) param, test.expected_params[n]);
            list_node = list_node->next;
        }

        program_free(program);
        parser_free(parser);
    }
}

void test_call_expression_parsing(void) {
    const char *input = "add(1, 2 * 3, 4 + 5);";
    print_test_separator_line();
    printf("Testing call expression parsing\n");
    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);

    TEST_ASSERT_NOT_NULL(program);

    check_parser_errors(parser);

    TEST_ASSERT_EQUAL_INT(program->statement_count, 1);

    TEST_ASSERT_EQUAL_INT(program->statements[0]->statement_type, EXPRESSION_STATEMENT);

    ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
    TEST_ASSERT_EQUAL_INT(exp_stmt->expression->expression_type, CALL_EXPRESSION);

    ast_call_expression *call_exp = (ast_call_expression *) exp_stmt->expression;
    TEST_ASSERT_EQUAL_INT(call_exp->function->expression_type, IDENTIFIER_EXPRESSION);

    test_identifier(call_exp->function, "add");

    TEST_ASSERT_EQUAL_INT(call_exp->arguments->size, 3);

    test_literal_expression((ast_expression *) call_exp->arguments->head->data, "1");
    test_infix_expression((ast_expression *) call_exp->arguments->head->next->data, "*", "2", "3");
    test_infix_expression((ast_expression *) call_exp->arguments->head->next->next->data, "+", "4", "5");

    program_free(program);
    parser_free(parser);
}

void test_call_expression_argument_parsing(void) {
    print_test_separator_line();
    printf("Testing Call Expression Argument Passing\n");

    typedef struct {
        const char *input;
        const char *expected_ident;
        size_t      nargs;
        const char *expected_args[3];
    } test_input;

    test_input tests[] = {{"add();", "add", 0, {NULL, NULL, NULL}},
                          {"add(1);", "add", 1, {"1", NULL, NULL}},
                          {"add(1, 2 * 3, 4 + 5);", "add", 3, {"1", "(2 * 3)", "(4 + 5)"}}};
    size_t     ntests  = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < ntests; i++) {
        test_input   test    = tests[i];
        lexer       *lexer   = lexer_init(test.input);
        parser      *parser  = parser_init(lexer);
        ast_program *program = parse_program(parser);

        TEST_ASSERT_NOT_NULL(program);

        check_parser_errors(parser);

        ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
        TEST_ASSERT_EQUAL_INT(exp_stmt->expression->expression_type, CALL_EXPRESSION);

        ast_call_expression *call_exp = (ast_call_expression *) exp_stmt->expression;
        test_identifier(call_exp->function, test.expected_ident);
        TEST_ASSERT_EQUAL_INT(call_exp->arguments->size, test.nargs);

        list_node *list_node = call_exp->arguments->head;
        for (size_t j = 0; j < test.nargs; j++) {
            ast_expression *arg        = (ast_expression *) list_node->data;
            char           *arg_string = arg->node.string(arg);

            TEST_ASSERT_EQUAL_STRING(arg_string, test.expected_args[j]);

            free(arg_string);
            list_node = list_node->next;
        }
        program_free(program);
        parser_free(parser);
    }
}

static void test_function_call_one_argument(void) {
    const char *input = "let oneArg = fn(a) {a}; oneArg(24);";
    print_test_separator_line();
    printf("Testing Function Call Argument Parsing: %s\n", input);
    lexer       *lexer   = lexer_init(input);
    parser            *parser  = parser_init(lexer);
    const ast_program *program = parse_program(parser);

    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_EQUAL_INT(program->statement_count, 2);
    ast_statement *stmt = program->statements[0];
    TEST_ASSERT_EQUAL_INT(stmt->statement_type, LET_STATEMENT);

    ast_call_expression *call_exp = (ast_call_expression *) ((ast_expression_statement *) program->statements[1])->expression;
    ast_node *arg = (ast_node *) linked_list_get_at(call_exp->arguments, 0);
    TEST_ASSERT_EQUAL(arg->type, INTEGER_EXPRESSION);

    free(lexer);
    free(parser);
    free(program);
}

void test_string_literal(void) {
    const char  *input   = "\"hello, world!\"";
    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);
    print_test_separator_line();
    printf("Testing Integer Literal Expression: %s\n", input);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_EQUAL_INT(program->statement_count, 1);
    TEST_ASSERT_EQUAL_INT(program->statements[0]->statement_type, EXPRESSION_STATEMENT);

    ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
    TEST_ASSERT_EQUAL_INT(exp_stmt->expression->expression_type, STRING_EXPRESSION);

    ast_string *string = (ast_string *) exp_stmt->expression;
    TEST_ASSERT_EQUAL_STRING(string->value, "hello, world!");

    program_free(program);
    parser_free(parser);
}

void test_parse_array_literal(void) {
    const char  *input   = "[1, 2 * 2,  3 + 3]";
    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);
    check_parser_errors(parser);
    print_test_separator_line();
    printf("Testing Parse Array Literal: %s\n", input);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_EQUAL_INT(program->statements[0]->statement_type, EXPRESSION_STATEMENT);

    ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
    TEST_ASSERT_EQUAL_INT(exp_stmt->expression->expression_type, ARRAY_LITERAL);

    ast_array_literal *array = (ast_array_literal *) exp_stmt->expression;

    TEST_ASSERT_EQUAL_INT(array->elements->size, 3);

    test_integer_literal_value((ast_expression *) array->elements->body[0], 1);
    test_infix_expression((ast_expression *) array->elements->body[1], "*", "2", "2");
    test_infix_expression((ast_expression *) array->elements->body[2], "+", "3", "3");

    program_free(program);
    parser_free(parser);
}

void test_parse_index_expression(void) {
    const char *input = "my_array[1 + 1]";
    print_test_separator_line();
    printf("Testing index expression parsing: %s\n", input);
    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);

    TEST_ASSERT_NOT_NULL(program);
    check_parser_errors(parser);

    TEST_ASSERT_EQUAL_INT(program->statements[0]->statement_type, EXPRESSION_STATEMENT);

    ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
    TEST_ASSERT_EQUAL_INT(exp_stmt->expression->expression_type, INDEX_EXPRESSION);

    ast_index_expression *index_exp = (ast_index_expression *) exp_stmt->expression;

    test_identifier(index_exp->left, "my_array");
    test_infix_expression(index_exp->index, "+", "1", "1");

    program_free(program);
    parser_free(parser);
}

void test_parse_hash_literals(void) {
    const char *input    = "{\"one\": 1, \"two\": 2, \"three\": 3}";
    hashtable  *expected = hashtable_create(string_hash_function, string_equals, NULL, NULL);
    int         one      = 1;
    int         two      = 2;
    int         three    = 3;
    hashtable_set(expected, "one", &one);
    hashtable_set(expected, "two", &two);
    hashtable_set(expected, "three", &three);
    print_test_separator_line();

    printf("Testing hash literal parsing: %s\n", input);

    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);

    TEST_ASSERT_NOT_NULL(program);
    check_parser_errors(parser);

    TEST_ASSERT_EQUAL_INT(program->statements[0]->statement_type, EXPRESSION_STATEMENT);

    ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
    TEST_ASSERT_EQUAL_INT(exp_stmt->expression->expression_type, HASH_LITERAL);

    ast_hash_literal *hash_exp = (ast_hash_literal *) exp_stmt->expression;
    TEST_ASSERT_EQUAL_INT(hash_exp->pairs->key_count, 3);

    for (size_t i = 0; i < hash_exp->pairs->used_slots->size; i++) {
        size_t      *index      = (size_t *) hash_exp->pairs->used_slots->body[i];
        linked_list *entry_list = hash_exp->pairs->table[*index];
        TEST_ASSERT_NOT_NULL(entry_list);

        list_node *node = entry_list->head;
        while (node != NULL) {
            hashtable_entry *entry          = (hashtable_entry *) node->data;
            ast_expression  *key            = (ast_expression *) entry->key;
            int             *expected_value = hashtable_get(expected, ((ast_string *) key)->value);
            TEST_ASSERT_NOT_NULL(expected_value);
            ast_integer *actual_value = (ast_integer *) entry->value;
            test_integer_literal_value((ast_expression *) actual_value, *expected_value);
            node = node->next;
        }
    }
    program_free(program);
    parser_free(parser);
    hashtable_destroy(expected);
}

void test_parsing_empty_hash_literal(void) {
    const char *input = "{}";
    print_test_separator_line();
    printf("Testing parsing of empty hash literal\n");
    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);
    check_parser_errors(parser);
    ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
    TEST_ASSERT_EQUAL_INT(exp_stmt->expression->expression_type, HASH_LITERAL);

    ast_hash_literal *hash_exp = (ast_hash_literal *) exp_stmt->expression;

    TEST_ASSERT_EQUAL_INT(hash_exp->pairs->key_count, 0);

    program_free(program);
    parser_free(parser);
}

void test_parsing_hash_literal_bool_keys(void) {
    const char *input = "{true: 1, false: 2}";
    print_test_separator_line();
    printf("Testing parsing of hash literals with boolean keys\n");

    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);

    check_parser_errors(parser);

    ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
    TEST_ASSERT_EQUAL_INT(exp_stmt->expression->expression_type, HASH_LITERAL);

    ast_hash_literal *hash_exp = (ast_hash_literal *) exp_stmt->expression;

    size_t visited_key_count = 0;

    for (size_t i = 0; i < hash_exp->pairs->used_slots->size; i++) {
        size_t      *index  = (size_t *) hash_exp->pairs->used_slots->body[i];
        linked_list *bucket = hash_exp->pairs->table[*index];
        list_node   *node   = bucket->head;

        while (node) {
            hashtable_entry *entry = (hashtable_entry *) node->data;

            ast_expression *key_exp = (ast_expression *) entry->key;
            TEST_ASSERT_EQUAL_INT(key_exp->expression_type, BOOLEAN_EXPRESSION);

            ast_expression *value_exp = (ast_expression *) entry->value;
            TEST_ASSERT_EQUAL_INT(value_exp->expression_type, INTEGER_EXPRESSION);

            ast_boolean_expression *bool_key  = (ast_boolean_expression *) key_exp;
            ast_integer            *int_value = (ast_integer *) value_exp;
            if (bool_key->value) {
                TEST_ASSERT_EQUAL_INT(int_value->value, 1);
            } else {
                TEST_ASSERT_EQUAL_INT(int_value->value, 2);
            }

            node = node->next;
            visited_key_count++;
        }
    }

    TEST_ASSERT_EQUAL_INT(hash_exp->pairs->key_count, visited_key_count);

    program_free(program);
    parser_free(parser);
}

void free_expected_value(void *value) { free(value); }

void test_parsing_hash_literal_with_expression_values(void) {
    print_test_separator_line();
    printf("Testing parsing of hash literal with expressions in values\n");

    typedef struct {
        const char *operator;
        const char *left;
        const char *right;
    } expected_value;

    const char *input = "{\"one\": 0 + 1, \"two\": 10 - 8, \"three\": 15 / 5}";

    hashtable      *expected = hashtable_create(string_hash_function, string_equals, NULL, free_expected_value);
    expected_value *one      = malloc(sizeof(expected_value));
    expected_value *two      = malloc(sizeof(expected_value));
    expected_value *three    = malloc(sizeof(expected_value));
    *one                     = (expected_value) {"+", "0", "1"};
    *two                     = (expected_value) {"-", "10", "8"};
    *three                   = (expected_value) {"/", "15", "5"};
    hashtable_set(expected, "one", one);
    hashtable_set(expected, "two", two);
    hashtable_set(expected, "three", three);

    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);

    check_parser_errors(parser);

    ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];

    TEST_ASSERT_EQUAL_INT(exp_stmt->expression->expression_type, HASH_LITERAL);

    ast_hash_literal *hash_exp = (ast_hash_literal *) exp_stmt->expression;
    TEST_ASSERT_EQUAL_INT(hash_exp->pairs->key_count, 3);

    for (size_t i = 0; i < hash_exp->pairs->used_slots->size; i++) {
        size_t          *index = (size_t *) hash_exp->pairs->used_slots->body[i];
        hashtable_entry *entry = (hashtable_entry *) hash_exp->pairs->table[*index]->head->data;
        ast_expression  *key   = (ast_expression *) entry->key;
        TEST_ASSERT_EQUAL_INT(key->expression_type, STRING_EXPRESSION);

        ast_string     *string_exp = (ast_string *) key;
        ast_expression *value_exp  = (ast_expression *) entry->value;
        expected_value *exp_value  = (expected_value *) hashtable_get(expected, string_exp->value);

        TEST_ASSERT_NOT_NULL(exp_value);

        test_infix_expression(value_exp, exp_value->operator, exp_value->left, exp_value->right);
    }
    program_free(program);
    parser_free(parser);
    hashtable_destroy(expected);
}

void test_parsing_hash_literal_with_integer_keys(void) {
    hashtable *expected = hashtable_create(string_hash_function, string_equals, NULL, NULL);
    hashtable_set(expected, "1", ((long[]) {1}));
    hashtable_set(expected, "2", ((long[]) {2}));
    hashtable_set(expected, "3", ((long[]) {3}));
    const char *input = "{1: 1, 2: 2, 3:3}";
    print_test_separator_line();
    printf("Testing hash literal parsing with integer keys\n");
    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);
    check_parser_errors(parser);
    ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
    TEST_ASSERT_EQUAL_INT(exp_stmt->expression->expression_type, HASH_LITERAL);

    ast_hash_literal *hash_exp = (ast_hash_literal *) exp_stmt->expression;
    TEST_ASSERT_EQUAL_INT(hash_exp->pairs->key_count, 3);

    for (size_t i = 0; i < hash_exp->pairs->used_slots->size; i++) {
        size_t          *index = (size_t *) hash_exp->pairs->used_slots->body[i];
        hashtable_entry *entry = (hashtable_entry *) hash_exp->pairs->table[*index]->head->data;

        TEST_ASSERT_NOT_NULL(entry);

        ast_expression *key_exp        = (ast_expression *) entry->key;
        char           *string_key     = key_exp->node.string(key_exp);
        long           *expected_value = hashtable_get(expected, string_key);
        test_integer_literal_value(key_exp, expected_value[0]);
        free(string_key);
    }
    parser_free(parser);
    program_free(program);
    hashtable_destroy(expected);
}

void test_function_literal_with_name(void) {
    const char *input = "let myfn = fn() {};";
    print_test_separator_line();
    printf("Testing function literal with name: %s\n", input);

    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);

    check_parser_errors(parser);

    TEST_ASSERT_EQUAL_INT(program->statement_count, 1);
    TEST_ASSERT_EQUAL_INT(program->statements[0]->statement_type, LET_STATEMENT);

    ast_let_statement *letstmt = (ast_let_statement *) program->statements[0];
    TEST_ASSERT_EQUAL_INT(letstmt->value->expression_type, FUNCTION_LITERAL);

    ast_function_literal *fn_literal = (ast_function_literal *) letstmt->value;
    TEST_ASSERT_EQUAL_STRING(fn_literal->name, "myfn");

    parser_free(parser);
    program_free(program);
}

void test_parsing_while_expression(void) {
    const char *input = "while (x > 2) {\n"
                        "   let x = x - 1;\n"
                        "   x;\n"
                        "}";
    print_test_separator_line();
    printf("Testing while expression parsing for: %s\n", input);
    lexer       *lexer   = lexer_init(input);
    parser      *parser  = parser_init(lexer);
    ast_program *program = parse_program(parser);
    check_parser_errors(parser);

    TEST_ASSERT_EQUAL_INT(program->statement_count, 1);
    TEST_ASSERT_EQUAL_INT(program->statements[0]->statement_type, EXPRESSION_STATEMENT);

    ast_expression_statement *exp_stmt = (ast_expression_statement *) program->statements[0];
    TEST_ASSERT_EQUAL_INT(exp_stmt->expression->expression_type, WHILE_EXPRESSION);

    ast_while_expression *while_exp = (ast_while_expression *) exp_stmt->expression;
    test_infix_expression(while_exp->condition, ">", "x", "2");
    TEST_ASSERT_EQUAL_INT(while_exp->body->statement_count, 2);

    TEST_ASSERT_EQUAL_INT(while_exp->body->statements[0]->statement_type, LET_STATEMENT);
    TEST_ASSERT_EQUAL_INT(while_exp->body->statements[1]->statement_type, EXPRESSION_STATEMENT);

    program_free(program);
    parser_free(parser);
}

int main() {
    UNITY_BEGIN();
    // RUN_TEST(test_let_stmt);
    // RUN_TEST(test_return_statement);
    // RUN_TEST(test_identifier_expression);
    // RUN_TEST(test_integer_literal_expression);
    // RUN_TEST(test_parse_prefix_expression);
    // RUN_TEST(test_parse_infix_expression);
    // RUN_TEST(test_operator_precedence_parsing);
    // RUN_TEST(test_string);
    // RUN_TEST(test_boolean_expression);
    // RUN_TEST(test_if_expression);
    // RUN_TEST(test_ifelse_expression);
    // RUN_TEST(test_function_literal);
    // RUN_TEST(test_function_parameter_parsing);
    // RUN_TEST(test_call_expression_parsing);
    // RUN_TEST(test_call_expression_argument_parsing);
    RUN_TEST(test_function_call_one_argument);
    // RUN_TEST(test_string_literal);
    // RUN_TEST(test_parse_array_literal);
    // RUN_TEST(test_parse_index_expression);
    // RUN_TEST(test_parse_hash_literals);
    // RUN_TEST(test_parsing_empty_hash_literal);
    // RUN_TEST(test_parsing_hash_literal_with_expression_values);
    // RUN_TEST(test_parsing_hash_literal_with_integer_keys);
    // RUN_TEST(test_parsing_hash_literal_bool_keys);
    // RUN_TEST(test_parsing_while_expression);
    // RUN_TEST(test_function_literal_with_name);
    return UNITY_END();
}
