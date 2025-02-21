#include "test_lexer.h"
#include "test_parser.h"
#ifndef ARENA_IMPLEMENTATION
#define ARENA_IMPLEMENTATION
#include "arena.h"
#endif

// Function prototypes for setup and teardown
void setUp(void) {
  // This function is called before each test
}

void tearDown(void) {
  // This function is called after each test
}

int main(void) {
  UNITY_BEGIN();
  // Lexer
  RUN_TEST(test_lexer_identifier);
  RUN_TEST(test_lexer_numeric);
  RUN_TEST(test_lexer_operator);
  RUN_TEST(test_lexer_keyword);
  RUN_TEST(test_lexer_delimiter);
  RUN_TEST(test_lexer_newline);
  RUN_TEST(test_lexer_square_brackets);
  RUN_TEST(test_lexer_endmarker);
  // Parser
  RUN_TEST(test_parser_single_number);
  RUN_TEST(test_parse_arithmetic_expression);
  RUN_TEST(test_parse_variable_assignment);
  RUN_TEST(test_parse_multiple_variable_assignment);
  RUN_TEST(test_import_assignment);
  return UNITY_END();
}