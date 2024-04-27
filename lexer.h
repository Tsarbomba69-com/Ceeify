#pragma once
#ifndef LEXER_H
#define LEXER_H

#include "utils.h"
#include <ctype.h>

typedef enum {
	IDENTIFIER,
	NUMBER,
	STRING,
	OPERATOR,
	KEYWORD,
	NEWLINE
} TokenType;

typedef struct Token
{
	TokenType type;
	char* lexeme;
} Token;

typedef struct Lexer
{
	char* source;
	size_t position;
} Lexer;

void Tokenize(Lexer lexer);

void PrintToken(Token* token);

const char* TokenTypeToString(TokenType type);

#endif // !LEXER_H


