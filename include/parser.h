#pragma once
#ifndef PARSER_H
#define PARSER_H
#include <ctype.h>
#include "lexer.h"

typedef enum {
	PROGRAM,
	ASSIGNMENT,
	BINARY_OPERATION,
	UNARY_OPERATION
} NodeType;

typedef struct Literal {
	NodeType type;
	char* value;
};

typedef struct Node {
	NodeType type;
	union {
		struct Literal* node;
		char* identifier;
	};
} Node;

typedef struct {
	NodeType type;
	char* operator;
	Node* left;
	Node* right;
} BinaryOperation;

void Parse(ArrayList* tokens);

#endif // !PARSER_H


