#pragma once

#ifndef LEXER_H_
#define LEXER_H_

#include "token_arraylist.h"
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include <ctype.h>
#endif

typedef struct Token_ArrayList Token_ArrayList;

typedef enum TokenType {
  IDENTIFIER,
  TEXT,
  NUMERIC,
  OPERATOR,
  KEYWORD,
  DELIMITER,
  NEWLINE,
  LSQB,
  RSQB,
  ENDMARKER
} TokenType;

typedef struct Token {
  TokenType type;
  char *lexeme;
  size_t line;
  size_t col;
  size_t ident;
} Token;

typedef struct Lexer {
  const char *source;
  size_t position;
  size_t token_idx;
  size_t source_length;
  Token_ArrayList tokens;
} Lexer;

Lexer tokenize(const char* source);

Token* next_token(Lexer *lexer);