#ifndef TEST_SEMANTIC_H_
#define TEST_SEMANTIC_H_
#pragma once
#include "semantic.h"
#include <unity.h>

void test_semantic_empty_program(void) {
    // Arrange
    ASTNode_LinkedList *program = NULL; // no nodes
    // Act
    SemanticAnalyzer sa = analyze_program(program);
    // Assert
    TEST_ASSERT_EQUAL(SEM_OK, sa.last_error.type);
    TEST_ASSERT_FALSE(sa_has_error(&sa));
}

void test_semantic_simple_assignment(void) {
    // Arrange
    Lexer lexer = tokenize("x = 5", "test.py");
    Parser parser = parse(&lexer);
    // Act
    SemanticAnalyzer sa = analyze_program(&parser.ast);
    // Assert
    TEST_ASSERT_EQUAL(SEM_OK, sa.last_error.type);
    TEST_ASSERT_FALSE(sa_has_error(&sa));
    // Cleanup
    parser_free(&parser);
}

void test_semantic_undefined_variable(void) {
    // Arrange
    Lexer lexer = tokenize("z = x + 1", "test.py");
    Parser parser = parse(&lexer);
    // Act
    SemanticAnalyzer sa = analyze_program(&parser.ast);
    SemanticError err = sa_get_error(&sa);
    // Assert
    TEST_ASSERT_TRUE(sa_has_error(&sa));
    TEST_ASSERT_EQUAL(SEM_UNDEFINED_VARIABLE, err.type);
    // Cleanup
    parser_free(&parser);
}

#endif // TEST_SEMANTIC_H_