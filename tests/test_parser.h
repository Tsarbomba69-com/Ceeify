#pragma once
#ifndef TEST_PARSER_H_
#define TEST_PARSER_H_

#include "parser.h"
#include "utils.h"
#include <unity.h>

void test_parser_single_number(void) {
  Lexer lexer = tokenize("123");
  Parser parser = parse(&lexer);
  ASTNode *node = ASTNode_pop(&parser.ast);
  TEST_ASSERT_EQUAL_INT(LITERAL, node->type);
  TEST_ASSERT_EQUAL_STRING("123", node->token->lexeme);
  ASTNode_free(&parser.ast);
  Token_free(&lexer.tokens);
}

void test_parse_arithmetic_expression(void) {
  Lexer lexer = tokenize("3 + 5 * 2");
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
  ASTNode_free(&parser.ast);
  Token_free(&lexer.tokens);
}

// Test parsing a variable assignment
void test_parse_variable_assignment(void) {
  Lexer lexer = tokenize("x = 42");
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
  astnode_free(node);
  parser_free(&parser);
}

void test_parse_multiple_variable_assignment(void) {
  Lexer lexer = tokenize("x, y, z = 42");
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
  astnode_free(node);
  parser_free(&parser);
}

void test_import_assignment(void) {
  Lexer lexer = tokenize("import x,y,z");
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
  astnode_free(node);
  parser_free(&parser);
}

void test_compare_expression(void) {
  Lexer lexer = tokenize("1 <= a < 10");
  Parser parser = parse(&lexer);
  cJSON *root = serialize_program(&parser.ast);
  char *root_str = cJSON_Print(root);
  ASTNode *node = ASTNode_pop(&parser.ast);
  trace_log(LOG_INFO, "%s", root_str);
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(COMPARE, node->type);
  ASTNode *compare = ASTNode_pop(&node->compare.comparators);
  TEST_ASSERT_EQUAL_STRING("a", compare->token->lexeme);
  free(root_str);
  cJSON_Delete(root);
  astnode_free(node);
  astnode_free(compare);
  parser_free(&parser);
}

void test_if_statement(void) {
  // Example source: if x < 10 then y = 5 end
  Lexer lexer = tokenize("if x < 10\n  y = 5\nend");
  Parser parser = parse(&lexer);

  ASTNode *node = ASTNode_pop(&parser.ast);
  TEST_ASSERT_NOT_NULL(node);
  TEST_ASSERT_EQUAL_INT(IF, node->type);

  // The condition of the if should be a compare expression
  ASTNode *condition = node->if_stmt.test;  // assuming parser uses left for condition
  TEST_ASSERT_NOT_NULL(condition);
  TEST_ASSERT_EQUAL_INT(COMPARE, condition->type);

  // Check left side of compare (x) and right side (10)
  TEST_ASSERT_EQUAL_STRING("x", condition->compare.left->token->lexeme);
  ASTNode *comp = ASTNode_pop(&condition->compare.comparators);
  TEST_ASSERT_EQUAL_STRING("10", comp->token->lexeme);

  // The body should be stored on the right
  ASTNode *body = ASTNode_pop(&node->if_stmt.body);
  TEST_ASSERT_NOT_NULL(body);
  TEST_ASSERT_EQUAL_INT(ASSIGNMENT, body->type);
  TEST_ASSERT_EQUAL_STRING("y", ASTNode_pop(&body->assign.targets)->token->lexeme);
  TEST_ASSERT_EQUAL_STRING("5", body->assign.value->token->lexeme);

  astnode_free(node);
  astnode_free(comp);
  parser_free(&parser);
}

#endif