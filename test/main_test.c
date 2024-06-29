#include "unity.h"
#include "parser.h"
#include "hashtable_test.h"

const char *SAMPLES[] = {
        "./test/samples/portal.py",
        "./test/samples/list.py",
        "./test/samples/if_statement.py",
        "./test/samples/while_statement.py",
        "./test/samples/for_statement.py",
        "./test/samples/assign_statement.py",
        "./test/samples/binary_operation_type.py"
};
#define OUTPUT_PATH "./test/output"

char *source = {0};
Lexer lexer = {0};

void test_fullscanning(void) {
    TEST_ASSERT_EQUAL(lexer.sourceLength, lexer.position);
}

void test_statement_number(void) {
    Parser parser = CreateParser(lexer, Symbol_CreateHashTable(10));
    Node_LinkedList program = ParseStatements(&parser);
    TEST_ASSERT_EQUAL(11, program.size);
}

void test_elif_statement_format(void) {
    source = LoadFileText(SAMPLES[2]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer, Symbol_CreateHashTable(10));
    Node_LinkedList program = ParseStatements(&parser);
    Node_Pop(&program);
    Node_Pop(&program);
    IfStmt *ifStmt = Node_Pop(&program)->ifStmt;
    TEST_ASSERT_GREATER_OR_EQUAL(1, ifStmt->orelse.size);
    Assign const *assign_stmt = Node_Pop(&ifStmt->body)->assignStmt;
    TEST_ASSERT_EQUAL_STRING("pow", assign_stmt->target->id);
}

void test_list_size(void) {
    source = LoadFileText(SAMPLES[1]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer, Symbol_CreateHashTable(10));
    Node_LinkedList program = ParseStatements(&parser);
    List const *num_list = Node_Pop(&program)->assignStmt->value->list;
    List const *str_list = Node_Pop(&program)->assignStmt->value->list;
    TEST_ASSERT_EQUAL(4, num_list->elts.size);
    TEST_ASSERT_EQUAL(3, str_list->elts.size);
}

void test_while_statement(void) {
    source = LoadFileText(SAMPLES[3]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer, Symbol_CreateHashTable(10));
    Node_LinkedList program = ParseStatements(&parser);
    Node_Pop(&program);
    Node const *node = Node_Pop(&program);
    // Perform assertions on the while statement
    TEST_ASSERT_EQUAL(WHILE, node->type);
    TEST_ASSERT_NOT_EQUAL(NULL, node->whileStmt->test);
    TEST_ASSERT_GREATER_OR_EQUAL(1, node->whileStmt->body.size);
    // Check the test expression
    TEST_ASSERT_EQUAL(BINARY_OPERATION, node->whileStmt->test->type);
    TEST_ASSERT_EQUAL_STRING("<=", node->whileStmt->test->binOp->operator);
    TEST_ASSERT_EQUAL(VARIABLE, node->whileStmt->test->binOp->left->type);
    TEST_ASSERT_EQUAL(LITERAL, node->whileStmt->test->binOp->right->type);
    TEST_ASSERT_EQUAL(LOAD, node->whileStmt->test->binOp->left->variable->ctx);
    // Check the body statement
    Node const *stmt = Node_Pop(&node->whileStmt->body);
    TEST_ASSERT_EQUAL(ASSIGNMENT, stmt->type);
    TEST_ASSERT_EQUAL_STRING("sum", stmt->assignStmt->target->id);
    TEST_ASSERT_EQUAL(STORE, stmt->assignStmt->target->ctx);
    TEST_ASSERT_EQUAL(BINARY_OPERATION, stmt->assignStmt->value->type);
    TEST_ASSERT_EQUAL_STRING("+", stmt->assignStmt->value->binOp->operator);
    TEST_ASSERT_EQUAL_STRING("sum", stmt->assignStmt->value->binOp->left->variable->id);
    TEST_ASSERT_EQUAL(LOAD, stmt->assignStmt->value->binOp->left->variable->ctx);
    TEST_ASSERT_EQUAL(LITERAL, stmt->assignStmt->value->binOp->right->type);
    TEST_ASSERT_EQUAL_STRING("1", stmt->assignStmt->value->binOp->right->literal->value);
}

void test_for_statement(void) {
    source = LoadFileText(SAMPLES[4]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer, Symbol_CreateHashTable(10));
    Node_LinkedList program = ParseStatements(&parser);
    Node_Pop(&program);
    Node const *node = Node_Pop(&program);
    // Perform assertions on the for statement
    TEST_ASSERT_EQUAL(FOR, node->type);
    TEST_ASSERT_NOT_EQUAL(NULL, node->forStmt->target.variable);
    TEST_ASSERT_EQUAL(LIST_EXPR, node->forStmt->iter->type);
    TEST_ASSERT_EQUAL(4, node->forStmt->iter->list->elts.size);
    // Check the body statement
    Node const *stmt = Node_Pop(&node->forStmt->body);
    TEST_ASSERT_EQUAL(ASSIGNMENT, stmt->type);
    TEST_ASSERT_EQUAL_STRING("total", stmt->assignStmt->target->id);
    TEST_ASSERT_EQUAL(STORE, stmt->assignStmt->target->ctx);
    TEST_ASSERT_EQUAL(BINARY_OPERATION, stmt->assignStmt->value->type);
    TEST_ASSERT_EQUAL_STRING("+", stmt->assignStmt->value->binOp->operator);
    TEST_ASSERT_EQUAL_STRING("total", stmt->assignStmt->value->binOp->left->variable->id);
    TEST_ASSERT_EQUAL(LOAD, stmt->assignStmt->value->binOp->left->variable->ctx);
    TEST_ASSERT_EQUAL(VARIABLE, stmt->assignStmt->value->binOp->right->type);
    TEST_ASSERT_EQUAL_STRING("n", stmt->assignStmt->value->binOp->right->variable->id);
    TEST_ASSERT_EQUAL(LOAD, stmt->assignStmt->value->binOp->right->variable->ctx);
}

void test_assign_statement(void) {
    source = LoadFileText(SAMPLES[5]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer, Symbol_CreateHashTable(10));
    Node_LinkedList program = ParseStatements(&parser);
    // Check the assignment statement
    Node const *stmt1 = Node_Pop(&program);
    TEST_ASSERT_EQUAL(ASSIGNMENT, stmt1->type);
    TEST_ASSERT_EQUAL_STRING("a", stmt1->assignStmt->target->id);
    TEST_ASSERT_EQUAL(STORE, stmt1->assignStmt->target->ctx);
    TEST_ASSERT_EQUAL(LITERAL, stmt1->assignStmt->value->type);
    TEST_ASSERT_EQUAL_STRING("1", stmt1->assignStmt->value->literal->value);
    // Semantics assertion
    TEST_ASSERT_EQUAL(INT, stmt1->assignStmt->value->literal->type);
    Node const *stmt2 = Node_Pop(&program);
    TEST_ASSERT_EQUAL(ASSIGNMENT, stmt2->type);
    TEST_ASSERT_EQUAL_STRING("c", stmt2->assignStmt->target->id);
    TEST_ASSERT_EQUAL(STORE, stmt2->assignStmt->target->ctx);
    TEST_ASSERT_EQUAL(BINARY_OPERATION, stmt2->assignStmt->value->type);
    TEST_ASSERT_EQUAL_STRING("+", stmt2->assignStmt->value->binOp->operator);
    TEST_ASSERT_EQUAL_STRING("a", stmt2->assignStmt->value->binOp->left->variable->id);
    TEST_ASSERT_EQUAL(LOAD, stmt2->assignStmt->value->binOp->left->variable->ctx);
    TEST_ASSERT_EQUAL(LITERAL, stmt2->assignStmt->value->binOp->right->type);
    TEST_ASSERT_EQUAL_STRING("1", stmt2->assignStmt->value->binOp->right->literal->value);
}

void test_bin_op_type_precedence() {
    source = LoadFileText(SAMPLES[6]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer, Symbol_CreateHashTable(10));
    Node_LinkedList program = ParseStatements(&parser);
    Node_Pop(&program);
    Node const *binOp = Node_Pop(&program)->assignStmt->value;
    TEST_ASSERT_EQUAL(FLOAT, binOp->binOp->type);
    TEST_ASSERT_EQUAL(FLOAT, binOp->binOp->left->variable->type);
    TEST_ASSERT_EQUAL(INT, binOp->binOp->right->literal->type);
}

void test_assignment_serialization(void) {
    // Arrange
    source = LoadFileText(SAMPLES[5]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer, Symbol_CreateHashTable(10));
    Node_LinkedList program = ParseStatements(&parser);
    Node *node = Node_Pop(&program);
    // Act
    const cJSON *root = SerializeNode(node);
    TEST_ASSERT_TRUE(SaveFileText(OUTPUT_PATH"/test_assignment_serialization.json", cJSON_Print(root)));
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
    RUN_TEST(test_bin_op_type_precedence);
    RUN_TEST(test_create_hashtable);
    RUN_TEST(test_insert_pair);
    RUN_TEST(test_insert_retrieve);
    RUN_TEST(test_assignment_serialization);
    return UNITY_END();
}
