#pragma once

#include <ctype.h>
#include <assert.h>
#include "lexer.h"
#include "Node_linkedlist.h"
#include "Symbol_hashtable.h"
#include "cJSON.h"

#ifndef PARSER_H
#define PARSER_H

typedef Token_ArrayList Tokens;

typedef enum NodeType {
    PROGRAM,
    ASSIGNMENT,
    IMPORT,
    BINARY_OPERATION,
    COMPARE,
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
    BOOL,
    LIST,
    OBJECT,
    VOID
} DataType;

typedef enum SymbolType {
    CLASS,
    MODULE,
    FUNCTION,
    BLOCK,
    VAR
} SymbolType;

typedef enum Context {
    STORE,
    DEL,
    LOAD
} Context;

typedef struct Parser {
    Lexer lexer;
    Symbol *context;
    Node_LinkedList ast;
} Parser;

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
    size_t line;
    size_t col;
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

typedef struct Compare {
    Node *left;
    Token_ArrayList ops;
    Node_LinkedList comparators;
} Compare;

typedef struct Node {
    NodeType type;
    size_t depth;
    union {
        Literal *literal;
        ImportStmt *importStmt;
        IfStmt *ifStmt;
        WhileStmt *whileStmt;
        ForStmt *forStmt;
        BinaryOperation *binOp;
        Compare *compare;
        UnaryOperation *unOp;
        Assign *assignStmt;
        Name *variable;
        List *list;
    };
} Node;

typedef struct Symbol {
    DataType type;
    SymbolType kind;
    size_t line;
    size_t col;
    Symbol *parent;
    Symbol_HashTable scope;
} Symbol;

Symbol *CreateSymbol(DataType type, SymbolType kind);

static inline Parser CreateParser(Lexer lexer) {
    Symbol *global = CreateSymbol(VOID, MODULE);
    return (Parser) {.lexer = lexer, .context = global};
}

Symbol *StackSymbolsLookup(Symbol *namespace, const char *id);

// Receives a list of tokens parses into a list of statements and advances the lexer's token index
Node_LinkedList ParseStatements(Parser *parser);

// Receives a list of tokens parses into a statement node and advances the lexer's token index
Node *ParseStatement(Parser *parser);

Node *ParseIfStatement(Parser *parser);

Node *ParseWhileStatement(Parser *parser);

Node *ParseForStatement(Parser *parser);

Token_ArrayList CollectUntil(Parser *parser, TokenType type);

// Receives a list of tokens parses into an expression node and advances the lexer's token index
Node *ParseExpression(Parser *parser);

ImportStmt *CreateImportStmt();

Assign *CreateAssignStmt();

Name *CreateNameExpr();

WhileStmt *CreateWhileStmt();

UnaryOperation *CreateUnaryOp(char *op);

ForStmt *CreateForStmt();

Node *ShuntingYard(Tokens const *tokens, Symbol *namespace);

void PrintList(Node const *node, char *spaces);

Literal *CreateLiteral(char *value);

IfStmt *CreateIfStmt();

Node *CreateNode(NodeType type);

Tokens InfixToPostfix(Tokens const *tokens);

Node *CreateBinOp(Token *token, Node *left, Node *right);

Node *CreateCompare(Token *token, Node *left, Node *right);

DataType InferType(Node const *node, Symbol *namespace);

DataType InferListType(Node_LinkedList list, Symbol *namespace);

DataType TypePrecedence(DataType left, DataType right);

// Recursively traverse AST and print each node
void PrintNode(Node *node);

void PrintVar(Name *, size_t);

void PrintImportStmt(Node const *);

size_t Precedence(const char *);

Tokens CollectExpression(Tokens const *tokens, size_t from);

cJSON *SerializeNode(Node *node);

cJSON *SerializeName(Name *variable);

cJSON *SerializeProgram(Node_LinkedList *program);

cJSON *SerializeToken(Token *token);

cJSON *SerializeSymbolTable(const Symbol_HashTable *namespaces);

cJSON *SerializeSymbol(const Symbol *symbol);

// Converts a node type to string. Mostly for printing purposes
const char *NodeTypeToString(NodeType type);

// Converts a variable context to string. Mostly for printing purposes
const char *CtxToString(Name const *var);

// Converts a data type to string. Mostly for printing purposes
const char *DataTypeToString(DataType type);

// Converts a symbol type to string. Mostly for printing purposes
const char *SymbolTypeToString(SymbolType type);

// Recursively traverse AST and assign depth to each node
void TraverseTree(Node *node, size_t depth);

bool BlacklistTokens(TokenType type, const TokenType blacklist[], size_t size);

#endif // !PARSER_H


