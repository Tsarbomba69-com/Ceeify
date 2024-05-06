#pragma once
#ifndef PARSER_H
#define PARSER_H
#include <ctype.h>
#include "lexer.h"

typedef struct {
	enum {
		PROGRAM,
		STATEMENT,
		EXPRESSION,
		IDENTIFIER_NODE,
		LITERAL
	} type;
	union {
		struct ASTNode* children;
		char* identifier;
	};
	size_t numChildren;
} ASTNode;

void Parse(ArrayList* tokens);

ASTNode* ParseKeyword(Token* token);
#endif // !PARSER_H


