#include "test_lexer.h"
#ifndef ARENA_IMPLEMENTATION
#define ARENA_IMPLEMENTATION
#include "arena.h"
#endif

// TODO: Implement propper logging

// Function prototypes for setup and teardown
void setUp(void) {
  // This function is called before each test
}

void tearDown(void) {
  // This function is called after each test
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_lexer_identifier);
  // RUN_TEST(test_lexer_numeric);
  // RUN_TEST(test_lexer_operator);
  // RUN_TEST(test_lexer_keyword);
  // RUN_TEST(test_lexer_delimiter);
  // RUN_TEST(test_lexer_newline);
  // RUN_TEST(test_lexer_square_brackets);
  // RUN_TEST(test_lexer_endmarker);
  return UNITY_END();
}