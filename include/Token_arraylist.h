#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "lexer.h"

#ifndef Token_ARRAYLIST_H
#define Token_ARRAYLIST_H

#ifndef ARRAYSIZE(a)
#define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))
#endif  // !ARRAYSIZE(a)

typedef struct Token Token;

typedef struct Token_ArrayList {
	Token** elements;
	size_t size;
	size_t capacity;
} Token_ArrayList;

typedef void (*Token_Action)(Token*);

typedef bool (*Token_CompareFn)(const Token*, const Token*);

// Memory operations ------------------------------------------

// Custom defined memory allocation function
Token** Token_AllocateContext(size_t size);
// Custom defined memory reallocation function
Token** Token_ReallocateContext(Token** oldptr, size_t oldptr_size, size_t size);

// ------------------------------------------------------------

// Find any element within the array list that satisfy the predicate
bool Token_Any(Token_ArrayList*, Token*, Token_CompareFn);
// Stack allocated array list constructor
Token_ArrayList Token_CreateArrayList(size_t capacity);
// Heap allocated array list constructor
Token_ArrayList* Token_AllocateArrayList(size_t capacity);
// Heap allocate the dynamic array within the array list struct
void Token_AllocateElementes(Token_ArrayList* list);
// Pushes an element to the end of the array list
void Token_Push(Token_ArrayList* list, Token* value);
// Get the last element and remove it
Token* Token_Pop(Token_ArrayList* list);
// Get the element stored at the index. Returns NULL if index is out-of-bounds
inline Token* Token_Get(Token_ArrayList* arrayList, size_t index)
{
	return index < arrayList->size ? arrayList->elements[index] : NULL;
}
// Iterate the array list and apply the callback for each element
void Token_ForEach(Token_ArrayList* list, Token_Action callback);
#endif // !Token_ARRAYLIST_H