#pragma once
#ifndef LEXER_H
#define LEXER_H

#include "utils.h"
#include "token.h"
#include "Token_arraylist.h"
#include <ctype.h>

typedef struct Lexer
{
	char* source;
	size_t position;
	size_t sourceLength;
} Lexer;

typedef Token_ArrayList Tokens;

Lexer CreateLexer(char* source);

Token_ArrayList Tokenize(Lexer* lexer);

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


