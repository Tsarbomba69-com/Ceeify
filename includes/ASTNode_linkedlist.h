#pragma once
#ifndef NODE_LIKNEDLIST_H_
#define NODE_LIKNEDLIST_H_

#include "utils.h"
#include <stdint.h>

#define DEFAULT_CAP 10

typedef struct ASTNode ASTNode;

typedef struct ASTNode_Node ASTNode_Node;

// node structure
typedef struct ASTNode_Node {
  ASTNode *data;
  uint8_t next;
} __attribute__((aligned(ALIGNED_64))) ASTNode_Node;

typedef struct ASTNode_LinkedList {
  Arena allocator;
  ASTNode_Node *elements; // Dynamic array of nodes
  size_t capacity;        // Current capacity of the array
  size_t size;            // Current number of elements in the list
  uint8_t head;           // Index of the first element in the list
  uint8_t tail;           // Index of the last element in the list
  uint8_t free;           // Index of the first free node in the array
} __attribute__((packed)) __attribute__((aligned(ALIGNED_64))) ASTNode_LinkedList;

// Stack allocated linkedlist constructor
ASTNode_LinkedList ASTNode_linkedlist_new(size_t capacity);

// Adds an element at the beginning of the list
void ASTNode_add_first(ASTNode_LinkedList *list, ASTNode *data);

// Adds an element at the end of the list
void ASTNode_add_last(ASTNode_LinkedList *list, ASTNode *data);

// Get the last element and remove it
ASTNode *ASTNode_pop(ASTNode_LinkedList *list);

// Free linked list resources
void ASTNode_linkedlist_free(ASTNode_LinkedList *list);

#endif // !NODE_LIKNEDLIST_H_
