#ifndef CODEGEN_H_
#define CODEGEN_H_
#pragma once

#include "semantic.h"
#include "string_builder.h"

/* -----------------------------
 *  CODEGEN ERROR
 * ----------------------------- */

typedef enum CodegenErrorType {
  CG_OK = 0,
  CG_SEMANTIC_ERROR,
  CG_UNSUPPORTED_NODE,
  CG_INTERNAL_ERROR
} CodegenErrorType;

typedef struct CodegenError {
  CodegenErrorType type;
  const char *message;
  ASTNode *node; // node that caused the error (if any)
} CodegenError;

/* -----------------------------
 *  CODEGEN CONTEXT
 * ----------------------------- */

typedef struct Codegen {
  SemanticAnalyzer sa;
  StringBuilder output;
  CodegenError last_error;
  bool is_standalone; // Tracks if the current node should be treated as a
                      // statement
} Codegen;

/* -----------------------------
 *  API
 * ----------------------------- */

/* Initialize codegen context */
Codegen codegen_init(SemanticAnalyzer *sa);

/* Free internal buffers */
void codegen_free(Codegen *cg);

/* Entry point: generate full C translation unit */
bool codegen_program(Codegen *cg);

/* Error helpers */
bool codegen_has_error(Codegen *cg);
CodegenError codegen_get_error(Codegen *cg);

#endif // CODEGEN_H_