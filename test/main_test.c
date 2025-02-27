#include "unity.h"
#include "parser.h"
#include "hashtable_test.h"
#include "stringbuilder_test.h"
#include "code_generator.h"
#include "ir.h"

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
    Parser parser = CreateParser(lexer);
    Node_LinkedList program = parse_statements(&parser);
    TEST_ASSERT_EQUAL(11, program.size);
}

void test_elif_statement_format(void) {
    source = LoadFileText(SAMPLES[2]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer);
    Node_LinkedList program = parse_statements(&parser);
    Node_Pop(&program);
    Node_Pop(&program);
    IfStmt *ifStmt = Node_Pop(&program)->ifStmt;
    TEST_ASSERT_GREATER_OR_EQUAL(1, ifStmt->orelse.size);
    Assign const *assign_stmt = Node_Pop(&ifStmt->body)->assignStmt;
    TEST_ASSERT_EQUAL_STRING("pow", assign_stmt->target->id);
}

void test_relational_operation(void) {
    // Arrange
    source = LoadFileText(SAMPLES[2]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    // Act
    Parser parser = CreateParser(lexer);
    Node_LinkedList program = parse_statements(&parser);
    Node_Pop(&program);
    Node_Pop(&program);
    const IfStmt *ifStmt = Node_Pop(&program)->ifStmt;
    // Assert
    TEST_ASSERT_EQUAL(LITERAL, ifStmt->test->compare->left->type);
}

void test_list_size(void) {
    source = LoadFileText(SAMPLES[1]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer);
    Node_LinkedList program = parse_statements(&parser);
    List const *num_list = Node_Pop(&program)->assignStmt->value->list;
    List const *str_list = Node_Pop(&program)->assignStmt->value->list;
    TEST_ASSERT_EQUAL(4, num_list->elts.size);
    TEST_ASSERT_EQUAL(3, str_list->elts.size);
}

void test_while_statement(void) {
    source = LoadFileText(SAMPLES[3]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer);
    Node_LinkedList program = parse_statements(&parser);
    const cJSON *root = SerializeProgram(&program);
    const cJSON *symTable = SerializeSymbol(parser.context);
    Node_Pop(&program);
    Node const *node = Node_Pop(&program);
    // Perform assertions on the while statement
    TEST_ASSERT_EQUAL(WHILE, node->type);
    TEST_ASSERT_NOT_EQUAL(NULL, node->whileStmt->test);
    TEST_ASSERT_GREATER_OR_EQUAL(1, node->whileStmt->body.size);
    // Check the test expression
    TEST_ASSERT_EQUAL(COMPARE, node->whileStmt->test->type);
    const Token *op = Token_Get(&node->whileStmt->test->compare->ops, 0);
    TEST_ASSERT_EQUAL_STRING("<=", op->lexeme);
    const Node *left = node->whileStmt->test->compare->left;
    TEST_ASSERT_EQUAL(VARIABLE, left->type);
    const Node *right = Node_Pop(&node->whileStmt->test->compare->comparators);
    TEST_ASSERT_EQUAL(LITERAL, right->type);
    TEST_ASSERT_EQUAL(LOAD, left->variable->ctx);
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
    // Assert serialization
    TEST_ASSERT_TRUE(SaveFileText(OUTPUT_PATH"/while_statement.json", cJSON_Print(root)));
    TEST_ASSERT_TRUE(SaveFileText(OUTPUT_PATH"/while_statement_symbol_table.json", cJSON_Print(symTable)));
}

void test_assign_codegen(void) {
    // Arrange
    source = LoadFileText(SAMPLES[5]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer);
    parser.ast = parse_statements(&parser);
    const char *code = Transpile(&parser);
    TEST_ASSERT_NOT_EMPTY(code);
    TEST_ASSERT_TRUE(SaveFileText(OUTPUT_PATH"/test_assign_codegen.c", (char *) code));
}


void test_if_stmt_codegen(void) {
    // Arrange
    source = LoadFileText(SAMPLES[2]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer);
    parser.ast = parse_statements(&parser);
    const char *code = Transpile(&parser);
    TEST_ASSERT_NOT_EMPTY(code);
    TEST_ASSERT_TRUE(SaveFileText(OUTPUT_PATH"/test_if_stmt_codegen.c", (char *) code));
}

void binary_operation_type_codegen(void) {
    // Arrange
    source = LoadFileText(SAMPLES[6]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer);
    parser.ast = parse_statements(&parser);
    const char *code = Transpile(&parser);
    TEST_ASSERT_NOT_EMPTY(code);
    TEST_ASSERT_TRUE(SaveFileText(OUTPUT_PATH"/binary_operation_type_codegen.c", (char *) code));
}

void test_for_statement(void) {
    source = LoadFileText(SAMPLES[4]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer);
    Node_LinkedList program = parse_statements(&parser);
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
    Parser parser = CreateParser(lexer);
    Node_LinkedList program = parse_statements(&parser);
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
    Parser parser = CreateParser(lexer);
    Node_LinkedList program = parse_statements(&parser);
    Node_Pop(&program);
    Node_Pop(&program);
    Node const *binOp = Node_Pop(&program)->assignStmt->value;
    TEST_ASSERT_EQUAL(FLOAT, binOp->binOp->type);
    TEST_ASSERT_EQUAL(FLOAT, binOp->binOp->left->variable->type);
    TEST_ASSERT_EQUAL(INT, binOp->binOp->right->literal->type);
}

void test_bin_op_type_precedence_IR(void) {
    source = LoadFileText(SAMPLES[6]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer);
    parser.ast = parse_statements(&parser);
    IRNode_ArrayList ir = ParseIR(&parser);
    IRNode_Pop(&ir);
    IRNode_Pop(&ir);
    IRNode const *binOp = IRNode_Pop(&ir)->assign.value;
    TEST_ASSERT_EQUAL(FLOAT, binOp->dataType);
    TEST_ASSERT_EQUAL(FLOAT, binOp->binary.left->dataType);
    TEST_ASSERT_EQUAL(INT, binOp->binary.right->dataType);
}

void test_assignment_serialization(void) {
    // Arrange
    source = LoadFileText(SAMPLES[5]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer);
    Node_LinkedList program = parse_statements(&parser);
    // Act
    const cJSON *root = SerializeProgram(&program);
    // Assert
    TEST_ASSERT_TRUE(SaveFileText(OUTPUT_PATH"/test_assignment_serialization.json", cJSON_Print(root)));
}

void test_bin_op_type_precedence_IRGen(void) {
    source = LoadFileText(SAMPLES[6]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer);
    parser.ast = parse_statements(&parser);
    IRNode_ArrayList ir = ParseIR(&parser);
    size_t cap = 1024;
    char *generatedCode = AllocateContext(cap * sizeof(char)); // Adjust size as needed
    size_t outLen = 0;
    for (size_t i = 0; i < ir.size; ++i) {
        char *nodeStr = GenerateFullIRText(IRNode_Get(&ir, i));
        size_t nodeStrLen = strlen(nodeStr) + 3;
        outLen += snprintf(generatedCode + outLen, cap - outLen, "%s\n", nodeStr);
        if (outLen + nodeStrLen >= cap) {
            cap *= 2;
            generatedCode = ReallocateContext(generatedCode, outLen, cap);
        }
    }
    TEST_ASSERT_TRUE(SaveFileText(OUTPUT_PATH"/test_bin_op_type_precedence_IRGen.ll", generatedCode));
}

void test_portal_serialization(void) {
    // Arrange
    source = LoadFileText(SAMPLES[0]);
    if (source == NULL) return;
    lexer = Tokenize(source);
    Parser parser = CreateParser(lexer);
    parser.ast = parse_statements(&parser);
    // Act
    const cJSON *root = SerializeProgram(&parser.ast);
    const cJSON *symTable = SerializeSymbol(parser.context);
    // Assert
    TEST_ASSERT_TRUE(SaveFileText(OUTPUT_PATH"/test_portal_serialization.json", cJSON_Print(root)));
    TEST_ASSERT_TRUE(SaveFileText(OUTPUT_PATH"/test_portal_symbol_table.json", cJSON_Print(symTable)));
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
    RUN_TEST(test_portal_serialization);
    RUN_TEST(test_relational_operation);
    RUN_TEST(test_assign_codegen);
    RUN_TEST(binary_operation_type_codegen);
    RUN_TEST(test_bin_op_type_precedence_IR);
    RUN_TEST(test_bin_op_type_precedence_IRGen);
    RUN_TEST(test_if_stmt_codegen);
    RUN_TEST(sb_append_test);
    return UNITY_END();
}
