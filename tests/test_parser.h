#pragma once
#ifndef TEST_PARSER_H_
#define TEST_PARSER_H_

#include "parser.h"
#include "utils.h"
#include <unity.h>

void test_parser_single_number(void) {
  Lexer lexer = tokenize("123");
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);
  TEST_ASSERT_EQUAL_INT(LITERAL, node->type);
  TEST_ASSERT_EQUAL_STRING("123", node->token->lexeme);
  ASTNode_free(&parser.ast);
  Token_free(&lexer.tokens);
}

void test_parse_arithmetic_expression(void) {
  Lexer lexer = tokenize("3 + 5 * 2");
  Parser parser = parse(&lexer);
  ASTNode *result = ASTNode_pop(&parser.ast);
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("+", result->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("3", result->bin_op.left->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("*", result->bin_op.right->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("5",
                           result->bin_op.right->bin_op.left->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("2",
                           result->bin_op.right->bin_op.right->token->lexeme);
  ASTNode_free(&parser.ast);
  Token_free(&lexer.tokens);
}

// Test parsing a variable assignment
void test_parse_variable_assignment(void) {
  Lexer lexer = tokenize("x = 42");
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);
  ASTNode *target = ASTNode_pop(&node->assign.targets);
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(ASSIGNMENT, node->type);
  TEST_ASSERT_EQUAL_INT(VARIABLE, target->type);
  TEST_ASSERT_EQUAL_INT(LITERAL, node->assign.value->type);
  TEST_ASSERT_EQUAL_STRING("=", node->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("x", target->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("42", node->assign.value->token->lexeme);
  Token_free(&lexer.tokens);
  ASTNode_free(&node->assign.targets);
  ASTNode_free(&parser.ast);
}

void test_parse_multiple_variable_assignment(void) {
  Lexer lexer = tokenize("x, y, z = 42");
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);

  for (size_t i = 0; i < node->assign.targets.size; i++) {
    ASTNode *el = ASTNode_get(&node->assign.targets, i);
    TEST_ASSERT_EQUAL_INT(VARIABLE, el->type);
  }

  ASTNode *target = ASTNode_pop(&node->assign.targets);
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(ASSIGNMENT, node->type);
  TEST_ASSERT_EQUAL_INT(LITERAL, node->assign.value->type);
  TEST_ASSERT_EQUAL_STRING("=", node->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("x", target->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("42", node->assign.value->token->lexeme);
  Token_free(&lexer.tokens);
  ASTNode_free(&node->assign.targets);
  ASTNode_free(&parser.ast);
}

#endif