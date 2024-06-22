#include "unity.h"
#include "parser.h"

const char *SAMPLES[] = {
        "./test/samples/portal.py",
        "./test/samples/list.py",
        "./test/samples/if_statement.py",
        "./test/samples/while_statement.py",
        "./test/samples/for_statement.py",
        "./test/samples/assign_statement.py"
};
char *source = {0};
Lexer lexer = {0};

void test_fullscanning(void) {
    TEST_ASSERT_EQUAL(lexer.sourceLength, lexer.position);
}

void test_statement_number(void) {
    Node_LinkedList program = ParseStatements(&lexer);
    TEST_ASSERT_EQUAL(10, program.size);
}

void test_elif_statement_format(void) {
    source = LoadFileText(SAMPLES[2]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Node_LinkedList program = ParseStatements(&lexer);
    IfStmt *if_stmt = Node_Pop(&program)->if_stmt;
    TEST_ASSERT_GREATER_OR_EQUAL(1, if_stmt->orelse.size);
    Assign const *assign_stmt = Node_Pop(&if_stmt->body)->assign_stmt;
    TEST_ASSERT_EQUAL_STRING("pow", assign_stmt->target->id);
}

void test_list_size(void) {
    source = LoadFileText(SAMPLES[1]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Node_LinkedList program = ParseStatements(&lexer);
    List const *num_list = Node_Pop(&program)->assign_stmt->value->list;
    List const *str_list = Node_Pop(&program)->assign_stmt->value->list;
    TEST_ASSERT_EQUAL(4, num_list->elts.size);
    TEST_ASSERT_EQUAL(3, str_list->elts.size);
}

void test_while_statement(void) {
    source = LoadFileText(SAMPLES[3]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Node_LinkedList program = ParseStatements(&lexer);
    Node const *node = Node_Pop(&program);
    // Perform assertions on the while statement
    TEST_ASSERT_EQUAL(WHILE, node->type);
    TEST_ASSERT_NOT_EQUAL(NULL, node->while_stmt->test);
    TEST_ASSERT_GREATER_OR_EQUAL(1, node->while_stmt->body.size);
    // Check the test expression
    TEST_ASSERT_EQUAL(BINARY_OPERATION, node->while_stmt->test->type);
    TEST_ASSERT_EQUAL_STRING("<=", node->while_stmt->test->bin_op->operator);
    TEST_ASSERT_EQUAL(VARIABLE, node->while_stmt->test->bin_op->left->type);
    TEST_ASSERT_EQUAL(LITERAL, node->while_stmt->test->bin_op->right->type);
    TEST_ASSERT_EQUAL(LOAD, node->while_stmt->test->bin_op->left->variable->ctx);
    // Check the body statement
    Node const *stmt = Node_Pop(&node->while_stmt->body);
    TEST_ASSERT_EQUAL(ASSIGNMENT, stmt->type);
    TEST_ASSERT_EQUAL_STRING("sum", stmt->assign_stmt->target->id);
    TEST_ASSERT_EQUAL(STORE, stmt->assign_stmt->target->ctx);
    TEST_ASSERT_EQUAL(BINARY_OPERATION, stmt->assign_stmt->value->type);
    TEST_ASSERT_EQUAL_STRING("+", stmt->assign_stmt->value->bin_op->operator);
    TEST_ASSERT_EQUAL_STRING("sum", stmt->assign_stmt->value->bin_op->left->variable->id);
    TEST_ASSERT_EQUAL(LOAD, stmt->assign_stmt->value->bin_op->left->variable->ctx);
    TEST_ASSERT_EQUAL(LITERAL, stmt->assign_stmt->value->bin_op->right->type);
    TEST_ASSERT_EQUAL_STRING("1", stmt->assign_stmt->value->bin_op->right->literal->value);
}

void test_for_statement(void) {
    source = LoadFileText(SAMPLES[4]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Node_LinkedList program = ParseStatements(&lexer);
    Node const *node = Node_Pop(&program);
    // Perform assertions on the for statement
    TEST_ASSERT_EQUAL(FOR, node->type);
    TEST_ASSERT_NOT_EQUAL(NULL, node->for_stmt->target.variable);
    TEST_ASSERT_EQUAL(LIST, node->for_stmt->iter->type);
    TEST_ASSERT_EQUAL(4, node->for_stmt->iter->list->elts.size);
    // Check the body statement
    Node const *stmt = Node_Pop(&node->for_stmt->body);
    TEST_ASSERT_EQUAL(ASSIGNMENT, stmt->type);
    TEST_ASSERT_EQUAL_STRING("total", stmt->assign_stmt->target->id);
    TEST_ASSERT_EQUAL(STORE, stmt->assign_stmt->target->ctx);
    TEST_ASSERT_EQUAL(BINARY_OPERATION, stmt->assign_stmt->value->type);
    TEST_ASSERT_EQUAL_STRING("+", stmt->assign_stmt->value->bin_op->operator);
    TEST_ASSERT_EQUAL_STRING("total", stmt->assign_stmt->value->bin_op->left->variable->id);
    TEST_ASSERT_EQUAL(LOAD, stmt->assign_stmt->value->bin_op->left->variable->ctx);
    TEST_ASSERT_EQUAL(VARIABLE, stmt->assign_stmt->value->bin_op->right->type);
    TEST_ASSERT_EQUAL_STRING("n", stmt->assign_stmt->value->bin_op->right->variable->id);
    TEST_ASSERT_EQUAL(LOAD, stmt->assign_stmt->value->bin_op->right->variable->ctx);
}

void test_assign_statement(void) {
    source = LoadFileText(SAMPLES[5]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Node_LinkedList program = ParseStatements(&lexer);
    // Check the assignment statement
    Node const *stmt1 = Node_Pop(&program);
    TEST_ASSERT_EQUAL(ASSIGNMENT, stmt1->type);
    TEST_ASSERT_EQUAL_STRING("a", stmt1->assign_stmt->target->id);
    TEST_ASSERT_EQUAL(STORE, stmt1->assign_stmt->target->ctx);
    TEST_ASSERT_EQUAL(LITERAL, stmt1->assign_stmt->value->type);
    TEST_ASSERT_EQUAL_STRING("1", stmt1->assign_stmt->value->literal->value);
    // Semantics assertion
    TEST_ASSERT_EQUAL(INT, stmt1->assign_stmt->value->literal->type);
    Node const *stmt2 = Node_Pop(&program);
    TEST_ASSERT_EQUAL(ASSIGNMENT, stmt2->type);
    TEST_ASSERT_EQUAL_STRING("c", stmt2->assign_stmt->target->id);
    TEST_ASSERT_EQUAL(STORE, stmt2->assign_stmt->target->ctx);
    TEST_ASSERT_EQUAL(BINARY_OPERATION, stmt2->assign_stmt->value->type);
    TEST_ASSERT_EQUAL_STRING("+", stmt2->assign_stmt->value->bin_op->operator);
    TEST_ASSERT_EQUAL_STRING("a", stmt2->assign_stmt->value->bin_op->left->variable->id);
    TEST_ASSERT_EQUAL(LOAD, stmt2->assign_stmt->value->bin_op->left->variable->ctx);
    TEST_ASSERT_EQUAL(LITERAL, stmt2->assign_stmt->value->bin_op->right->type);
    TEST_ASSERT_EQUAL_STRING("1", stmt2->assign_stmt->value->bin_op->right->literal->value);
}

void setUp(void) {
    source = LoadFileText(SAMPLES[0]);
    if (source == NULL) return;
    lexer = Tokenize(source);
}

void tearDown(void) {
    // Clean up after each test
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_fullscanning);
    RUN_TEST(test_assign_statement);
    RUN_TEST(test_statement_number);
    RUN_TEST(test_list_size);
    RUN_TEST(test_elif_statement_format);
    RUN_TEST(test_while_statement);
    RUN_TEST(test_for_statement);
    return UNITY_END();
}
