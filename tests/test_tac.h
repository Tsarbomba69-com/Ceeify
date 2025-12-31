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
    // TEST_ASSERT_NOT_EQUAL(0, inst1->result.id); // Should have assigned a virtual register
    // 3. Second instruction: STORE to x
    TACInstruction *inst2 = &tac.instructions[1];
    TEST_ASSERT_EQUAL_INT(TAC_STORE, inst2->op);
    TEST_ASSERT_EQUAL_INT(INT, inst2->lhs.type); // Source type
    TEST_ASSERT_EQUAL(inst1->result.id, inst2->lhs.id); // Should use register from CONST
    // 4. Verify no memory leaks in TAC generation
    TEST_ASSERT_TRUE(tac.capacity >= tac.count);
    // Should generate something like:
    // %0 = CONST 5 (INT)
    // STORE %0 -> x
    StringBuilder sb = tac_generate_code(&tac);
    slog_info("Generated TAC Code:\n%.*s", (int)sb.count, sb.items);
    // Cleanup
    parser_free(&parser);
}

#endif // TEST_TAC_H_