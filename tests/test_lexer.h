#pragma once
#ifndef TEST_LEXER_H_
#define TEST_LEXER_H_

#include "lexer.h"
#include "utils.h"
#include <unity.h>

void test_lexer_identifier(void) {
  Lexer lexer = tokenize("my_variable = 10", "test_file.py");
  Token *token = Token_get(&lexer.tokens, lexer.token_idx);
  TEST_ASSERT_EQUAL(IDENTIFIER, token->type);
  TEST_ASSERT_EQUAL_STRING("my_variable", token->lexeme);
  TEST_ASSERT_EQUAL(1, token->line);
  TEST_ASSERT_EQUAL(1, token->col);
  Token_free(&lexer.tokens);
}

void test_lexer_numeric(void) {
  Lexer lexer = tokenize("123", "test_file.py");
  Token *token = Token_get(&lexer.tokens, lexer.token_idx);
  // Assertions
  TEST_ASSERT_EQUAL_INT(NUMBER, token->type);
  TEST_ASSERT_EQUAL_STRING("123", token->lexeme);
  Token_free(&lexer.tokens);
}

void test_lexer_operator(void) {
  Lexer lexer = tokenize("+", "test_file.py");
  Token *token = Token_get(&lexer.tokens, lexer.token_idx);
  // Assertions
  TEST_ASSERT_EQUAL_INT(OPERATOR, token->type);
  TEST_ASSERT_EQUAL_STRING("+", token->lexeme);
  Token_free(&lexer.tokens);
}

void test_lexer_keyword(void) {
  Lexer lexer = tokenize("class", "test_file.py");
  Token *token = Token_get(&lexer.tokens, lexer.token_idx);
  // Assertions
  TEST_ASSERT_EQUAL_INT(KEYWORD, token->type);
  TEST_ASSERT_EQUAL_STRING("class", token->lexeme);
  Token_free(&lexer.tokens);
}

void test_lexer_delimiter(void) {
  Lexer lexer = tokenize("()", "test_file.py");
  Token *token = Token_get(&lexer.tokens, lexer.token_idx);
  // Assertions
  TEST_ASSERT_EQUAL_INT(LPAR, token->type);
  TEST_ASSERT_EQUAL_STRING("(", token->lexeme);
  Token_free(&lexer.tokens);
}

void test_lexer_newline(void) {
  Lexer lexer = tokenize("\n", "test_file.py");
  Token *token = Token_get(&lexer.tokens, lexer.token_idx);
  // Assertions
  TEST_ASSERT_EQUAL_INT(NEWLINE, token->type);
  TEST_ASSERT_EQUAL_STRING("\\n", token->lexeme);
  Token_free(&lexer.tokens);
}

void test_lexer_square_brackets(void) {
  Lexer lexer = tokenize("[]", "test_file.py");
  Token *token = Token_get(&lexer.tokens, lexer.token_idx);
  // Assertions
  TEST_ASSERT_EQUAL_INT(LSQB, token->type);
  TEST_ASSERT_EQUAL_STRING("[", token->lexeme);
  Token_free(&lexer.tokens);
}

void test_lexer_endmarker(void) {
  Lexer lexer = tokenize("", "test_file.py");
  Token *token = Token_get(&lexer.tokens, lexer.token_idx);
  // Assertions
  TEST_ASSERT_EQUAL_INT(ENDMARKER, token->type);
  TEST_ASSERT_EQUAL_STRING("EOF", token->lexeme);
  Token_free(&lexer.tokens);
}

void test_lexer_augassign(void) {
  Lexer lexer = tokenize("+=", "test_file.py");
  Token *token = Token_get(&lexer.tokens, lexer.token_idx);
  // Assertions
  TEST_ASSERT_EQUAL_INT(OPERATOR, token->type);
  TEST_ASSERT_EQUAL_STRING("+=", token->lexeme);
  Token_free(&lexer.tokens);
}

void test_lexer_rarrow(void) {
  Lexer lexer = tokenize("->", "test_file.py");
  Token *token = Token_get(&lexer.tokens, lexer.token_idx);
  TEST_ASSERT_EQUAL(RARROW, token->type);
  TEST_ASSERT_EQUAL_STRING("->", token->lexeme);
  TEST_ASSERT_EQUAL(1, token->line);
  TEST_ASSERT_EQUAL(1, token->col);
  Token_free(&lexer.tokens);
}

#endif
