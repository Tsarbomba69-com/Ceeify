#pragma once
#ifndef TEST_PARSER_H_
#define TEST_PARSER_H_

#include "parser.h"
#include "utils.h"
#include <unity.h>

void test_parser_single_number(void) {
  Lexer lexer = tokenize("123");
  Parser parser = parse(&lexer);
  ASTNode *node = Node_pop(&parser.ast);
  TEST_ASSERT_EQUAL_INT(LITERAL, node->type);
  TEST_ASSERT_EQUAL_STRING("123", node->token->lexeme);
  arena_free(&parser.ast.allocator);
  arena_free(&lexer.tokens.allocator);
}

void test_parse_arithmetic_expression(void) {
  Lexer lexer = tokenize("3 + 5 * 2");
  Parser parser = parse(&lexer);
  ASTNode *result = Node_pop(&parser.ast);
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("+", result->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("3", result->bin_op.left->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("*", result->bin_op.right->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("5",
                        result->bin_op.right->bin_op.left->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("2",
                        result->bin_op.right->bin_op.right->token->lexeme);
  arena_free(&parser.ast.allocator);
  arena_free(&lexer.tokens.allocator);
}

#endif