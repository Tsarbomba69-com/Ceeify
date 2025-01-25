#pragma once
#ifndef TEST_LEXER_H_
#define TEST_LEXER_H_

#include "lexer.h"
#include "utils.h"
#include <unity.h>
#endif

void test_lexer_identifier(void) {
  Arena arena = {0};
  Lexer lexer = tokenize(load_file_text(&arena, "../tests/mock/lexer_identifier.py"));
  Token *token = next_token(&lexer);
  TEST_ASSERT_EQUAL(IDENTIFIER, token->type);
  TEST_ASSERT_EQUAL_STRING("my_variable", token->lexeme);
  TEST_ASSERT_EQUAL(1, token->line);
  TEST_ASSERT_EQUAL(1, token->col);
  arena_free(&lexer.tokens.arena);
  arena_free(&arena);
}