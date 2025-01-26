#pragma once
#ifndef TEST_LEXER_H_
#define TEST_LEXER_H_

#include "lexer.h"
#include "utils.h"
#include <unity.h>

void test_lexer_identifier(void) {
  Lexer lexer = tokenize("my_variable = 10");
  Token *token = next_token(&lexer);
  TEST_ASSERT_EQUAL(IDENTIFIER, token->type);
  TEST_ASSERT_EQUAL_STRING("my_variable", token->lexeme);
  TEST_ASSERT_EQUAL(1, token->line);
  TEST_ASSERT_EQUAL(1, token->col);
  arena_free(&lexer.tokens.allocator);
}

void test_lexer_numeric(void) {
  Lexer lexer = tokenize("123");
  Token *token = next_token(&lexer);
  // Assertions
  TEST_ASSERT_EQUAL_INT(NUMERIC, token->type);
  TEST_ASSERT_EQUAL_STRING("123", token->lexeme);
  arena_free(&lexer.tokens.allocator);
}

void test_lexer_operator(void) {
  Lexer lexer = tokenize("+");
  Token *token = next_token(&lexer);
  // Assertions
  TEST_ASSERT_EQUAL_INT(OPERATOR, token->type);
  TEST_ASSERT_EQUAL_STRING("+", token->lexeme);
  arena_free(&lexer.tokens.allocator);
}

void test_lexer_keyword(void) {
  Lexer lexer = tokenize("class");
  Token *token = next_token(&lexer);
  // Assertions
  TEST_ASSERT_EQUAL_INT(KEYWORD, token->type);
  TEST_ASSERT_EQUAL_STRING("class", token->lexeme);
  arena_free(&lexer.tokens.allocator);
}

void test_lexer_delimiter(void) {
  Lexer lexer = tokenize("()");
  Token *token = next_token(&lexer);
  // Assertions
  TEST_ASSERT_EQUAL_INT(DELIMITER, token->type);
  TEST_ASSERT_EQUAL_STRING("(", token->lexeme);
  arena_free(&lexer.tokens.allocator);
}

void test_lexer_newline(void) {
  Lexer lexer = tokenize("\n");
  Token *token = next_token(&lexer);
  // Assertions
  TEST_ASSERT_EQUAL_INT(NEWLINE, token->type);
  TEST_ASSERT_EQUAL_STRING("\\n", token->lexeme);
  arena_free(&lexer.tokens.allocator);
}

void test_lexer_square_brackets(void) {
  Lexer lexer = tokenize("[]");
  Token *token = next_token(&lexer);
  // Assertions
  TEST_ASSERT_EQUAL_INT(LSQB, token->type);
  TEST_ASSERT_EQUAL_STRING("[", token->lexeme);
  arena_free(&lexer.tokens.allocator);
}

void test_lexer_endmarker(void) {
  Lexer lexer = tokenize("");
  Token *token = next_token(&lexer);
  // Assertions
  TEST_ASSERT_EQUAL_INT(ENDMARKER, token->type);
  TEST_ASSERT_EQUAL_STRING("EOF", token->lexeme);
  arena_free(&lexer.tokens.allocator);
}

#endif