#include "test_lexer.h"
#include "test_parser.h"
#include "test_semantic.h"
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

void cleanup(void *p) {
  (void)p;
  slog_destroy();
}

int main(void) {
    slog_init("ceeify", SLOG_FLAGS_ALL, 0);
  DEFER(cleanup) slog_config_t cfg;
  slog_config_get(&cfg);
  cfg.eDateControl = SLOG_DATE_FULL;
  cfg.nTraceTid = true;
  cfg.nToFile = true;
  cfg.nKeepOpen = true;
  slog_config_set(&cfg);
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
  RUN_TEST(test_lexer_augassign);
  // Parser
  RUN_TEST(test_parser_single_number);
  RUN_TEST(test_parse_arithmetic_expression);
  RUN_TEST(test_parse_variable_assignment);
  RUN_TEST(test_parse_multiple_variable_assignment);
  RUN_TEST(test_import_assignment);
  RUN_TEST(test_compare_expression);
  RUN_TEST(test_if_statement);
  RUN_TEST(test_if_elif_statement);
  RUN_TEST(test_if_else_statement);
  RUN_TEST(test_while_statement);
  RUN_TEST(test_while_else_statement);
  RUN_TEST(test_parse_augmented_assignment);
  // Semantic
  RUN_TEST(test_semantic_empty_program);
  RUN_TEST(test_semantic_simple_assignment);
  RUN_TEST(test_semantic_undefined_variable);
  return UNITY_END();
}