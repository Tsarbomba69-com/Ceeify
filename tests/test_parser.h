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
  arena_free(&parser.allocator);
  arena_free(&lexer.tokens.allocator);
  free(parser.ast.head);
}

#endif