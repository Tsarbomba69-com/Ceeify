#ifndef TEST_CODEGEN_H_
#define TEST_CODEGEN_H_

#include "codegen.h"
#include <unity.h>

Codegen compile_to_c(const char *source) {
  Lexer lexer = tokenize(source, "test.py");
  Parser parser = parse(&lexer);

  SemanticAnalyzer sa = analyze_program(&parser);
  TEST_ASSERT_FALSE(sa_has_error(&sa));

  Codegen cg = codegen_init(&sa);
  TEST_ASSERT_TRUE(codegen_program(&cg));
  TEST_ASSERT_FALSE(codegen_has_error(&cg));
  return cg;
}

void test_codegen_function_return_literal(void) {
  // Arrange
  const char *expected = "int f(void) {\n"
                         "    return 42;\n"
                         "}\n";
  // Act
  Codegen cg = compile_to_c("def f() -> int:\n"
                            "    return 42\n");
  // Assert
  TEST_ASSERT_EQUAL_STRING(expected, cg.output.items);
  // Cleanup
  codegen_free(&cg);
}

void test_codegen_function_call(void) {
  // Arrange
  const char *expected = "int f(int a) {\n"
                         "    return a;\n"
                         "}\n"
                         "int g(void) {\n"
                         "    return f(3);\n"
                         "}\n";
  // Act
  Codegen cg = compile_to_c("def f(a: int) -> int:\n"
                            "    return a\n"
                            "\n"
                            "def g() -> int:\n"
                            "    return f(3)\n");

  // Assert
  TEST_ASSERT_EQUAL_STRING(expected, cg.output.items);
  // Cleanup
  codegen_free(&cg);
}

#endif // TEST_CODEGEN_H_