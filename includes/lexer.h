#ifndef LEXER_H_
#define LEXER_H_
#pragma once

#include "token_arraylist.h"
#include "utils.h"
#include <cJSON.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <slog.h>

typedef struct Token_ArrayList Token_ArrayList;

typedef enum TokenType {
  IDENTIFIER = 0,
  STRING,
  NUMBER,
  OPERATOR,
  KEYWORD,
  COMMA,
  COLON,
  EQUAL,
  EQEQUAL,
  NEWLINE,
  LPAR,
  RPAR,
  LSQB,
  RSQB,
  ENDMARKER
} TokenType;

typedef struct __attribute__((packed)) __attribute__((aligned(64))) Token {
  TokenType type;
  char *lexeme;
  size_t line;
  size_t col;
  size_t ident;
} Token;

typedef struct Lexer {
  const char *source;
  const char *filename;
  size_t position;
  size_t token_idx;
  size_t source_length;
  Token_ArrayList tokens;
} __attribute__((aligned(128))) Lexer;

Lexer tokenize(const char *source, const char *filename);

Token *next_token(Lexer *lexer);

cJSON *serialize_token(Token *token);

cJSON *serialize_tokens(Token_ArrayList *tokens);

cJSON* serialize_lexer(Lexer *lexer);

Token *peek_token(Lexer *lexer);

#endif // !LEXER_H_
