#pragma once
#ifndef LEXER_H
#define LEXER_H

#include "utils.h"
#include <ctype.h>

typedef enum {
	IDENTIFIER,
	INTEGER,
	FLOAT,
	STRING,
	OPERATOR,
	KEYWORD,
	DELIMITER,
	NEWLINE,
	INDENT,
	DEDENT,
	ENDMARKER
} TokenType;

typedef struct
{
	TokenType type;
	char* lexeme;
	size_t line;
	size_t col;
} Token;

typedef struct Lexer
{
	char* source;
	size_t position;
	size_t sourceLength;
} Lexer;

typedef ArrayList Tokens;

Lexer CreateLexer(char* source);

ArrayList Tokenize(Lexer* lexer);

void PrintToken(Token* token);

const char* TokenTypeToString(TokenType type);

Token* CreateToken(TokenType type);

Token* CreateDelimiterToken(Lexer* lexer, const char* matchedDelimiter);

Token* CreateStringToken(Lexer* lexer, char character);

Token* CreateOperatorToken(Lexer* lexer, const char* matchedOperator);

Token* CreateKeywordToken(Lexer* lexer, char character);

Token* CreateNumberToken(Lexer* lexer, char character);

Token* CreateNewLineToken();

Token* CreateEOFToken(Lexer* lexer);

void DestroyToken(Token* token);

#endif // !LEXER_H


