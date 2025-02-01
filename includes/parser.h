#ifndef PARSER_H_
#define PARSER_H_
#pragma once

#include "ASTNode_linkedlist.h"
#include "lexer.h"
#include "utils.h"
#include <stdbool.h>

typedef enum NodeType {
  PROGRAM = 0,
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

typedef struct Symbol Symbol;

typedef enum SymbolType { CLASS, MODULE, FUNCTION, BLOCK, VAR } SymbolType;

typedef enum Context { STORE, DEL, LOAD } Context;

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

typedef struct __attribute__((aligned(ALIGNED_16))) BinaryOperation {
  ASTNode *left;
  ASTNode *right;
} BinaryOperation;

typedef struct Assign {
  ASTNode_LinkedList targets;
  ASTNode *value;
  char *type_comment;
} __attribute__((aligned(128))) Assign;

typedef struct Parser {
  Lexer *lexer;
  ASTNode_LinkedList ast;
} __attribute__((aligned(128))) Parser;

typedef struct ASTNode {
  NodeType type;
  size_t depth;
  Token *token;
  union {
    BinaryOperation bin_op;
    Assign assign;
    Context ctx;
  };
} __attribute__((packed)) __attribute__((aligned(128))) ASTNode;

Symbol *create_symbol(DataType type, SymbolType kind);

Parser parse(Lexer *lexer);

size_t precedence(const char *operation);

#endif // PARSER_H_
