#ifndef TAC_H_
#define TAC_H_
#pragma once

// TODO: Implement constant table folding during TAC generation

#include "semantic.h"
#include "string_builder.h"

/* -----------------------------
 *  CORE TYPES
 * ----------------------------- */

typedef struct TACProgram {
  struct TACInstruction *instructions;
  size_t count;
  size_t capacity;
} TACProgram;

typedef struct {
  size_t reg_counter;
  TACProgram program;
  SemanticAnalyzer *sa;
} Tac;

typedef enum {
  // Data operations
  TAC_CONST,
  TAC_LOAD,
  TAC_STORE,
  // Arithmetic operations
  TAC_ADD,
  TAC_SUB,
  TAC_MUL,
  TAC_DIV,
  TAC_CMP,
  // Function operations
  TAC_CALL,
  TAC_RETURN,
  // Control flow operations
  TAC_JUMP,
  TAC_CJUMP,
  TAC_LABEL,
} TACOp;

typedef struct {
  size_t id;     // virtual register
  DataType type; // from semantic analysis
} TACValue;

typedef struct TACInstruction {
  TACOp op;
  TACValue lhs;
  TACValue rhs;
  TACValue result;
  const char *label;       // target / definition
  const char *label_false; // for conditional jump
} TACInstruction;

TACProgram tac_generate(SemanticAnalyzer *sa);

void tac_free(TACProgram *program);

StringBuilder tac_generate_code(TACProgram *program, Allocator *allocator);

#endif // TAC_H