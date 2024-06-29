#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#ifndef Symbol_HASH_TABLE_H
#define Symbol_HASH_TABLE_H

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))
#endif  // !ARRAYSIZE(a)

#define LOAD_FACTOR_THRESHOLD 0.75

typedef struct Symbol Symbol;

typedef bool (Symbol_CompareFn)(const Symbol *, const Symbol *);

typedef struct char_Symbol_Pair {
    char *key;
    Symbol *value;
    bool occupied;
} char_Symbol_Pair;

typedef struct Symbol_HashTable {
    size_t capacity;
    size_t size;
    char_Symbol_Pair **buckets;
} Symbol_HashTable;

//
Symbol_HashTable Symbol_CreateHashTable(size_t capacity);

// Hash function for the key-value pair
size_t Symbol_Hash(const char *key);

// Heap allocate the dynamic array within the table struct
void Symbol_AllocateElementes(Symbol_HashTable *table);

// Insert a key-value pair into the hash table
void Symbol_Insert(Symbol_HashTable *table, char const *key, Symbol *value);

//
Symbol *Symbol_Search(Symbol_HashTable *ht, const char *key);

//
void Symbol_Remove(Symbol_HashTable *table, char *key);

#endif // !Symbol_HASH_TABLE_H
