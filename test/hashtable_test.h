#pragma once
#include "unity.h"
#include "Symbol_hashtable.h"

void test_create_hashtable(void) {
    Symbol_HashTable table = Symbol_CreateHashTable(10);
    TEST_ASSERT_EQUAL(10, table.capacity);
}

void test_insert_pair(void) {
    // Arrange
    Symbol_HashTable table = Symbol_CreateHashTable(1);
    Symbol* sym = AllocateContext(sizeof(Symbol));
    // Act
    Symbol_Insert(&table, "1", sym);
    Symbol_Insert(&table, "2", sym);
    Symbol_Insert(&table, "3", sym);
    // Assert
    TEST_ASSERT_GREATER_OR_EQUAL(3, table.size);
    TEST_ASSERT_GREATER_OR_EQUAL(0, table.capacity);
}

void test_insert_retrieve(void) {
    // Arrange
    Symbol_HashTable table = Symbol_CreateHashTable(1);
    Symbol* sym = AllocateContext(sizeof(Symbol));
    Symbol* sym2 = AllocateContext(sizeof(Symbol));
    Symbol* sym3 = AllocateContext(sizeof(Symbol));
    sym->type = FUNCTION;
    sym2->type = LIST;
    sym3->type = FLOAT;
    // Act
    Symbol_Insert(&table, "1", sym);
    Symbol_Insert(&table, "2", sym2);
    Symbol_Insert(&table, "3", sym3);
    // Assert
    TEST_ASSERT_EQUAL(FUNCTION, Symbol_Search(&table, "1")->type);
    TEST_ASSERT_EQUAL(LIST, Symbol_Search(&table, "2")->type);
    TEST_ASSERT_EQUAL(FLOAT, Symbol_Search(&table, "3")->type);
}
