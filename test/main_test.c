#include "unity.h"
#include "parser.h"

const char *SAMPLES[] = {"./test/samples/portal.py", "./test/samples/list.py", "./test/samples/if_statement.py"};
char *source = {0};
Lexer lexer = {0};

void test_fullscanning(void) {
    TEST_ASSERT_EQUAL(lexer.sourceLength, lexer.position);
}

void test_statement_number(void) {
    Node_LinkedList program = ParseStatements(&lexer);
    TEST_ASSERT_EQUAL(9, program.size);
}

void test_elif_statement_format(void) {
    source = LoadFileText(SAMPLES[2]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Node_LinkedList program = ParseStatements(&lexer);
    IfStmt const *if_stmt = Node_Pop(&program)->if_stmt;
    TEST_ASSERT_GREATER_OR_EQUAL(1, if_stmt->orelse.size);
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

void setUp(void) {
    source = LoadFileText(SAMPLES[0]);
    if (source == NULL) return;
    lexer = Tokenize(source);
}

void tearDown(void) {
    // Clean up after each test
}

int main2(void) {
    UNITY_BEGIN();
    RUN_TEST(test_fullscanning);
    RUN_TEST(test_statement_number);
    RUN_TEST(test_list_size);
    RUN_TEST(test_elif_statement_format);
    return UNITY_END();
}
