#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "token.h"

#ifndef Token_ARRAYLIST_H
#define Token_ARRAYLIST_H

#ifndef ARRAYSIZE(a)
#define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))
#endif  // !ARRAYSIZE(a)

typedef struct {
	Token** elements;
	size_t size;
	size_t capacity;
} Token_ArrayList;

typedef void (*Action)(Token*);

typedef bool (*CompareFn)(const Token*, const Token*);


Token_ArrayList Token_CreateArrayList(size_t capacity);
// Creates an array list on the heap
Token_ArrayList* Token_AllocateArrayList(size_t capacity);
// 
void Token_AllocateElementes(Token_ArrayList* list);
// 
bool Token_ArrayListAny(Token_ArrayList*, Token*, CompareFn);
// Pushes an element to the end of the array list
void Token_ArrayListPush(Token_ArrayList* list, Token* value);
// Get the last element and remove it
Token* Token_ArrayListPop(Token_ArrayList* list);
// Get the element stored at the index. Returns NULL if index is out-of-bounds
inline Token* Token_ArrayListGet(Token_ArrayList* arrayList, size_t index)
{
	return index < arrayList->size ? arrayList->elements[index] : NULL;
}
// Iterate the array list and apply the callback for each element
void Token_ArrayListForEach(Token_ArrayList* list, Action callback);
#endif // !Token_ARRAYLIST_H