#include "unity.h"
#include "parser.h"

// TODO: FIX - Each lexer should have it's current token index

const char *SAMPLES[] = {"./test/samples/portal.py", "./test/samples/list.py"};
char *source = {0};
Token_ArrayList tokens = {0};
Lexer lexer = {0};

void test_fullscanning(void) {
    TEST_ASSERT_EQUAL(lexer.sourceLength, lexer.position);
}

void test_statement_number(void) {
    Node_LinkedList program = ParseStatements(&tokens);
    TEST_ASSERT_EQUAL(8, program.size);
}

void test_list_size(void) {
    source = LoadFileText(SAMPLES[1]);
    if (source == NULL) return;
    lexer = CreateLexer(source);
    tokens = Tokenize(&lexer);
    Node_LinkedList program = ParseStatements(&tokens);
    List const *num_list = Node_Pop(&program)->assign_stmt->value->list;
    List const *str_list = Node_Pop(&program)->assign_stmt->value->list;
    TEST_ASSERT_EQUAL(4, num_list->elts.size);
    TEST_ASSERT_EQUAL(3, str_list->elts.size);
}

void setUp(void) {
    source = LoadFileText(SAMPLES[0]);
    if (source == NULL) return;
    lexer = CreateLexer(source);
    tokens = Tokenize(&lexer);
}

void tearDown(void) {
    // Clean up after each test
}

int main2(void) {
    UNITY_BEGIN();
    RUN_TEST(test_fullscanning);
    RUN_TEST(test_statement_number);
    RUN_TEST(test_list_size);
    return UNITY_END();
}
