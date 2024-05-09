#pragma once
#ifndef PARSER_H
#include <ctype.h>
#include <assert.h>
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

typedef struct {
	NodeType type;
	char* value;
} Literal;

typedef struct Node {
	NodeType type;
	union {
		Literal* literal;
		ImportStmt* importStm;
		char* identifier;
	};
} Node;

typedef struct {
	NodeType type;
	char* operator;
	Node* left;
	Node* right;
} BinaryOperation;

typedef struct {
	NodeType type;
	ArrayList body;
} Program;

void Parse(ArrayList* tokens);

ImportStmt* CreateImportStmt();

Node* CreateNode(NodeType type);

void PrintNode(Node* node);

void PrintImportStmt(ImportStmt* stmt);

const char* NodeTypeToString(NodeType type);

#endif // !PARSER_H


