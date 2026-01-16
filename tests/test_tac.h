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
  // Arrange
  Lexer lexer = tokenize("x = 1\n"
                         "if x:\n"
                         "    y = 2\n",
                         "test.py");

  Parser parser = parse(&lexer);
  SemanticAnalyzer sa = analyze_program(&parser);
  // Act
  TACProgram tac = tac_generate(&sa);
  // Assert
  TEST_ASSERT_NOT_NULL(tac.instructions);
  TEST_ASSERT_TRUE(tac.count >= 8);

  // 0: CONST 1
  TEST_ASSERT_EQUAL_INT(TAC_CONST, tac.instructions[0].op);

  // 1: STORE x
  TEST_ASSERT_EQUAL_INT(TAC_STORE, tac.instructions[1].op);

  // 2: LABEL 
  TEST_ASSERT_EQUAL_INT(TAC_LABEL, tac.instructions[2].op);
  
  // 3: LOAD x
  TEST_ASSERT_EQUAL_INT(TAC_LOAD, tac.instructions[3].op);
  
  // 3: JZ L0
  TEST_ASSERT_EQUAL_INT(TAC_JZ, tac.instructions[4].op);

  // 4: CONST 2
  TEST_ASSERT_EQUAL_INT(TAC_CONST, tac.instructions[5].op);

  // 5: STORE y
  TEST_ASSERT_EQUAL_INT(TAC_LABEL, tac.instructions[6].op);

  // 6: LABEL L0
  TEST_ASSERT_EQUAL_INT(TAC_RETURN, tac.instructions[7].op);

  // Jump must target the label
  TEST_ASSERT_EQUAL_STRING(tac.instructions[6].label,
                           tac.instructions[4].label);
  // Cleanup
  parser_free(&parser);
}

void test_tac_operator_precedence(void) {
  Lexer lexer = tokenize("x = 1 + 2 * 3", "test.py");
  Parser parser = parse(&lexer);
  SemanticAnalyzer sa = analyze_program(&parser);

  TACProgram tac = tac_generate(&sa);
  // Expected:
  // CONST 1
  // CONST 2
  // CONST 3
  // MUL r1, r2 -> r3
  // ADD r0, r3 -> r4
  // STORE r4 -> x
  TEST_ASSERT_EQUAL_INT(6, tac.count);

  TEST_ASSERT_EQUAL_INT(TAC_MUL, tac.instructions[3].op);
  TEST_ASSERT_EQUAL_INT(TAC_ADD, tac.instructions[4].op);
  TEST_ASSERT_EQUAL_INT(TAC_STORE, tac.instructions[5].op);

  parser_free(&parser);
}

void test_tac_parenthesized_expression(void) {
  Lexer lexer = tokenize("x = (1 + 2) * 3", "test.py");
  Parser parser = parse(&lexer);
  SemanticAnalyzer sa = analyze_program(&parser);

  TACProgram tac = tac_generate(&sa);

  // ADD must happen before MUL
  TEST_ASSERT_EQUAL_INT(TAC_ADD, tac.instructions[2].op);
  TEST_ASSERT_EQUAL_INT(TAC_MUL, tac.instructions[4].op);

  parser_free(&parser);
}

void test_tac_if_else_statement(void) {
  Lexer lexer = tokenize("x = 1\n"
                         "if x:\n"
                         "    y = 2\n"
                         "else:\n"
                         "    y = 3\n",
                         "test.py");

  Parser parser = parse(&lexer);
  SemanticAnalyzer sa = analyze_program(&parser);
  TACProgram tac = tac_generate(&sa);
  int32_t jz = -1, jmp = -1;
  const char *jz_label = NULL;
  const char *jmp_label = NULL;
  int label_count = 0;

  for (size_t i = 0; i < tac.count; i++) {
    TACInstruction *in = &tac.instructions[i];

    if (in->op == TAC_JZ || in->op == TAC_CJMP) {
      jz = (int32_t)i;
      jz_label = in->label;
    }

    if (in->op == TAC_JMP) {
      jmp = (int32_t)i;
      jmp_label = in->label;
    }

    if (in->op == TAC_LABEL) {
      label_count++;
    }
  }

  TEST_ASSERT_TRUE(jz >= 0);
  TEST_ASSERT_TRUE(jmp >= 0);
  TEST_ASSERT_TRUE(label_count == 3);

  // Labels must exist
  TEST_ASSERT_NOT_NULL(jz_label);
  TEST_ASSERT_NOT_NULL(jmp_label);

  // Labels must be different
  TEST_ASSERT_NOT_EQUAL(jz_label, jmp_label);

  // Both jump targets must be defined labels
  int jz_label_found = 0, jmp_label_found = 0;

  for (size_t i = 0; i < tac.count; i++) {
    if (tac.instructions[i].op == TAC_LABEL) {
      if (strcmp(tac.instructions[i].label, jz_label) == 0)
        jz_label_found = 1;
      if (strcmp(tac.instructions[i].label, jmp_label) == 0)
        jmp_label_found = 1;
    }
  }

  TEST_ASSERT_TRUE(jz_label_found);
  TEST_ASSERT_TRUE(jmp_label_found);
  // Cleanup
  parser_free(&parser);
}

#endif // TEST_TAC_H_