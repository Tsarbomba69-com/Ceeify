#pragma once
#ifndef TOKEN_H
#define TOKEN_H

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

#endif
