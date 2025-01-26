#pragma once
#ifndef PARSER_H_
#define PARSER_H_

#include "lexer.h"
#include "utils.h"
#endif

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

typedef struct Node_LinkedList Node_LinkedList;

typedef struct BinaryOperation BinaryOperation;

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

typedef struct Parser {
  Lexer lexer;
  Symbol *context;
  Node_LinkedList *ast;
} Parser;

typedef struct ASTNode {
  NodeType type;
  size_t depth;
  Token *token;
  union {
    BinaryOperation *bin_op;
  };
} ASTNode;

typedef struct __attribute__((aligned(ALIGNED_16))) BinaryOperation {
  ASTNode *left;
  ASTNode *right;
} BinaryOperation;

Symbol *create_symbol(DataType type, SymbolType kind);

static inline Parser parse(Lexer lexer) {
  Symbol *global = create_symbol(VOID, MODULE);
  return (Parser){.lexer = lexer, .context = global};
}