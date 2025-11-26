#pragma once
#ifndef TEST_PARSER_H_
#define TEST_PARSER_H_

#include "parser.h"
#include "utils.h"
#include <unity.h>

void test_parser_single_number(void) {
  Lexer lexer = tokenize("123", "test_file.py");
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);
  TEST_ASSERT_EQUAL_INT(LITERAL, node->type);
  TEST_ASSERT_EQUAL_STRING("123", node->token->lexeme);
  ASTNode_free(&parser.ast);
  Token_free(&lexer.tokens);
}

void test_parse_arithmetic_expression(void) {
  Lexer lexer = tokenize("3 + 5 * 2", "test_file.py");
  Parser parser = parse(&lexer);
  ASTNode *result = ASTNode_pop(&parser.ast);
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("+", result->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("3", result->bin_op.left->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("*", result->bin_op.right->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("5",
                           result->bin_op.right->bin_op.left->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("2",
                           result->bin_op.right->bin_op.right->token->lexeme);
  parser_free(&parser);
}

// Test parsing a variable assignment
void test_parse_variable_assignment(void) {
  Lexer lexer = tokenize("x = 42", "test_file.py");
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);
  ASTNode *target = ASTNode_pop(&node->assign.targets);
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(ASSIGNMENT, node->type);
  TEST_ASSERT_EQUAL_INT(VARIABLE, target->type);
  TEST_ASSERT_EQUAL_INT(LITERAL, node->assign.value->type);
  TEST_ASSERT_EQUAL_STRING("=", node->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("x", target->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("42", node->assign.value->token->lexeme);
  parser_free(&parser);
}

void test_parse_multiple_variable_assignment(void) {
  Lexer lexer = tokenize("x, y, z = 42", "test_file.py");
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);

  for (size_t current = node->assign.targets.head; current != SIZE_MAX;
       current = node->assign.targets.elements[current].next) {
    ASTNode *el = node->assign.targets.elements[current].data;
    TEST_ASSERT_EQUAL_INT(VARIABLE, el->type);
  }

  ASTNode *target = ASTNode_pop(&node->assign.targets);
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(ASSIGNMENT, node->type);
  TEST_ASSERT_EQUAL_INT(LITERAL, node->assign.value->type);
  TEST_ASSERT_EQUAL_STRING("=", node->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("z", target->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("42", node->assign.value->token->lexeme);
  parser_free(&parser);
}

void test_import_assignment(void) {
  Lexer lexer = tokenize("import x,y,z", "test_file.py");
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(IMPORT, node->type);

  for (size_t current = node->assign.targets.head; current != SIZE_MAX;
       current = node->assign.targets.elements[current].next) {
    ASTNode *el = node->assign.targets.elements[current].data;
    TEST_ASSERT_EQUAL_INT(VARIABLE, el->type);
  }

  ASTNode *name = ASTNode_pop(&node->import);
  TEST_ASSERT_EQUAL_STRING("z", name->token->lexeme);
  parser_free(&parser);
}

void test_compare_expression(void) {
  Lexer lexer = tokenize("1 <= a < 10", "test_file.py");
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(COMPARE, node->type);
  ASTNode *compare = ASTNode_pop(&node->compare.comparators);
  TEST_ASSERT_EQUAL_STRING("a", compare->token->lexeme);
  parser_free(&parser);
}

void test_if_statement(void) {
  Lexer lexer = tokenize("if x < 10:\n"
                          "\ty = 5\n", "test_file.py");
  Parser parser = parse(&lexer);

  ASTNode *node = ASTNode_pop(&parser.ast);
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(IF, node->type);
  // Type assert
  ASTNode *condition = node->ctrl_stmt.test;
  TEST_ASSERT_NOT_NULL(condition);
  TEST_ASSERT_EQUAL_INT(COMPARE, condition->type);

  // Check left side of compare (x) and right side (10)
  TEST_ASSERT_EQUAL_STRING("x", condition->compare.left->token->lexeme);
  ASTNode *comp = ASTNode_pop(&condition->compare.comparators);
  TEST_ASSERT_EQUAL_STRING("10", comp->token->lexeme);

  // The body should be stored on the right
  ASTNode *body = ASTNode_pop(&node->ctrl_stmt.body);
  ASTNode *y = ASTNode_pop(&body->assign.targets);
  TEST_ASSERT_NOT_NULL(body);
  TEST_ASSERT_EQUAL_INT(ASSIGNMENT, body->type);
  TEST_ASSERT_EQUAL_STRING("y", y->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("5", body->assign.value->token->lexeme);
  // cleanup
  parser_free(&parser);
}

void test_if_elif_statement(void) {
  Lexer lexer = tokenize("if x < 10:\n"
                         "  y = 5\n"
                         "elif x < 20:\n"
                         "  y = 15\n", "test_file.py");

  Parser parser = parse(&lexer);

  // Pop the top-level node
  ASTNode *node = ASTNode_pop(&parser.ast);
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(IF, node->type);

  //
  // --- check main IF block ---
  //
  ASTNode *condition = node->ctrl_stmt.test;
  TEST_ASSERT_NOT_NULL(condition);
  TEST_ASSERT_EQUAL_INT(COMPARE, condition->type);
  TEST_ASSERT_EQUAL_STRING("x", condition->compare.left->token->lexeme);
  ASTNode *comp = ASTNode_pop(&condition->compare.comparators);
  TEST_ASSERT_EQUAL_STRING("10", comp->token->lexeme);

  // check if-body: y = 5
  ASTNode *body_stmt = ASTNode_pop(&node->ctrl_stmt.body);
  TEST_ASSERT_NOT_NULL(body_stmt);
  TEST_ASSERT_EQUAL_INT(ASSIGNMENT, body_stmt->type);
  ASTNode *target = ASTNode_pop(&body_stmt->assign.targets);
  TEST_ASSERT_EQUAL_STRING("y", target->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("5", body_stmt->assign.value->token->lexeme);

  //
  // --- check ELIF block (should appear in orelse list) ---
  //
  ASTNode *elif_node = ASTNode_pop(&node->ctrl_stmt.orelse);
  TEST_ASSERT_NOT_NULL(elif_node);
  TEST_ASSERT_EQUAL_INT(IF, elif_node->type);

  // condition: x < 20
  ASTNode *elif_condition = elif_node->ctrl_stmt.test;
  TEST_ASSERT_NOT_NULL(elif_condition);
  TEST_ASSERT_EQUAL_INT(COMPARE, elif_condition->type);
  TEST_ASSERT_EQUAL_STRING("x", elif_condition->compare.left->token->lexeme);
  ASTNode *elif_comp = ASTNode_pop(&elif_condition->compare.comparators);
  TEST_ASSERT_EQUAL_STRING("20", elif_comp->token->lexeme);

  // body: y = 15
  ASTNode *elif_body_stmt = ASTNode_pop(&elif_node->ctrl_stmt.body);
  TEST_ASSERT_NOT_NULL(elif_body_stmt);
  TEST_ASSERT_EQUAL_INT(ASSIGNMENT, elif_body_stmt->type);
  ASTNode *elif_target = ASTNode_pop(&elif_body_stmt->assign.targets);
  TEST_ASSERT_EQUAL_STRING("y", elif_target->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("15", elif_body_stmt->assign.value->token->lexeme);

  // cleanup
  parser_free(&parser);
}

void test_if_else_statement(void) {
  Lexer lexer = tokenize("if x < 10:\n"
                         "  y = 5\n"
                         "else:\n"
                         "  y = 100\n", "test_file.py");

  Parser parser = parse(&lexer);

  // Pop the top-level node
  ASTNode *node = ASTNode_pop(&parser.ast);
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(IF, node->type);

  //
  // --- check IF condition ---
  //
  ASTNode *condition = node->ctrl_stmt.test;
  TEST_ASSERT_NOT_NULL(condition);
  TEST_ASSERT_EQUAL_INT(COMPARE, condition->type);
  TEST_ASSERT_EQUAL_STRING("x", condition->compare.left->token->lexeme);
  ASTNode *comp = ASTNode_pop(&condition->compare.comparators);
  TEST_ASSERT_EQUAL_STRING("10", comp->token->lexeme);

  //
  // --- check IF body: y = 5 ---
  //
  ASTNode *body_stmt = ASTNode_pop(&node->ctrl_stmt.body);
  TEST_ASSERT_NOT_NULL(body_stmt);
  TEST_ASSERT_EQUAL_INT(ASSIGNMENT, body_stmt->type);
  ASTNode *target = ASTNode_pop(&body_stmt->assign.targets);
  TEST_ASSERT_EQUAL_STRING("y", target->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("5", body_stmt->assign.value->token->lexeme);

  //
  // --- check ELSE block ---
  //
  ASTNode *else_stmt = ASTNode_pop(&node->ctrl_stmt.orelse);
  TEST_ASSERT_NOT_NULL(else_stmt);

  // `else` is NOT another IF node — it is a normal statement
  TEST_ASSERT_EQUAL_INT(ASSIGNMENT, else_stmt->type);

  // expected: y = 100
  ASTNode *else_target = ASTNode_pop(&else_stmt->assign.targets);
  TEST_ASSERT_EQUAL_STRING("y", else_target->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("100", else_stmt->assign.value->token->lexeme);

  // cleanup
  parser_free(&parser);
}

void test_while_statement(void) {
  // Arrange
  Lexer lexer = tokenize("while x < 10:\n"
                         "  x = x + 1\n", "test_file.py");
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(WHILE, node->type);
  // Assert
  ASTNode *condition = node->ctrl_stmt.test;
  TEST_ASSERT_NOT_NULL(condition);
  TEST_ASSERT_EQUAL_INT(COMPARE, condition->type);
  TEST_ASSERT_EQUAL_STRING("x", condition->compare.left->token->lexeme);
  ASTNode *comp = ASTNode_pop(&condition->compare.comparators);
  TEST_ASSERT_EQUAL_STRING("10", comp->token->lexeme);

  //
  // --- check WHILE body: x = x + 1 ---
  //
  ASTNode *body_stmt = ASTNode_pop(&node->ctrl_stmt.body);
  TEST_ASSERT_NOT_NULL(body_stmt);
  TEST_ASSERT_EQUAL_INT(ASSIGNMENT, body_stmt->type);
  ASTNode *target = ASTNode_pop(&body_stmt->assign.targets);
  TEST_ASSERT_EQUAL_STRING("x", target->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("1", body_stmt->assign.value->bin_op.right->token->lexeme);

  // cleanup
  parser_free(&parser);
}

#endif