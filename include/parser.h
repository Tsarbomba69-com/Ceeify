#pragma once

#include <ctype.h>
#include <assert.h>
#include "lexer.h"
#include "Node_linkedlist.h"

#ifndef PARSER_H
#define PARSER_H

typedef Token_ArrayList Tokens;

typedef enum NodeType {
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

typedef enum Context {
    STORE,
    DEL,
    LOAD
} Context;

typedef struct ImportStmt {
    Token_ArrayList modules;
} ImportStmt;

typedef struct Literal {
    char *value;
} Literal;

typedef struct BinaryOperation {
    char *operator;
    Node *left;
    Node *right;
} BinaryOperation;

typedef struct UnaryOperation {
    char *operator;
    Node *operand;
} UnaryOperation;

typedef struct Name {
    char *id;
    Context ctx;
} Name;

typedef struct IfStmt {
    struct Node *test;
    Node_LinkedList body;
    Node_LinkedList orelse;
} IfStmt;

typedef struct Assign {
    Name *target;
    Node *value;
} Assign;

typedef struct List {
    Node_LinkedList elts;
} List;

typedef struct Node {
    NodeType type;
    size_t depth;
    union {
        Literal *literal;
        ImportStmt *import_stm;
        IfStmt *if_stmt;
        BinaryOperation *bin_op;
        UnaryOperation *unOp;
        Assign *assign_stmt;
        Name *variable;
        List *list;
    };
} Node;

// Receives a list of tokens parses into a list of statements and advances the global token index
Node_LinkedList ParseStatements(Tokens *tokens);

// Receives a list of tokens parses into a statement node and advances the global token index
Node *ParseStatement(Tokens *tokens);

Token_ArrayList CollectUntil(Tokens const *tokens, TokenType type);

// Receives a list of tokens parses into an expression node and advances the global token index
Node *ParseExpression(Tokens const *tokens);

ImportStmt *CreateImportStmt();

Assign *CreateAssignStmt();

Name *CreateNameExpr();

UnaryOperation *CreateUnaryOp(char *op);

Node *ShuntingYard(Tokens const *tokens);

void PrintList(Node const *node, char *spaces);

Literal *CreateLiteral(char *value);

IfStmt *CreateIfStmt();

Node *CreateNode(NodeType type);

Tokens InfixToPostfix(Tokens const *tokens);

Node *CreateBinOp(Token *token, Node *left, Node *right);

// Recursively traverse AST and print each node
void PrintNode(Node *node);

void PrintVar(Name *, size_t);

void PrintImportStmt(Node const *);

size_t Precedence(const char *);

Tokens CollectExpression(Tokens const *tokens, size_t from);

// Converts a node type to string. Mostly for printing purposes
const char *NodeTypeToString(NodeType type);

// Converts a variable context to string. Mostly for printing purposes
const char *CtxToString(Name const *var);

// Recursively traverse AST and assign depth to each node
void TraverseTree(Node *node, size_t depth);

bool BlacklistTokens(TokenType type, const TokenType blacklist[], size_t size);

#endif // !PARSER_H


