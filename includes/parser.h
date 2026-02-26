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
  IMPORT_FROM,
  BINARY_OPERATION,
  COMPARE,
  UNARY_OPERATION,
  LITERAL,
  VARIABLE,
  IF,
  IF_EXPR,
  LIST_EXPR,
  LIST_COMPREHENSION,
  WHILE,
  FOR,
  FUNCTION_DEF,
  RETURN,
  CALL,
  CLASS_DEF,
  ATTRIBUTE,
  MATCH,
  CASE,
  TUPLE,
  SUBSCRIPT,
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

typedef struct Attribute {
  ASTNode *value; // The object (e.g., the 'snake' Name node)
  char *attr;     // The name of the attribute (e.g., "bite")
} Attribute;

typedef struct Compare {
  ASTNode *left;
  Token_ArrayList ops;
  ASTNode_LinkedList comparators;
} Compare;

typedef struct Parser {
  Lexer lexer;
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

typedef struct {
  ASTNode *key;  // dict comp only, NULL otherwise
  ASTNode *expr; // The expression being evaluated (e.g., 'i'). Value if it's a
                 // dictionary comprehension
  ASTNode *target; // The target variable(s) (e.g., 'i' in the for-clause)
  ASTNode *iter;   // The iterable (e.g., 'x')
  ASTNode_LinkedList ifs; // (Optional) Guards, e.g., 'if i > 2'
} Comprehension;

typedef struct {
  ASTNode *value; // the object being subscripted, e.g. `item`
  ASTNode *slice; // the key/index expression, e.g. `'amount'`
} Subscript;

typedef struct ASTNode {
  NodeType type;
  size_t depth;
  Token *token;
  ASTNode *parent;
  Context ctx; // I'm out of ideas for this will sufice
  union {
    BinOp bin_op;
    Assign assign;
    AugAssign aug_assign;
    FunctionDef def;
    CallExpr call;
    ASTNode *child; // Types that use this: RETURN
    Compare compare;
    ASTNode_LinkedList collection;
    ControlFlowStatement ctrl_stmt;
    Attribute attribute;
    Subscript subscript;
    Comprehension list_comp;
  };
} ASTNode;

Parser parse(Lexer *lexer);

ASTNode *node_new(Parser *parser, Token *token, NodeType type);

void parser_free(Parser *parser);

Token *advance(Parser *parser);

Token *consume(Parser *parser, TokenType expected_type);

int8_t get_infix_precedence(const char *op);

int8_t get_prefix_precedence(const char *op);

cJSON *serialize_program(ASTNode_LinkedList *program);

cJSON *serialize_node(ASTNode *node);

char *dump_program(ASTNode_LinkedList *program);

char *dump_node(ASTNode *node);

const char *node_type_to_string(NodeType type);

static inline bool is_boolean_operator(Token *t) {
  return strcmp(t->lexeme, "and") == 0 || strcmp(t->lexeme, "or") == 0 ||
         strcmp(t->lexeme, "not") == 0;
}

static inline bool is_executable(NodeType type) {
  switch (type) {
  case CALL:
  case IF:
  case WHILE:
  case FOR:
  case BINARY_OPERATION: // e.g., 1 + 1 at top level
    return true;
  default:
    return false;
  }
}

#endif // PARSER_H_
