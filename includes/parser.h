#ifndef PARSER_H_
#define PARSER_H_
#pragma once

// TODO: Implement error handling in the parser (e.g., unexpected tokens)

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
  FUNCTION_DEF,
  RETURN,
  CALL,
  END_BLOCK
} NodeType;

typedef enum Context { STORE, DEL, LOAD } Context;

typedef enum DataType {
  STR,
  INT,
  FLOAT,
  COMPLEX,
  BOOL,
  LIST,
  OBJECT,
  NONE,
  VOID,
  UNKNOWN
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
  Token *current;
  Token *next;
  ASTNode_LinkedList ast;
} Parser;

typedef struct ControlFlowStatement {
  ASTNode *test;
  ASTNode_LinkedList body;
  ASTNode_LinkedList orelse;
} ControlFlowStatement;

typedef struct {
  ASTNode *name;
  ASTNode_LinkedList params;
  ASTNode_LinkedList body;
  ASTNode *returns;
} FunctionDef;

typedef struct {
  ASTNode *func;
  ASTNode_LinkedList args;
} CallExpr;

typedef struct ASTNode {
  NodeType type;
  size_t depth;
  Token *token;
  union {
    BinOp bin_op;
    Assign assign;
    AugAssign aug_assign;
    FunctionDef funcdef;
    CallExpr call;
    ASTNode *ret;
    ASTNode *annotation;
    Context ctx;
    Compare compare;
    ASTNode_LinkedList import;
    ControlFlowStatement ctrl_stmt;
  };
} ASTNode;

Parser parse(Lexer *lexer);

void parser_free(Parser *parser);

Token *next_token(Parser *parser);

Token *consume(Parser *parser, TokenType expected_type);

int8_t precedence(const char *op);

cJSON *serialize_program(ASTNode_LinkedList *program);

cJSON *serialize_node(ASTNode *node);

char *dump_program(ASTNode_LinkedList *program);

char *dump_node(ASTNode *node);

static inline bool is_boolean_operator(Token *t) {
  return strcmp(t->lexeme, "and") == 0 || strcmp(t->lexeme, "or") == 0 ||
         strcmp(t->lexeme, "not") == 0;
}

#endif // PARSER_H_
