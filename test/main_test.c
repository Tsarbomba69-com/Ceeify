#include "unity.h"
#include "parser.h"

const char *SAMPLES[] = {
        "./test/samples/portal.py",
        "./test/samples/list.py",
        "./test/samples/if_statement.py",
        "./test/samples/while_statement.py",
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
    TEST_ASSERT_GREATER_OR_EQUAL(0, node->while_stmt->body.size);
    // Check the test expression
    TEST_ASSERT_EQUAL(BINARY_OPERATION, node->while_stmt->test->type);
    TEST_ASSERT_EQUAL_STRING("<=", node->while_stmt->test->bin_op->operator);
    TEST_ASSERT_EQUAL(VARIABLE, node->while_stmt->test->bin_op->left->type);
    TEST_ASSERT_EQUAL(LITERAL, node->while_stmt->test->bin_op->right->type);
    TEST_ASSERT_EQUAL(LOAD, node->while_stmt->test->bin_op->left->variable->ctx);
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
    RUN_TEST(test_statement_number);
    RUN_TEST(test_list_size);
    RUN_TEST(test_elif_statement_format);
    RUN_TEST(test_while_statement);
    return UNITY_END();
}
