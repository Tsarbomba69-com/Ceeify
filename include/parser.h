#pragma once
#ifndef PARSER_H
#include <ctype.h>
#include <assert.h>
#include "lexer.h"
#define PARSER_H
#define MODULE_NAME_CAP 10

typedef ArrayList Program;

typedef ArrayList Nodes;

typedef enum {
	PROGRAM,
	ASSIGNMENT,
	IMPORT,
	BINARY_OPERATION,
	UNARY_OPERATION,
	LITERAL,
	VARIABLE,
	LIST
} NodeType;

typedef enum {
	STORE,
	DEL,
	LOAD
} Context;

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
	Context ctx;
} Name;

typedef struct {
	Name* target;
	struct Node* value;
} Assign;

typedef struct {
	Nodes elts;
	Context ctx;
} List;

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
		List* list;
	};
} Node;

void Parse(Tokens* tokens);

ImportStmt* CreateImportStmt();

Assign* CreateAssignStmt();

Name* CreateNameExpr();

UnaryOperation* CreateUnaryOp(char* op);

Node* ShantingYard(Tokens* tokens);

void PrintList(Node* node);

Literal* CreateLiteral(char* value);

Node* CreateNode(NodeType type);

Tokens InfixToPostfix(Tokens* tokens);

Node* CreateBinOp(Token* token, Node* left, Node* right);

Node* CreateListNode(Tokens* elements);

void PrintNode(Node* node);

void PrintVar(Name*, size_t);

void PrintImportStmt(Node*);

size_t Precedence(char op);

const char* NodeTypeToString(NodeType type);

const char* CtxToString(Name* var);

void TraverseTree(Node* node, size_t depth);

#endif // !PARSER_H


