#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "ir.h"

#ifndef IRNode_ARRAYLIST_H
#define IRNode_ARRAYLIST_H

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))
#endif  // !ARRAYSIZE(a)

typedef struct IRNode IRNode;

typedef struct IRNode_ArrayList {
    IRNode** elements;
    size_t size;
    size_t capacity;
} IRNode_ArrayList;

typedef void (*IRNode_Action)(IRNode*);

typedef bool (*IRNode_CompareFn)(const IRNode*, const IRNode*);

// Find any element within the array list that satisfy the predicate
bool IRNode_Any(IRNode_ArrayList const*, IRNode* const, IRNode_CompareFn);
// Stack allocated array list constructor
IRNode_ArrayList IRNode_CreateArrayList(size_t capacity);
// Heap allocated array list constructor
IRNode_ArrayList* IRNode_AllocateArrayList(size_t capacity);
// Heap allocate the dynamic array within the array list struct
void IRNode_AllocateElementes(IRNode_ArrayList* list);
// Pushes an element to the end of the array list
void IRNode_Push(IRNode_ArrayList* list, IRNode* value);
// Get the last element and remove it
IRNode* IRNode_Pop(IRNode_ArrayList* list);
// Get the element stored at the index. Returns NULL if index is out-of-bounds
static inline IRNode* IRNode_Get(IRNode_ArrayList const* arrayList, size_t index)
{
    return index < arrayList->size ? arrayList->elements[index] : NULL;
}
// Iterate the array list and apply the callback for each element
void IRNode_ForEach(IRNode_ArrayList const *list, IRNode_Action callback);
#endif // !IRNode_ARRAYLIST_H
