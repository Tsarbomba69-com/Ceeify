#ifndef TEST_TAC_H_
#define TEST_TAC_H_
#pragma once
#include "tac.h"
#include <unity.h>

void test_tac_simple_assignment(void) {
  // Arrange
  Lexer lexer = tokenize("x = 5", "test.py");
  Parser parser = parse(&lexer);
  SemanticAnalyzer sa = analyze_program(&parser);
  // Act
  TACProgram tac = tac_generate(&sa);
  // Assert
  TEST_ASSERT_NOT_NULL(tac.instructions);
  TEST_ASSERT_NOT_NULL(tac.instructions);
  TEST_ASSERT_EQUAL_INT(2, tac.count); // CONST + STORE instructions
  // 2. First instruction: CONST 5
  TACInstruction *inst1 = &tac.instructions[0];
  TEST_ASSERT_EQUAL_INT(TAC_CONST, inst1->op);
  TEST_ASSERT_EQUAL_INT(INT, inst1->lhs.type);
  // TEST_ASSERT_NOT_EQUAL(0, inst1->result.id); // Should have assigned a
  // virtual register
  // 3. Second instruction: STORE to x
  TACInstruction *inst2 = &tac.instructions[1];
  TEST_ASSERT_EQUAL_INT(TAC_STORE, inst2->op);
  TEST_ASSERT_EQUAL_INT(INT, inst2->lhs.type); // Source type
  TEST_ASSERT_EQUAL(inst1->result.id,
                    inst2->lhs.id); // Should use register from CONST
  // 4. Verify no memory leaks in TAC generation
  TEST_ASSERT_TRUE(tac.capacity >= tac.count);
  // Cleanup
  parser_free(&parser);
}

void test_tac_binary_expression(void) {
  // Arrange
  Lexer lexer = tokenize("x = 1 + 2", "test.py");
  Parser parser = parse(&lexer);
  SemanticAnalyzer sa = analyze_program(&parser);
  // Act
  TACProgram tac = tac_generate(&sa);
  // Assert
  TEST_ASSERT_NOT_NULL(tac.instructions);
  TEST_ASSERT_EQUAL_INT(4, tac.count);
  // Expected:
  // 0: CONST 1
  // 1: CONST 2
  // 2: ADD r0, r1 -> r2
  // 3: STORE r2 -> x

  TEST_ASSERT_EQUAL_INT(TAC_CONST, tac.instructions[0].op);
  TEST_ASSERT_EQUAL_INT(TAC_CONST, tac.instructions[1].op);

  TEST_ASSERT_EQUAL_INT(TAC_ADD, tac.instructions[2].op);
  TEST_ASSERT_EQUAL(tac.instructions[0].result.id, tac.instructions[2].lhs.id);
  TEST_ASSERT_EQUAL(tac.instructions[1].result.id, tac.instructions[2].rhs.id);

  TEST_ASSERT_EQUAL_INT(TAC_STORE, tac.instructions[3].op);
  TEST_ASSERT_EQUAL(tac.instructions[2].result.id, tac.instructions[3].lhs.id);
  // Cleanup
  parser_free(&parser);
}

void test_tac_variable_to_variable_assignment(void) {
  // Arrange
  Lexer lexer = tokenize("x = 1\ny = x", "test.py");
  Parser parser = parse(&lexer);
  SemanticAnalyzer sa = analyze_program(&parser);
  // Act
  TACProgram tac = tac_generate(&sa);
  TEST_ASSERT_NOT_NULL(tac.instructions);
  TEST_ASSERT_EQUAL_INT(4, tac.count);
  // 0: CONST 1
  // 1: STORE -> x
  // 2: LOAD x
  // 3: STORE -> y

  TEST_ASSERT_EQUAL_INT(TAC_CONST, tac.instructions[0].op);
  TEST_ASSERT_EQUAL_INT(TAC_STORE, tac.instructions[1].op);

  TEST_ASSERT_EQUAL_INT(TAC_LOAD, tac.instructions[2].op);
  TEST_ASSERT_EQUAL_INT(TAC_STORE, tac.instructions[3].op);

  TEST_ASSERT_EQUAL(tac.instructions[2].result.id, tac.instructions[3].lhs.id);
  // Cleanup
  parser_free(&parser);
}

void test_tac_variable_reassignment(void) {
  // Arrange
  Lexer lexer = tokenize("x = 1\nx = 2", "test.py");
  Parser parser = parse(&lexer);
  SemanticAnalyzer sa = analyze_program(&parser);
  // Act
  TACProgram tac = tac_generate(&sa);
  // Assert
  TEST_ASSERT_EQUAL_INT(4, tac.count);

  TEST_ASSERT_EQUAL_INT(TAC_CONST, tac.instructions[0].op);
  TEST_ASSERT_EQUAL_INT(TAC_STORE, tac.instructions[1].op);

  TEST_ASSERT_EQUAL_INT(TAC_CONST, tac.instructions[2].op);
  TEST_ASSERT_EQUAL_INT(TAC_STORE, tac.instructions[3].op);

  // Ensure second assignment does not reuse first CONST register
  TEST_ASSERT_NOT_EQUAL(tac.instructions[0].result.id,
                        tac.instructions[2].result.id);
  // Cleanup
  parser_free(&parser);
}

void test_tac_expression_with_variable(void) {
  // Arrange
  Lexer lexer = tokenize("x = 1\ny = x + 3", "test.py");
  Parser parser = parse(&lexer);
  SemanticAnalyzer sa = analyze_program(&parser);
  // Act
  TACProgram tac = tac_generate(&sa);
  // Assert
  TEST_ASSERT_EQUAL_INT(6, tac.count);
  // 0: CONST 1
  // 1: STORE x
  // 2: LOAD x
  // 3: CONST 3
  // 4: ADD
  // 5: STORE y

  TEST_ASSERT_EQUAL_INT(TAC_LOAD, tac.instructions[2].op);
  TEST_ASSERT_EQUAL_INT(TAC_CONST, tac.instructions[3].op);
  TEST_ASSERT_EQUAL_INT(TAC_ADD, tac.instructions[4].op);

  TEST_ASSERT_EQUAL(tac.instructions[2].result.id, tac.instructions[4].lhs.id);
  TEST_ASSERT_EQUAL(tac.instructions[3].result.id, tac.instructions[4].rhs.id);
  // Cleanup
  parser_free(&parser);
}

void test_tac_multiple_statements(void) {
  Lexer lexer = tokenize("a = 1\nb = 2\nc = a + b", "test.py");
  Parser parser = parse(&lexer);
  SemanticAnalyzer sa = analyze_program(&parser);

  TACProgram tac = tac_generate(&sa);
  TEST_ASSERT_TRUE(tac.capacity >= tac.count);
  TEST_ASSERT_EQUAL_INT(8, tac.count);
  // Instruction 4: LOAD a
  TEST_ASSERT_EQUAL_INT(TAC_LOAD, tac.instructions[4].op);
  TEST_ASSERT_EQUAL_INT(0, tac.instructions[4].lhs.id); // v0

  // Instruction 5: LOAD b
  TEST_ASSERT_EQUAL_INT(TAC_LOAD, tac.instructions[5].op);
  TEST_ASSERT_EQUAL_INT(1, tac.instructions[5].lhs.id);
  // Cleanup
  parser_free(&parser);
}

void test_tac_unary_minus(void) {
  // Arrange
  Lexer lexer = tokenize("x = -5", "test.py");
  Parser parser = parse(&lexer);
  SemanticAnalyzer sa = analyze_program(&parser);
  // Act
  TACProgram tac = tac_generate(&sa);
  // Assert
  TEST_ASSERT_EQUAL_INT(4, tac.count);
  // 0: CONST 5
  // 1: SUB 0, r0 -> r1
  // 2: STORE r1 -> x
  TEST_ASSERT_EQUAL_INT(TAC_CONST, tac.instructions[0].op);
  TEST_ASSERT_EQUAL_INT(TAC_CONST, tac.instructions[1].op);
  TEST_ASSERT_EQUAL_INT(TAC_SUB, tac.instructions[2].op);
  TEST_ASSERT_EQUAL(tac.instructions[1].result.id, tac.instructions[2].lhs.id);
  TEST_ASSERT_EQUAL_INT(TAC_STORE, tac.instructions[3].op);
  // Cleanup
  parser_free(&parser);
}

void test_tac_if_statement_no_else(void) {
  Lexer lexer = tokenize(
    "x = 1\n"
    "if x:\n"
    "    y = 2\n",
    "test.py"
  );

  Parser parser = parse(&lexer);
  SemanticAnalyzer sa = analyze_program(&parser);
  TACProgram tac = tac_generate(&sa);
  slog_info("%s", tac_generate_code(&tac).items);
  TEST_ASSERT_NOT_NULL(tac.instructions);
  TEST_ASSERT_TRUE(tac.count >= 7);

  // 0: CONST 1
  TEST_ASSERT_EQUAL_INT(TAC_CONST, tac.instructions[0].op);

  // 1: STORE x
  TEST_ASSERT_EQUAL_INT(TAC_STORE, tac.instructions[1].op);

  // 2: LOAD x
  TEST_ASSERT_EQUAL_INT(TAC_LOAD, tac.instructions[2].op);

  // 3: JZ L0
  TEST_ASSERT_EQUAL_INT(TAC_JZ, tac.instructions[3].op);

  // 4: CONST 2
  TEST_ASSERT_EQUAL_INT(TAC_CONST, tac.instructions[4].op);

  // 5: STORE y
  TEST_ASSERT_EQUAL_INT(TAC_STORE, tac.instructions[5].op);

  // 6: LABEL L0
  TEST_ASSERT_EQUAL_INT(TAC_LABEL, tac.instructions[6].op);

  // Jump must target the label
  TEST_ASSERT_EQUAL_STRING(
    tac.instructions[6].label,
    tac.instructions[3].label
  );
  // Cleanup
  parser_free(&parser);
}

#endif // TEST_TAC_H_