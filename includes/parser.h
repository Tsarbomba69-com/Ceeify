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
  AUG_ASSIGNMENT,
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

typedef struct __attribute__((aligned(16))) BinOp {
  ASTNode *left;
  ASTNode *right;
} BinOp;

typedef struct Assign {
  ASTNode_LinkedList targets;
  ASTNode *value;
  char *type_comment;
} __attribute__((aligned(128))) Assign;

typedef struct {
  ASTNode *target;
  Token *op;
  ASTNode *value;
} AugAssign;

typedef struct Compare {
  ASTNode *left;
  Token_ArrayList ops;
  ASTNode_LinkedList comparators;
} Compare;

typedef struct Parser {
  Lexer *lexer;
  ASTNode_LinkedList ast;
} __attribute__((aligned(128))) Parser;

typedef struct ControlFlowStatement {
  ASTNode *test;
  ASTNode_LinkedList body;
  ASTNode_LinkedList orelse;
} ControlFlowStatement;

typedef struct ASTNode {
  NodeType type;
  size_t depth;
  Token *token;
  union {
    BinOp bin_op;
    Assign assign;
    AugAssign aug_assign;
    Context ctx;
    Compare compare;
    ASTNode_LinkedList import;
    ControlFlowStatement ctrl_stmt;
  };
} __attribute__((aligned(128))) ASTNode;

Parser parse(Lexer *lexer);

void parser_free(Parser *parser);

uint8_t precedence(const char *operation);

cJSON *serialize_program(ASTNode_LinkedList *program);

cJSON *serialize_node(ASTNode *node);

#endif // PARSER_H_
