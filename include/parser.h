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
	UNARY_OPERATION,
	LITERAL,
	VARIABLE,
} NodeType;

typedef struct {
	ArrayList modules;
} ImportStmt;

typedef struct {
	char* value;
} Literal;

typedef struct {
	char* operator;
	struct Node* left;
	struct Node* right;
} BinaryOperation;

typedef struct {
	char* operator;
	struct Node* operand;
} UnaryOperation;

typedef struct {
	char* id;
	enum {
		STORE,
		DEL,
		LOAD
	} ctx;
} Name;

typedef struct {
	Name* target;
	struct Node* value;
} Assign;

typedef struct Node {
	NodeType type;
	size_t depth;
	union {
		Literal* literal;
		ImportStmt* importStm;
		BinaryOperation* binOp;
		UnaryOperation* unOp;
		Assign* assignStmt;
		Name* variable;
	};
} Node;

typedef ArrayList Program;

void Parse(ArrayList* tokens);

ImportStmt* CreateImportStmt();

Assign* CreateAssignStmt();

Name* CreateNameExpr();

UnaryOperation* CreateUnaryOp(char* op);

Literal* CreateLiteral(char* value);

Node* CreateNode(NodeType type);

Tokens InfixToPostfix(Tokens* tokens);

void PrintNode(Node* node);

void PrintVar(Name* variable);

void PrintImportStmt(ImportStmt* stmt);

const char* NodeTypeToString(NodeType type);

const char* CtxToString(Name* var);

size_t Precedence(char op);

#endif // !PARSER_H


