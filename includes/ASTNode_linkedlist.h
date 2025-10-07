#ifndef NODE_LIKNEDLIST_H_
#define NODE_LIKNEDLIST_H_

#pragma once

#include "utils.h"
#include <stdint.h>

#define DEFAULT_CAP 10

typedef struct ASTNode ASTNode;

typedef struct ASTNode_Node ASTNode_Node;

// node structure
typedef struct ASTNode_Node {
  ASTNode *data;
  size_t next;
} __attribute__((packed)) __attribute__((aligned(16))) ASTNode_Node;

typedef struct ASTNode_LinkedList {
  Allocator allocator;
  ASTNode_Node *elements; // Dynamic array of nodes
  size_t capacity;        // Current capacity of the array
  size_t size;            // Current number of elements in the list
  size_t head;            // Index of the first element in the list
  size_t tail;            // Index of the last element in the list
  size_t free;            // Index of the first free node in the array
} __attribute__((packed)) __attribute__((aligned(64))) ASTNode_LinkedList;

// Stack allocated linkedlist constructor
ASTNode_LinkedList ASTNode_new(size_t capacity);

ASTNode_LinkedList ASTNode_new_with_allocator(Allocator *allocator,
                                              size_t capacity);

// Adds an element at the beginning of the list
void ASTNode_add_first(ASTNode_LinkedList *list, ASTNode *data);

// Adds an element at the end of the list
void ASTNode_add_last(ASTNode_LinkedList *list, ASTNode *data);

// Get the last element and remove it
ASTNode *ASTNode_pop(ASTNode_LinkedList *list);

// Get the element stored at the index. Returns NULL if index is out-of-bounds
static inline ASTNode *ASTNode_get(ASTNode_LinkedList const *list,
                                   size_t index) {
  return index < list->size ? list->elements[index].data : NULL;
}

// Free linked list resources
void ASTNode_free(ASTNode_LinkedList *list);

#endif // NODE_LIKNEDLIST_H_
