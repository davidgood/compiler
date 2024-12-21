//
// Created by dgood on 12/15/24.
//
#include <stdint.h>
#include "../../Unity/src/unity.h"
#include "../src/lexer/lexer.h"
#include "../src/token/token.h"

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_next_token(void) {
	char* input = "let five = 5; \n"\
			 "let ten = 10;"\
			 "\n"\
			 "\n"\
			 "let add = fn(x, y) {\n"\
			 "	x + y;\n"\
			 "};\n"\
			 "\n"\
			 "let result = add(five, ten);\n"\
			 "!-/*5;\n"\
			 "5 < 10 > 5;\n"\
			 "if (5 < 10 ) {\n"\
			 "	return true;\n"\
			 "} else {\n"\
			 "	return false;\n"\
			 "}\n"\
			 "\n"\
			 "10 == 10;\n"\
			 "10 != 9;\n"\
			 "!5;\n"\
			 "return 10; 10;\n"\
			 "\"foobar\"\n"\
			 "\"foo bar\"\n"\
			 "[1, 2];\n"\
			 "{\"foo\": \"bar\"};\n"\
			 "true && false;\n"\
			 "!true;\n"\
			 "x == y || x > z;\n"\
			 "while (x > 10) {\n"\
			 "let x = x - 1;\n"\
			 "}\n"\
			 "x % y;\n";

	token tests[] = {
		{ LET, "let"},
		{ IDENT, "five"},
		{ ASSIGN, "="},
		{ INT, "5"},
		{ SEMICOLON, ";"},
		{ LET, "let"},
		{ IDENT, "ten"},
		{ ASSIGN, "="},
		{ INT, "10" },
		{ SEMICOLON, ";" },
		{ LET, "let" },
		{ IDENT, "add" },
		{ ASSIGN, "=" },
		{ FUNCTION, "fn" },
		{ LPAREN, "(" },
		{ IDENT, "x" },
		{ COMMA, "," },
		{ IDENT, "y" },
		{ RPAREN, ")" },
		{ LBRACE, "{" },
		{ IDENT, "x" },
		{ PLUS, "+" },
		{ IDENT, "y" },
		{ SEMICOLON, ";" },
		{ RBRACE, "}" },
		{ SEMICOLON, ";" },
		{ LET, "let" },
		{ IDENT, "result" },
		{ ASSIGN, "=" },
		{ IDENT, "add" },
		{ LPAREN, "(" },
		{ IDENT, "five" },
		{ COMMA, "," },
		{ IDENT, "ten" },
		{ RPAREN, ")" },
		{ SEMICOLON, ";" },
		{ BANG, "!" },
		{ MINUS, "-" },
		{ SLASH, "/" },
		{ ASTERISK, "*" },
		{ INT, "5" },
		{ SEMICOLON, ";"},
		{ INT, "5" },
		{ LT, "<" },
		{ INT, "10" },
		{ GT, ">" },
		{ INT, "5" },
		{ SEMICOLON, ";" },
		{ IF, "if" },
		{ LPAREN, "(" },
		{ INT, "5" },
		{ LT, "<" },
		{ INT, "10" },
		{ RPAREN, ")" },
		{ LBRACE, "{" },
		{ RETURN, "return" },
		{ TRUE, "true" },
		{ SEMICOLON, ";" },
		{ RBRACE, "}" },
		{ ELSE, "else" },
		{ LBRACE, "{" },
		{ RETURN, "return" },
		{ FALSE, "false" },
		{ SEMICOLON, ";" },
		{ RBRACE, "}" },
		{ INT, "10" },
		{ EQ, "==" },
		{ INT, "10" },
		{ SEMICOLON, ";" },
		{ INT, "10" },
		{ NOT_EQ, "!=" },
		{ INT, "9" },
		{ SEMICOLON, ";" },
		{ BANG, "!"},
		{ INT, "5"},
		{ SEMICOLON, ";"},
		{ RETURN, "return"},
		{ INT, "10"},
		{ SEMICOLON, ";"},
		{ INT, "10"},
		{ SEMICOLON, ";"},
		{ STRING, "foobar"},
		{ STRING, "foo bar"},
		{ LBRACKET, "["},
		{ INT, "1"},
		{ COMMA, ","},
		{ INT, "2"},
		{ RBRACKET, "]"},
		{ SEMICOLON, ";"},
		{ LBRACE, "{"},
		{ STRING, "foo"},
		{ COLON, ":"},
		{ STRING, "bar"},
		{ RBRACE, "}"},
		{ SEMICOLON, ";"},
		{ TRUE, "true"},
		{ AND, "&&"},
		{ FALSE, "false"},
		{ SEMICOLON, ";"},
		{ BANG, "!"},
		{ TRUE, "true"},
		{ SEMICOLON, ";"},
		{ IDENT, "x"},
		{ EQ, "=="},
		{ IDENT, "y"},
		{ OR, "||"},
		{ IDENT, "x"},
		{ GT, ">"},
		{ IDENT, "z"},
		{ SEMICOLON, ";"},
		{ WHILE, "while"},
		{ LPAREN, "("},
		{ IDENT, "x"},
		{ GT, ">"},
		{ INT, "10"},
		{ RPAREN, ")"},
		{ LBRACE, "{"},
		{ LET, "let"},
		{ IDENT, "x"},
		{ ASSIGN, "="},
		{ IDENT, "x"},
		{ MINUS, "-"},
		{ INT, "1"},
		{ SEMICOLON, ";"},
		{ RBRACE, "}"},
		{ IDENT, "x"},
		{ PERCENT, "%"},
		{ IDENT, "y"},
		{ SEMICOLON, ";"},
		{ END_OF_FILE, "" }
	};

	lexer *l = lexer_init(input);
	int i = 0;
	token *t;
	for (i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
		t = lexer_next_token(l);
		printf("Testing lexing for input %s\n", tests[i].literal);
		TEST_ASSERT_EQUAL_STRING(&t->type, &tests[i].type);
		TEST_ASSERT_EQUAL_STRING(t->literal, tests[i].literal);
		token_free(t);
	}
	lexer_free(l);
}

// not needed when using generate_test_runner.rb
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_next_token);
    return UNITY_END();
}
