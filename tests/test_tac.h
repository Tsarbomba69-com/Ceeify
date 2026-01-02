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
    slog_info("%s", tac_generate_code(&tac).items);
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
    TEST_ASSERT_EQUAL(
        tac.instructions[0].result.id,
        tac.instructions[2].lhs.id
    );
    TEST_ASSERT_EQUAL(
        tac.instructions[1].result.id,
        tac.instructions[2].rhs.id
    );

    TEST_ASSERT_EQUAL_INT(TAC_STORE, tac.instructions[3].op);
    TEST_ASSERT_EQUAL(
        tac.instructions[2].result.id,
        tac.instructions[3].lhs.id
    );

    parser_free(&parser);
}


#endif // TEST_TAC_H_