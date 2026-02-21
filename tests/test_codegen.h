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
  char expected[] = "typedef struct {\n"
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
  normalize_whitespace(expected);
  normalize_whitespace(cg.output.items);
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

void test_codegen_while_loop(void) {
  // Arrange
  char expected[] = "void countdown(int n) {\n"
                    "    while (n > 0) {\n"
                    "        n = n - 1;\n"
                    "    }\n"
                    "}\n";
  // Act
  Codegen cg = compile_to_c("def countdown(n: int) -> None:\n"
                            "    while n > 0:\n"
                            "        n = n - 1\n");
  normalize_whitespace(expected);
  normalize_whitespace(cg.output.items);
  // Assert
  TEST_ASSERT_EQUAL_STRING(expected, cg.output.items);
  codegen_free(&cg);
}

void test_codegen_variable_shadowing(void) {
  // Arrange
  char expected[] =
      "int x = 10;\n"
      "void f(void) {\n"
      "    int x = 5;\n" // Should have 'int' because it's a new scope
      "    x = 2;\n"     // Should NOT have 'int'
      "}\n";
  // Act
  Codegen cg = compile_to_c("x: int = 10\n"
                            "def f():\n"
                            "    x: int = 5\n"
                            "    x = 2\n");
  normalize_whitespace(expected);
  normalize_whitespace(cg.output.items);
  // Assert
  TEST_ASSERT_EQUAL_STRING(expected, cg.output.items);
  codegen_free(&cg);
}

void test_codegen_match_literal(void) {
  // Arrange: Simple literal matching should lower to if/else if
  char expected[] = "void f(int x) {\n"
                    "    int _tmp0 = x;\n"
                    "    if (_tmp0 == 1) {\n"
                    "        print(\"one\");\n"
                    "    } else if (_tmp0 == 2) {\n"
                    "        print(\"two\");\n"
                    "    } else {\n"
                    "        print(\"other\");\n"
                    "    }\n"
                    "}\n";

  // Act
  Codegen cg = compile_to_c("def f(x: int):\n"
                            "    match x:\n"
                            "        case 1:\n"
                            "            print(\"one\")\n"
                            "        case 2:\n"
                            "            print(\"two\")\n"
                            "        case _:\n"
                            "            print(\"other\")\n");

  normalize_whitespace(expected);
  normalize_whitespace(cg.output.items);

  // Assert
  TEST_ASSERT_EQUAL_STRING(expected, cg.output.items);
  codegen_free(&cg);
}

void test_codegen_match_capture(void) {
  // Arrange: Matching with a variable capture
  char expected[] = "int f(int x) {\n"
                    "    int _tmp0 = x;\n"
                    "    if (_tmp0 == 0) {\n"
                    "        return 0;\n"
                    "    } else {\n"
                    "        int val = _tmp0;\n" // Capture occurs here
                    "        return val;\n"
                    "    }\n"
                    "}\n";

  // Act
  Codegen cg = compile_to_c("def f(x: int) -> int:\n"
                            "    match x:\n"
                            "        case 0:\n"
                            "            return 0\n"
                            "        case val:\n"
                            "            return val\n");

  normalize_whitespace(expected);
  normalize_whitespace(cg.output.items);

  // Assert
  TEST_ASSERT_EQUAL_STRING(expected, cg.output.items);
  codegen_free(&cg);
}

void test_codegen_match_guard(void) {
  // Arrange: Match with an 'if' guard
  char expected[] = "void f(int x) {\n"
                    "    int _tmp0 = x;\n"
                    "    if (_tmp0 > 0 && _tmp0 < 10) {\n"
                    "        print(\"small positive\");\n"
                    "    }\n"
                    "}\n";

  // Act
  Codegen cg = compile_to_c("def f(x: int):\n"
                            "    match x:\n"
                            "        case _ if x > 0 and x < 10:\n"
                            "            print(\"small positive\")\n");

  normalize_whitespace(expected);
  normalize_whitespace(cg.output.items);

  // Assert
  TEST_ASSERT_EQUAL_STRING(expected, cg.output.items);
  codegen_free(&cg);
}

#endif // TEST_CODEGEN_H_
