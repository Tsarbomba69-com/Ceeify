#include "test_codegen.h"
#include "test_lexer.h"
#include "test_parser.h"
#include "test_semantic.h"
#include "test_tac.h"
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
  RUN_TEST(test_lexer_rarrow);
  // Parser
  RUN_TEST(test_parser_single_number);
  RUN_TEST(test_parse_arithmetic_expression);
  RUN_TEST(test_expression_parentheses_override_precedence);
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
  RUN_TEST(test_parse_function_declaration);
  RUN_TEST(test_parse_function_call);
  RUN_TEST(test_parse_function_call_no_args);
  RUN_TEST(test_parse_nested_function_call);
  RUN_TEST(test_parse_call_inside_expression);
  RUN_TEST(test_parse_annotated_assignment);
  RUN_TEST(test_parse_function_def_with_annotations);
  RUN_TEST(test_parse_class);
  // Semantic
  RUN_TEST(test_semantic_empty_program);
  RUN_TEST(test_semantic_simple_assignment);
  RUN_TEST(test_semantic_undefined_variable);
  RUN_TEST(test_semantic_type_mismatch);
  RUN_TEST(test_semantic_invalid_operation);
  RUN_TEST(test_semantic_function_declaration);
  RUN_TEST(test_semantic_undefined_variable_in_function);
  RUN_TEST(test_semantic_reassignment_type_mismatch);
  RUN_TEST(test_semantic_function_call_arity_mismatch);
  RUN_TEST(test_semantic_function_call_type_mismatch);
  RUN_TEST(test_semantic_function_return_type_mismatch);
  RUN_TEST(test_semantic_function_return_type_ok);
  RUN_TEST(test_func_type_error_int_str);
  RUN_TEST(test_semantic_class_inheritance_and_init);
  // Three-address code (TAC)
  RUN_TEST(test_tac_simple_assignment);
  RUN_TEST(test_tac_binary_expression);
  RUN_TEST(test_tac_variable_to_variable_assignment);
  RUN_TEST(test_tac_variable_reassignment);
  RUN_TEST(test_tac_expression_with_variable);
  RUN_TEST(test_tac_multiple_statements);
  RUN_TEST(test_tac_unary_minus);
  RUN_TEST(test_tac_if_statement_no_else);
  RUN_TEST(test_tac_operator_precedence);
  RUN_TEST(test_tac_parenthesized_expression);
  RUN_TEST(test_tac_if_else_statement);
  // Codegen (Python -> C)
  RUN_TEST(test_codegen_function_return_literal);
  RUN_TEST(test_codegen_function_call);
  RUN_TEST(test_codegen_class_inheritance_and_init);
  RUN_TEST(test_codegen_if_else_statement);
  RUN_TEST(test_codegen_while_loop);
  return UNITY_END();
}

int main2(void) {
  slog_init("ceeify", SLOG_FLAGS_ALL, 0);
  DEFER(cleanup) slog_config_t cfg;
  slog_config_get(&cfg);
  cfg.eDateControl = SLOG_DATE_FULL;
  cfg.nTraceTid = true;
  cfg.nToFile = true;
  cfg.nKeepOpen = true;
  slog_config_set(&cfg);
  UNITY_BEGIN();
  RUN_TEST(test_codegen_class_inheritance_and_init);
  return UNITY_END();
}
