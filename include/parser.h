#pragma once
#ifndef PARSER_H
#include <ctype.h>
#include "lexer.h"
#define PARSER_H
#define MODULE_NAME_CAP 10

typedef enum {
	PROGRAM,
	ASSIGNMENT,
	IMPORT,
	BINARY_OPERATION,
	UNARY_OPERATION
} NodeType;

typedef struct {
	NodeType type;
	char* moduleNames[MODULE_NAME_CAP];
	int moduleNamesCount;
} ImportStmt;

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

ImportStmt* CreateImportStmt();

void PrintImportStmt(ImportStmt* stmt);

const char* NodeTypeToString(NodeType type);

#endif // !PARSER_H


