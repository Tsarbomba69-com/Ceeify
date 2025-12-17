#pragma once
#ifndef TEST_PARSER_H_
#define TEST_PARSER_H_

#include "parser.h"
#include "utils.h"
#include <unity.h>

void test_parser_single_number(void) {
  // Arrane
  Lexer lexer = tokenize("123", "test_file.py");
  // Act
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);
  // Assert
  TEST_ASSERT_EQUAL_INT(LITERAL, node->type);
  TEST_ASSERT_EQUAL_STRING("123", node->token->lexeme);
  // Cleanup
  parser_free(&parser);
}

void test_parse_arithmetic_expression(void) {
  // Arrange
  Lexer lexer = tokenize("3 + 5 * 2", "test_file.py");
  // Act
  Parser parser = parse(&lexer);
  ASTNode *result = ASTNode_pop(&parser.ast);
  // Assert
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("+", result->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("3", result->bin_op.left->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("*", result->bin_op.right->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("5",
                           result->bin_op.right->bin_op.left->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("2",
                           result->bin_op.right->bin_op.right->token->lexeme);
  // Cleanup
  parser_free(&parser);
}

void test_expression_parentheses_override_precedence(void) {
  Lexer lexer = tokenize("(3 + 5) * 2", "test_file.py");
  Parser parser = parse(&lexer);

  ASTNode *expr = ASTNode_pop(&parser.ast);
  TEST_ASSERT_EQUAL_INT(BINARY_OPERATION, expr->type);
  TEST_ASSERT_EQUAL_STRING("*", expr->token->lexeme);

  // left: (3 + 5)
  ASTNode *left = expr->bin_op.left;
  TEST_ASSERT_EQUAL_STRING("+", left->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("3", left->bin_op.left->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("5", left->bin_op.right->token->lexeme);

  // right: 2
  TEST_ASSERT_EQUAL_STRING("2", expr->bin_op.right->token->lexeme);

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
  // Arrange
  Lexer lexer = tokenize("1 <= a < 10", "test_file.py");
  // Act
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);
  // Assert
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(COMPARE, node->type);
  ASTNode *compare = ASTNode_pop(&node->compare.comparators);
  TEST_ASSERT_EQUAL_STRING("10", compare->token->lexeme);
  compare = ASTNode_pop(&node->compare.comparators);
  TEST_ASSERT_EQUAL_STRING("a", compare->token->lexeme);
  parser_free(&parser);
}

void test_if_statement(void) {
  Lexer lexer = tokenize("if x < 10:\n"
                         "\ty = 5\n",
                         "test_file.py");
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
                         "  y = 15\n",
                         "test_file.py");

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
                         "  y = 100\n",
                         "test_file.py");

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
                         "  x = x + 1\n",
                         "test_file.py");
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
  TEST_ASSERT_EQUAL_STRING(
      "1", body_stmt->assign.value->bin_op.right->token->lexeme);

  // cleanup
  parser_free(&parser);
}

void test_while_else_statement(void) {
  // Arrange
  Lexer lexer = tokenize("while x < 10:\n"
                         "  x = x + 1\n"
                         "else:\n"
                         "  y = 100\n",
                         "test_file.py");
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(WHILE, node->type);

  // Check WHILE condition
  ASTNode *condition = node->ctrl_stmt.test;
  TEST_ASSERT_NOT_NULL(condition);
  TEST_ASSERT_EQUAL_INT(COMPARE, condition->type);
  TEST_ASSERT_EQUAL_STRING("x", condition->compare.left->token->lexeme);
  ASTNode *comp = ASTNode_pop(&condition->compare.comparators);
  TEST_ASSERT_EQUAL_STRING("10", comp->token->lexeme);

  // Check WHILE body: x = x + 1
  ASTNode *body_stmt = ASTNode_pop(&node->ctrl_stmt.body);
  TEST_ASSERT_NOT_NULL(body_stmt);
  TEST_ASSERT_EQUAL_INT(ASSIGNMENT, body_stmt->type);
  ASTNode *target = ASTNode_pop(&body_stmt->assign.targets);
  TEST_ASSERT_EQUAL_STRING("x", target->token->lexeme);
  TEST_ASSERT_EQUAL_STRING(
      "1", body_stmt->assign.value->bin_op.right->token->lexeme);

  // Check ELSE block
  ASTNode *else_stmt = ASTNode_pop(&node->ctrl_stmt.orelse);
  TEST_ASSERT_NOT_NULL(else_stmt);
  TEST_ASSERT_EQUAL_INT(ASSIGNMENT, else_stmt->type);

  ASTNode *else_target = ASTNode_pop(&else_stmt->assign.targets);
  TEST_ASSERT_EQUAL_STRING("y", else_target->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("100", else_stmt->assign.value->token->lexeme);

  // Cleanup
  parser_free(&parser);
}

void test_parse_augmented_assignment(void) {
  // Arrange & Act
  Lexer lexer = tokenize("a += 69", "test_file.py");
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);
  // Assert
  TEST_ASSERT_NOT_NULL(node);
  // Check node type: augmented assignment
  TEST_ASSERT_EQUAL_INT(AUG_ASSIGNMENT, node->type);
  TEST_ASSERT_NOT_NULL(node->token);
  TEST_ASSERT_EQUAL_STRING("+=", node->token->lexeme);

  // Check the target (left side)
  ASTNode *target = node->aug_assign.target;
  TEST_ASSERT_NOT_NULL(target);
  TEST_ASSERT_EQUAL_INT(VARIABLE, target->type);
  TEST_ASSERT_EQUAL_STRING("a", target->token->lexeme);

  // Check the value (right side)
  TEST_ASSERT_NOT_NULL(node->aug_assign.value);
  TEST_ASSERT_EQUAL_INT(LITERAL, node->aug_assign.value->type);
  TEST_ASSERT_EQUAL_STRING("69", node->aug_assign.value->token->lexeme);
  parser_free(&parser);
}

void test_parse_function_declaration(void) {
  // Arrange
  Lexer lexer = tokenize("def add(x, y):\n"
                         "    return x + y\n",
                         "test_file.py");
  // Act
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);
  // Assert
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(FUNCTION_DEF, node->type);
  // Function name
  TEST_ASSERT_NOT_NULL(node->token);
  TEST_ASSERT_EQUAL_STRING("add", node->funcdef.name->token->lexeme);
  //
  // ---- PARAMETERS ----
  //
  ASTNode *param_y = ASTNode_pop(&node->funcdef.params);
  TEST_ASSERT_NOT_NULL(param_y);
  TEST_ASSERT_EQUAL_INT(VARIABLE, param_y->type);
  TEST_ASSERT_EQUAL_STRING("y", param_y->token->lexeme);
  ASTNode *param_x = ASTNode_pop(&node->funcdef.params);
  TEST_ASSERT_NOT_NULL(param_x);
  TEST_ASSERT_EQUAL_INT(VARIABLE, param_x->type);
  TEST_ASSERT_EQUAL_STRING("x", param_x->token->lexeme);
  //
  // ---- BODY ----
  //
  ASTNode *body_stmt = ASTNode_pop(&node->funcdef.body);
  TEST_ASSERT_NOT_NULL(body_stmt);
  // Body should begin with a return statement
  TEST_ASSERT_EQUAL_INT(RETURN, body_stmt->type);
  TEST_ASSERT_NOT_NULL(body_stmt->token); // "return"
  //
  // return expression: x + y
  //
  ASTNode *ret_expr = body_stmt->bin_op.left; // or body_stmt->return.value
  TEST_ASSERT_NOT_NULL(ret_expr);
  TEST_ASSERT_EQUAL_INT(BINARY_OPERATION, ret_expr->type);
  TEST_ASSERT_EQUAL_STRING("+", ret_expr->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("x", ret_expr->bin_op.left->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("y", ret_expr->bin_op.right->token->lexeme);
  parser_free(&parser);
}

void test_parse_function_call(void) {
  // Arrange
  Lexer lexer = tokenize("add(1, 2)", "test_file.py");
  // Act
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);
  // Assert: CALL node
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(CALL, node->type);
  // Callee name
  TEST_ASSERT_NOT_NULL(node->token);
  TEST_ASSERT_EQUAL_STRING("add", node->token->lexeme);
  ASTNode *arg2 = ASTNode_pop(&node->call.args);
  ASTNode *arg1 = ASTNode_pop(&node->call.args);
  // Check arguments
  TEST_ASSERT_NOT_NULL(arg1);
  TEST_ASSERT_NOT_NULL(arg2);
  TEST_ASSERT_EQUAL_INT(LITERAL, arg1->type);
  TEST_ASSERT_EQUAL_STRING("1", arg1->token->lexeme);
  TEST_ASSERT_EQUAL_INT(LITERAL, arg2->type);
  TEST_ASSERT_EQUAL_STRING("2", arg2->token->lexeme);
  // Cleanup
  parser_free(&parser);
}

void test_parse_function_call_no_args(void) {
  // Arrange
  Lexer lexer = tokenize("foo()", "test_file.py");
  // Act
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);
  // Assert
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(CALL, node->type);
  // function name
  TEST_ASSERT_EQUAL_STRING("foo", node->call.func->token->lexeme);
  // args should be empty
  TEST_ASSERT_EQUAL_INT(SIZE_MAX, node->call.args.head);
  // Cleanup
  parser_free(&parser);
}

void test_parse_nested_function_call(void) {
  // Arrange
  Lexer lexer = tokenize("foo(bar(1))", "test_file.py");
  // Act
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);
  // Assert
  TEST_ASSERT_EQUAL_INT(CALL, node->type);
  TEST_ASSERT_EQUAL_STRING("foo", node->call.func->token->lexeme);
  ASTNode *inner = ASTNode_pop(&node->call.args);
  TEST_ASSERT_NOT_NULL(inner);
  TEST_ASSERT_EQUAL_INT(CALL, inner->type);
  TEST_ASSERT_EQUAL_STRING("bar", inner->call.func->token->lexeme);
  ASTNode *inner_arg = ASTNode_pop(&inner->call.args);
  TEST_ASSERT_EQUAL_STRING("1", inner_arg->token->lexeme);
  parser_free(&parser);
}

void test_parse_call_inside_expression(void) {
  Lexer lexer = tokenize("x = foo(1) + 2", "test_file.py");
  Parser parser = parse(&lexer);

  ASTNode *assign = ASTNode_pop(&parser.ast);
  TEST_ASSERT_EQUAL_INT(ASSIGNMENT, assign->type);

  ASTNode *expr = assign->assign.value;
  TEST_ASSERT_EQUAL_INT(BINARY_OPERATION, expr->type);
  TEST_ASSERT_EQUAL_STRING("+", expr->token->lexeme);

  ASTNode *call = expr->bin_op.left;
  TEST_ASSERT_EQUAL_INT(CALL, call->type);
  TEST_ASSERT_EQUAL_STRING("foo", call->call.func->token->lexeme);

  ASTNode *arg = ASTNode_pop(&call->call.args);
  TEST_ASSERT_EQUAL_STRING("1", arg->token->lexeme);

  parser_free(&parser);
}

#endif