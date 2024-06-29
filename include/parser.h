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
    LIST_EXPR,
    WHILE,
    FOR,
    END_BLOCK
} NodeType;

typedef enum DataType {
    STR,
    INT,
    FLOAT,
    COMPLEX,
    OBJECT,
    LIST,
    FUNCTION
} DataType;

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
    DataType type;
} Literal;

typedef struct BinaryOperation {
    char *operator;
    DataType type;
    Node *left;
    Node *right;
} BinaryOperation;

typedef struct UnaryOperation {
    char *operator;
    DataType type;
    Node *operand;
} UnaryOperation;

typedef struct Name {
    char *id;
    DataType type;
    Context ctx;
} Name;

typedef struct IfStmt {
    Node *test;
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

typedef struct WhileStmt {
    Node *test;
    Node_LinkedList body;
    Node_LinkedList orelse;
} WhileStmt;

typedef struct ForStmt {
    union {
        Name *variable;
        List *list;
        // TODO: Implement Tuple, Attribute, Subscript
    } target;
    Node *iter;
    Node_LinkedList body;
    Node_LinkedList orelse;
} ForStmt;

typedef struct Node {
    NodeType type;
    size_t depth;
    union {
        Literal *literal;
        ImportStmt *import_stmt;
        IfStmt *ifStmt;
        WhileStmt *whileStmt;
        ForStmt *forStmt;
        BinaryOperation *binOp;
        UnaryOperation *unOp;
        Assign *assignStmt;
        Name *variable;
        List *list;
    };
} Node;

typedef struct Symbol {
    DataType type;
    size_t line;
    size_t col;
    struct Symbol *params;
    char *scope; // The context that owns this symbol
} Symbol;

// Receives a list of tokens parses into a list of statements and advances the global token index
Node_LinkedList ParseStatements(Lexer *lexer);

// Receives a list of tokens parses into a statement node and advances the global token index
Node *ParseStatement(Lexer *lexer);

Node *ParseIfStatement(Lexer *lexer);

Node *ParseWhileStatement(Lexer *lexer);

Node *ParseForStatement(Lexer *lexer);

Token_ArrayList CollectUntil(Lexer *lexer, TokenType type);

// Receives a list of tokens parses into an expression node and advances the global token index
Node *ParseExpression(Lexer *lexer);

ImportStmt *CreateImportStmt();

Assign *CreateAssignStmt();

Name *CreateNameExpr();

WhileStmt *CreateWhileStmt();

UnaryOperation *CreateUnaryOp(char *op);

ForStmt *CreateForStmt();

Node *ShuntingYard(Tokens const *tokens);

void PrintList(Node const *node, char *spaces);

Literal *CreateLiteral(char *value);

IfStmt *CreateIfStmt();

Node *CreateNode(NodeType type);

Tokens InfixToPostfix(Tokens const *tokens);

Node *CreateBinOp(Token *token, Node *left, Node *right);

DataType InferType(Node const *node);

DataType TypePrecedence(DataType left, DataType right);

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

// Converts a data type to string. Mostly for printing purposes
const char *DataTypeToString(DataType type);

// Recursively traverse AST and assign depth to each node
void TraverseTree(Node *node, size_t depth);

bool BlacklistTokens(TokenType type, const TokenType blacklist[], size_t size);

#endif // !PARSER_H


