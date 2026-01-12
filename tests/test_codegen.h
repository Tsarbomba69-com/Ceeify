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

void test_codegen_class_to_struct(void) {
  // Arrange
  const char *expected = "typedef struct {\n"
                         "    int x;\n"
                         "    int y;\n"
                         "} Point;\n";

  // Act
  Codegen cg = compile_to_c("class Point:\n"
                            "    x: int\n"
                            "    y: int\n");

  // Assert
  TEST_ASSERT_EQUAL_STRING(expected, cg.output.items);

  // Cleanup
  codegen_free(&cg);
}

void test_codegen_class_inheritance_and_init(void) {
  // Arrange
  const char *expected = "typedef struct {\n"
                         "  char* name;\n"
                         "} Animal;\n"
                         "\n"
                         "typedef struct {\n"
                         "  Animal* base;\n"
                         "  int tails;\n"
                         "  int butt;\n"
                         "} Dog;\n"
                         "\n"
                         "void Dog___init__(Dog* self, char* name) {\n"
                         "    self->base->name = name;\n"
                         "    self->butt = 1;\n"
                         "}\n";
  // Act
  Codegen cg = compile_to_c("class Animal:\n"
                            "    name: str\n"
                            "\n"
                            "class Dog(Animal):\n"
                            "    tails: int\n"
                            "\n"
                            "    def __init__(self, name: str):\n"
                            "        self.name = name\n"
                            "        self.butt = 1\n");
  // Assert
  TEST_ASSERT_EQUAL_STRING(expected, cg.output.items);

  // Cleanup
  codegen_free(&cg);
}

void test_codegen_if_else_statement(void) {
  // Arrange
  char expected[] = "int f(int x) {\n"
                         "  if (x) {\n"
                         "    return 1;\n"
                         "  } else {\n"
                         "    return 0;\n"
                         "  }\n"
                         "}\n";
  normalize_whitespace(expected);
  // Act
  Codegen cg = compile_to_c("def f(x: int) -> int:\n"
                            "  if x:\n"
                            "    return 1\n"
                            "  else:\n"
                            "    return 0\n");
  normalize_whitespace(cg.output.items);
  // Assert
  TEST_ASSERT_EQUAL_STRING(expected, cg.output.items);
  // Cleanup
  codegen_free(&cg);
}

#endif // TEST_CODEGEN_H_
