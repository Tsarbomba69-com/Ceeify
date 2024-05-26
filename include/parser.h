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
	IF,
	LIST
} NodeType;

typedef enum {
	STORE,
	DEL,
	LOAD
} Context;

typedef struct {
	ArrayList* modules;
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
	struct Node* test;
	Program* body;
	Program* orelse;
} IfStmt;

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
	struct Node* parent;
	union {
		Literal* literal;
		ImportStmt* import_stm;
		IfStmt* if_stmt;
		BinaryOperation* bin_op;
		UnaryOperation* unOp;
		Assign* assign_stmt;
		Name* variable;
		List* list;
	};
} Node;

void Parse(Tokens* tokens);

Node* ParseBlock(Tokens* tokens);

Node* ParseExpression(Tokens* tokens);

ImportStmt* CreateImportStmt();

Assign* CreateAssignStmt();

Name* CreateNameExpr();

UnaryOperation* CreateUnaryOp(char* op);

Node* ShantingYard(Tokens* tokens);

void PrintList(Node* node, char* spaces);

Literal* CreateLiteral(char* value);

IfStmt* CreateIfStmt();

Node* CreateNode(NodeType type);

Tokens InfixToPostfix(Tokens* tokens);

Node* CreateBinOp(Token* token, Node* left, Node* right);

Node* CreateListNode(Tokens* elements);

void PrintNode(Node* node);

void PrintVar(Name*, size_t);

void PrintImportStmt(Node*);

size_t Precedence(const char*);

Tokens CollectExpression(Tokens* tokens, size_t from);

const char* NodeTypeToString(NodeType type);

const char* CtxToString(Name* var);

void TraverseTree(Node* node, size_t depth);

bool BlacklistTokens(TokenType type, TokenType blacklist[], size_t size);

#endif // !PARSER_H


