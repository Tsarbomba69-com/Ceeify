#ifndef TEST_SEMANTIC_H_
#define TEST_SEMANTIC_H_
#pragma once
#include "semantic.h"
#include <unity.h>

void test_semantic_empty_program(void) {
  // Arrange
  Lexer lexer = tokenize("", "test.py");
  Parser parser = parse(&lexer);
  // Act
  SemanticAnalyzer sa = analyze_program(&parser);
  SemanticError err = sa_get_error(&sa);
  // Assert
  TEST_ASSERT_EQUAL(SEM_OK, err.type);
  TEST_ASSERT_FALSE(sa_has_error(&sa));
  // Cleanup
  parser_free(&parser);
}

void test_semantic_simple_assignment(void) {
  // Arrange
  Lexer lexer = tokenize("x = 5", "test.py");
  Parser parser = parse(&lexer);
  // Act
  SemanticAnalyzer sa = analyze_program(&parser);
  SemanticError err = sa_get_error(&sa);
  // Assert
  TEST_ASSERT_EQUAL(SEM_OK, err.type);
  TEST_ASSERT_FALSE(sa_has_error(&sa));
  // Cleanup
  parser_free(&parser);
}

void test_semantic_undefined_variable(void) {
  // Arrange
  Lexer lexer = tokenize("z = x + 1", "test.py");
  Parser parser = parse(&lexer);
  // Act
  SemanticAnalyzer sa = analyze_program(&parser);
  SemanticError err = sa_get_error(&sa);
  // Assert
  TEST_ASSERT_TRUE(sa_has_error(&sa));
  TEST_ASSERT_EQUAL(SEM_UNDEFINED_VARIABLE, err.type);
  // Cleanup
  parser_free(&parser);
}

void test_semantic_type_mismatch(void) {
  // Arrange
  Lexer lexer = tokenize("x = 1\n"
                         "y = x + \"hello\"",
                         "test.py");
  Parser parser = parse(&lexer);
  // Act
  SemanticAnalyzer sa = analyze_program(&parser);
  SemanticError err = sa_get_error(&sa);
  // Assert
  TEST_ASSERT_TRUE(sa_has_error(&sa));
  TEST_ASSERT_EQUAL(SEM_TYPE_MISMATCH, err.type);
  // Cleanup
  parser_free(&parser);
}

void test_semantic_invalid_operation(void) {
  // Arrange
  Lexer lexer = tokenize("x = 5\n"
                         "y = x and 10",
                         "test.py");
  Parser parser = parse(&lexer);
  // Act
  SemanticAnalyzer sa = analyze_program(&parser);
  SemanticError err = sa_get_error(&sa);

  // Assert
  TEST_ASSERT_TRUE(sa_has_error(&sa));
  TEST_ASSERT_EQUAL(SEM_TYPE_MISMATCH, err.type);

  parser_free(&parser);
}

void test_semantic_function_declaration(void) {
  // Arrange
  Lexer lexer = tokenize("def add(x: float, y: float):\n"
                         "    return x + y\n",
                         "test_file.py");
  Parser parser = parse(&lexer);
  // Act
  SemanticAnalyzer sa = analyze_program(&parser);
  SemanticError err = sa_get_error(&sa);
  TEST_ASSERT_FALSE(sa_has_error(&sa));
  TEST_ASSERT_EQUAL(SEM_OK, err.type);
  Symbol *fn = sa_lookup(&sa, "add");
  TEST_ASSERT_NOT_NULL(fn);
  TEST_ASSERT_EQUAL(FUNCTION, fn->kind);
  TEST_ASSERT_NOT_NULL(fn->decl_node);
  sa.current_scope = fn->scope;
  Symbol *px = sa_lookup(&sa, "x");
  Symbol *py = sa_lookup(&sa, "y");
  TEST_ASSERT_NOT_NULL(px);
  TEST_ASSERT_NOT_NULL(py);
  TEST_ASSERT_EQUAL(VAR, px->kind);
  TEST_ASSERT_EQUAL(VAR, py->kind);
  // Cleanup
  parser_free(&parser);
}

void test_semantic_undefined_variable_in_function(void) {
  // Arrange
  Lexer lexer = tokenize("def f():\n"
                         "    return x\n",
                         "test.py");
  Parser parser = parse(&lexer);
  // Act
  SemanticAnalyzer sa = analyze_program(&parser);
  SemanticError err = sa_get_error(&sa);
  // Assert
  TEST_ASSERT_TRUE(sa_has_error(&sa));
  TEST_ASSERT_EQUAL(SEM_UNDEFINED_VARIABLE, err.type);
  // Cleanup
  parser_free(&parser);
}

void test_semantic_reassignment_type_mismatch(void) {
  // Arrange
  Lexer lexer = tokenize("x = 1\n"
                         "x = \"hello\"\n",
                         "test.py");
  Parser parser = parse(&lexer);
  // Act
  SemanticAnalyzer sa = analyze_program(&parser);
  SemanticError err = sa_get_error(&sa);
  // Assert
  TEST_ASSERT_TRUE(sa_has_error(&sa));
  TEST_ASSERT_EQUAL(SEM_TYPE_MISMATCH, err.type);
  // Cleanup
  parser_free(&parser);
}

void test_semantic_function_call_arity_mismatch(void) {
  // Arrange
  Lexer lexer = tokenize("def add(x: int, y: int):\n"
                         "    return x + y\n"
                         "add(1)\n",
                         "test.py");
  Parser parser = parse(&lexer);
  // Act
  SemanticAnalyzer sa = analyze_program(&parser);
  SemanticError err = sa_get_error(&sa);
  // Assert
  TEST_ASSERT_TRUE(sa_has_error(&sa));
  TEST_ASSERT_EQUAL(SEM_ARITY_MISMATCH, err.type);
  // Cleanup
  parser_free(&parser);
}

void test_semantic_function_call_type_mismatch(void) {
  // Arrange
  Lexer lexer = tokenize("def add(x: int, y: int):\n"
                         "    return x + y\n"
                         "add(1, \"hello\")\n",
                         "test.py");
  Parser parser = parse(&lexer);
  // Act
  SemanticAnalyzer sa = analyze_program(&parser);
  SemanticError err = sa_get_error(&sa);
  // Assert
  TEST_ASSERT_TRUE(sa_has_error(&sa));
  TEST_ASSERT_EQUAL(SEM_TYPE_MISMATCH, err.type);
  // Cleanup
  parser_free(&parser);
}

void test_semantic_function_return_type_mismatch(void) {
  // Arrange
  Lexer lexer = tokenize("def f() -> int:\n"
                         "    return \"hello\"\n",
                         "test.py");
  Parser parser = parse(&lexer);

  // Act
  SemanticAnalyzer sa = analyze_program(&parser);
  SemanticError err = sa_get_error(&sa);

  // Assert
  TEST_ASSERT_TRUE(sa_has_error(&sa));
  TEST_ASSERT_EQUAL(SEM_TYPE_MISMATCH, err.type);

  // Cleanup
  parser_free(&parser);
}

void test_semantic_function_return_type_ok(void) {
  // Arrange
  Lexer lexer = tokenize("def f() -> int:\n"
                         "    return 42\n",
                         "test.py");
  Parser parser = parse(&lexer);

  // Act
  SemanticAnalyzer sa = analyze_program(&parser);
  SemanticError err = sa_get_error(&sa);

  // Assert
  TEST_ASSERT_FALSE(sa_has_error(&sa));
  TEST_ASSERT_EQUAL(SEM_OK, err.type);

  // Cleanup
  parser_free(&parser);
}

void test_func_type_error_int_str(void) {
  // Arrange
  Lexer lexer = tokenize("def fun(age: int, name: str) -> None:\n"
                         "    result = age - name\n",
                         "test.py");
  const char *expected_msg =
      "TypeError: unsupported operand type(s) for -: 'int' and 'str'";
  // Act
  Parser parser = parse(&lexer);
  SemanticAnalyzer sa = analyze_program(&parser);
  SemanticError err = sa_get_error(&sa);
  // Assert
  TEST_ASSERT_TRUE(sa_has_error(&sa));
  TEST_ASSERT_EQUAL(SEM_TYPE_MISMATCH, err.type);
  TEST_ASSERT_NOT_NULL(strstr(err.message, expected_msg));
  // Cleanup
  parser_free(&parser);
}

void test_semantic_class_inheritance_and_init(void) {
  // Arrange
  Lexer lexer = tokenize("class Animal:\n"
                         "    name: str\n"
                         "\n"
                         "class Dog(Animal):\n"
                         "    tails: int\n"
                         "\n"
                         "    def __init__(self, name: str):\n"
                         "        self.name = name\n"
                         "        self.butt = 1\n",
                         "test.py");

  Parser parser = parse(&lexer);
  // Act
  SemanticAnalyzer sa = analyze_program(&parser);
  // Assert: no semantic errors
  TEST_ASSERT_FALSE(sa_has_error(&sa));

  // Assert: Animal class exists
  Symbol *animal = sa_lookup(&sa, "Animal");
  TEST_ASSERT_NOT_NULL(animal);
  TEST_ASSERT_EQUAL(CLASS, animal->kind);

  // Assert: Dog class exists
  Symbol *dog = sa_lookup(&sa, "Dog");
  TEST_ASSERT_NOT_NULL(dog);
  TEST_ASSERT_EQUAL(CLASS, dog->kind);

  // Assert: Animal has field 'name'
  Symbol *animal_name = NULL;
  if (animal->scope) {
    animal_name = sa_lookup(
        (SemanticAnalyzer *)&(SemanticAnalyzer){.current_scope = animal->scope},
        "name");
  }
  TEST_ASSERT_NOT_NULL(animal_name);
  TEST_ASSERT_EQUAL(VAR, animal_name->kind);
  TEST_ASSERT_EQUAL(STR, animal_name->dtype);

  // Assert: Dog has field 'tails'
  Symbol *dog_tails = NULL;
  if (dog->scope) {
    dog_tails = sa_lookup(
        (SemanticAnalyzer *)&(SemanticAnalyzer){.current_scope = dog->scope},
        "tails");
  }
  TEST_ASSERT_NOT_NULL(dog_tails);
  TEST_ASSERT_EQUAL(VAR, dog_tails->kind);
  TEST_ASSERT_EQUAL(INT, dog_tails->dtype);

  // Assert: inherited field is visible in Dog
  Symbol *dog_name = NULL;
  if (dog->scope) {
    dog_name = sa_lookup_member(dog, "name");
  }
  TEST_ASSERT_NOT_NULL(dog_name);
  TEST_ASSERT_EQUAL(STR, dog_name->dtype);

  Symbol *init_method = sa_lookup_member(dog, "__init__");
  TEST_ASSERT_NOT_NULL(init_method);
  // Cleanup
  parser_free(&parser);
}

void test_variable_redeclaration_error(void) {
  // Arrange
  Lexer lexer = tokenize("x: int = 10\n"
                         "def f():\n"
                         "    x: int = 5\n"
                         "    x: int = 2\n",
                         "test.py");
  const char *expected_msg = "variable 'x' already declared in this scope";
  // Act
  Parser parser = parse(&lexer);
  SemanticAnalyzer sa = analyze_program(&parser);
  SemanticError err = sa_get_error(&sa);
  // Assert
  TEST_ASSERT_TRUE(sa_has_error(&sa));
  TEST_ASSERT_EQUAL(SEM_REDECLARATION, err.type);
  TEST_ASSERT_NOT_NULL(strstr(err.message, expected_msg));
  // Cleanup
  parser_free(&parser);
}

void test_semantic_match_unreachable_after_wildcard(void) {
  // Arrange: A case after a wildcard '_' is unreachable
  Lexer lexer = tokenize("x = 1\n"
                         "match x:\n"
                         "    case _:\n"
                         "        y = 1\n"
                         "    case 10:\n" // Should fail here
                         "        y = 2\n",
                         "test.py");
  Parser parser = parse(&lexer);

  // Act
  SemanticAnalyzer sa = analyze_program(&parser);
  SemanticError err = sa_get_error(&sa);

  // Assert
  TEST_ASSERT_TRUE(sa_has_error(&sa));
  TEST_ASSERT_EQUAL(SEM_UNREACHABLE_PATTERN, err.type);
  TEST_ASSERT_NOT_NULL(strstr(err.message, "unreachable"));
  // Cleanup
  parser_free(&parser);
}

#endif // TEST_SEMANTIC_H_
