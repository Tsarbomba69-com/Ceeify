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
  SEM_ARITY_MISMATCH,
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
  size_t id; // unique identifier
  char *name;
  SymbolType kind;           // VAR, FUNCTION, MODULE, CLASS, BLOCK
  DataType dtype;            // INT, STR, BOOL, LIST, etc.
  ASTNode *decl_node;        // node where it was declared
  size_t scope_level;        // lexical depth / nesting
  struct SymbolTable *scope; // for FUNCTION, CLASS,
  struct Symbol *base_class;
} Symbol;

typedef enum {
  ATTR_OWN_CURRENT,
  ATTR_OWN_BASE
} AttrOwnership;

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
  size_t next_symbol_id;
  SymbolTable *current_scope;
  SemanticError last_error;
  Parser parser;
  Symbol *current_class;
} SemanticAnalyzer;

/* -----------------------------
 *  API
 * ----------------------------- */

void semantic_analyzer_free(SemanticAnalyzer *sa);

/* -----------------------------
 * SCOPE MANAGEMENT
 * ----------------------------- */

void sa_enter_scope(SemanticAnalyzer *sa);
void sa_exit_scope(SemanticAnalyzer *sa);
AttrOwnership resolve_attribute_owner(SemanticAnalyzer *sa, ASTNode *attr_node);

/**
 * @brief Determines if an ASTNode refers to the 'self' parameter of the current
 * method.
 */
bool is_self_reference(SemanticAnalyzer *sa, ASTNode *node);

/* -----------------------------
 * SYMBOL TABLE OPERATIONS
 * ----------------------------- */

void sa_define_symbol(SemanticAnalyzer *sa, Symbol *sym);
Symbol *sa_lookup(SemanticAnalyzer *sa, const char *name);
Symbol *sa_lookup_member(Symbol *class_sym, const char *name);
Symbol *resolve_symbol(SemanticAnalyzer *sa, ASTNode *node);

/* Main entrypoint */
SemanticAnalyzer analyze_program(Parser *parser);

/* Analyze a single AST node */
bool analyze_node(SemanticAnalyzer *sa, ASTNode *node);

// Error helpers
/**
 * @brief Records a semantic error and generates a formatted error message.
 * * This function captures the error type, the specific token where the error
 * occurred, and a custom formatted message. The resulting error is stored in
 * the `SemanticAnalyzer`'s `last_error` field.
 *
 * @param sa    Pointer to the SemanticAnalyzer instance.
 * @param type  The category of semantic error (e.g., UNDEFINED_VARIABLE).
 * @param tok   The token associated with the error location.
 * @param fmt   A printf-style format string for the error detail.
 * @param ...   Additional arguments for the format string.
 * * @note This function uses an internal arena allocator for the error strings.
 */
void sa_set_error(SemanticAnalyzer *sa, SemanticErrorType type, Token *tok,
                  const char *fmt, ...);

bool sa_has_error(SemanticAnalyzer *sa);

SemanticError sa_get_error(SemanticAnalyzer *sa);

DataType sa_infer_type(SemanticAnalyzer *sa, ASTNode *node);

static inline bool is_primitive(DataType type) {
  return type == INT || type == FLOAT || type == STR || type == BOOL;
}

const char *datatype_to_string(DataType t);

/* -----------------------------
 *  DIAGNOSTICS
 * ----------------------------- */

char *dump_symbol_table(SymbolTable *st);

#endif // SEMANTIC_H_
