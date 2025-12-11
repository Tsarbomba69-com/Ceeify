#ifndef TEST_SEMANTIC_H_
#define TEST_SEMANTIC_H_
#pragma once
#include "semantic.h"
#include <unity.h>

void test_semantic_empty_program(void) {
    // Arrange
    Parser parser = {0};
    // Act
    SemanticAnalyzer sa = analyze_program(&parser);
    SemanticError err = sa_get_error(&sa);
    // Assert
    TEST_ASSERT_EQUAL(SEM_OK, err.type);
    TEST_ASSERT_FALSE(sa_has_error(&sa));
}

void test_semantic_simple_assignment(void) {
    // Arrange
    Lexer lexer = tokenize("x = 5", "test.py");
    Parser parser = parse(&lexer);
    // Act
    SemanticAnalyzer sa = analyze_program(&parser);
    SemanticError err = sa_get_error(&sa);
    // Assert
    TEST_ASSERT_EQUAL(SEM_OK, err.type);
    TEST_ASSERT_FALSE(sa_has_error(&sa));
    // Cleanup
    parser_free(&parser);
}

void test_semantic_undefined_variable(void) {
    // Arrange
    Lexer lexer = tokenize("z = x + 1", "test.py");
    Parser parser = parse(&lexer);
    // Act
    SemanticAnalyzer sa = analyze_program(&parser);
    SemanticError err = sa_get_error(&sa);
    // Assert
    TEST_ASSERT_TRUE(sa_has_error(&sa));
    TEST_ASSERT_EQUAL(SEM_UNDEFINED_VARIABLE, err.type);
    // Cleanup
    parser_free(&parser);
}

void test_semantic_type_mismatch(void) {
    // Arrange
    Lexer lexer = tokenize("x = 1\n"
                           "y = x + \"hello\"", "test.py");
    Parser parser = parse(&lexer);
    // Act
    SemanticAnalyzer sa = analyze_program(&parser);
    SemanticError err = sa_get_error(&sa);
    // Assert
    TEST_ASSERT_TRUE(sa_has_error(&sa));
    TEST_ASSERT_EQUAL(SEM_TYPE_MISMATCH, err.type);
    // Cleanup
    parser_free(&parser);
}

void test_semantic_invalid_operation(void) {
    // Arrange
    Lexer lexer = tokenize(
        "x = 5\n"
        "y = x and 10", 
        "test.py"
    );
    Parser parser = parse(&lexer);

    // Act
    SemanticAnalyzer sa = analyze_program(&parser);
    SemanticError err = sa_get_error(&sa);

    // Assert
    TEST_ASSERT_TRUE(sa_has_error(&sa));
    TEST_ASSERT_EQUAL(SEM_TYPE_MISMATCH, err.type);

    parser_free(&parser);
}

void test_semantic_function_declaration(void) {
    // Arrange
    Lexer lexer = tokenize(
        "def add(x, y):\n"
        "    return x + y\n",
        "test_file.py"
    );
    Parser parser = parse(&lexer);
    // Act
    SemanticAnalyzer sa = analyze_program(&parser);
    SemanticError err = sa_get_error(&sa);
    TEST_ASSERT_FALSE(sa_has_error(&sa));
    TEST_ASSERT_EQUAL(SEM_OK, err.type);
    Symbol *fn = sa_lookup(&sa, "add");
    TEST_ASSERT_NOT_NULL(fn);
    TEST_ASSERT_EQUAL(FUNCTION, fn->kind);
    TEST_ASSERT_NOT_NULL(fn->decl_node);
    sa_enter_scope(&sa);
    Symbol *px = sa_lookup(&sa, "x");
    Symbol *py = sa_lookup(&sa, "y");
    TEST_ASSERT_NOT_NULL(px);
    TEST_ASSERT_NOT_NULL(py);
    TEST_ASSERT_EQUAL(VAR, px->kind);
    TEST_ASSERT_EQUAL(VAR, py->kind);
    // Cleanup
    parser_free(&parser);
}


#endif // TEST_SEMANTIC_H_