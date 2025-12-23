#ifndef SEMANTIC_H_
#define SEMANTIC_H_
#pragma once

#include "parser.h"

typedef enum SymbolType { CLASS, MODULE, FUNCTION, BLOCK, VAR } SymbolType;

/* -----------------------------
 *  SEMANTIC ERROR
 * ----------------------------- */

typedef enum SemanticErrorType {
  SEM_OK = 0,
  SEM_UNDEFINED_VARIABLE,
  SEM_TYPE_MISMATCH,
  SEM_INVALID_OPERATION,
  SEM_ASSIGN_TO_READONLY,
  SEM_UNSUPPORTED_FEATURE,
  SEM_UNKNOWN
} SemanticErrorType;

typedef struct SemanticError {
  SemanticErrorType type;
  char *detail;  // new: dynamic message body ("name 'x' is not defined")
  char *message; // final formatted traceback (Python-like)
  Token *token;  // where the error occurred
} SemanticError;

/* -----------------------------
 *  SYMBOL TABLE
 * ----------------------------- */

typedef struct Symbol {
  char *name;
  SymbolType kind;           // VAR, FUNCTION, MODULE, CLASS, BLOCK
  DataType dtype;            // INT, STR, BOOL, LIST, etc.
  ASTNode *decl_node;        // node where it was declared
  size_t scope_level;        // lexical depth / nesting
  struct SymbolTable *scope; // for FUNCTION, CLASS, MODULE
} Symbol;

typedef struct SymbolTableEntry {
  Symbol *symbol;
  struct SymbolTableEntry *next;
} SymbolTableEntry;

typedef struct SymbolTable {
  SymbolTableEntry *entries;
  struct SymbolTable *parent; // NULL for global scope
  size_t depth;
} SymbolTable;

/* -----------------------------
 *  SEMANTIC ANALYZER
 * ----------------------------- */

typedef struct SemanticAnalyzer {
  SymbolTable *current_scope;
  SemanticError last_error;
  Parser *parser;
} SemanticAnalyzer;

/* -----------------------------
 *  API
 * ----------------------------- */

void semantic_analyzer_free(SemanticAnalyzer *sa);

/* Scope management */
void sa_enter_scope(SemanticAnalyzer *sa);
void sa_exit_scope(SemanticAnalyzer *sa);

/* Symbol table operations */
void sa_define_symbol(SemanticAnalyzer *sa, Symbol *sym);
Symbol *sa_lookup(SemanticAnalyzer *sa, const char *name);

/* Main entrypoint */
SemanticAnalyzer analyze_program(Parser *parser);

/* Analyze a single AST node */
bool analyze_node(SemanticAnalyzer *sa, ASTNode *node);

/* Error helpers */
void sa_set_error(SemanticAnalyzer *sa, SemanticErrorType type, Token *tok,
                  const char *fmt, ...);

bool sa_has_error(SemanticAnalyzer *sa);

SemanticError sa_get_error(SemanticAnalyzer *sa);

#endif // SEMANTIC_H_
