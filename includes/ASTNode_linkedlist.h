#pragma once
#ifndef NODE_LIKNEDLIST_H_
#define NODE_LIKNEDLIST_H_

#include "stdlib.h"
#include "utils.h"

typedef struct ASTNode ASTNode;

typedef struct ASTNode_Node ASTNode_Node;

// node structure
typedef struct ASTNode_Node {
  ASTNode *data;
  ASTNode_Node *next;
} ASTNode_Node;

typedef struct ASTNode_LinkedList {
  ASTNode_Node *head;
  size_t size;
} ASTNode_LinkedList;

// Stack allocated linkedlist constructor
ASTNode_LinkedList ASTNode_linkedlist_new();

// Adds an element at the beginning of the list
void ASTNode_add_first(ASTNode_LinkedList *list, ASTNode *data);

// Adds an element at the end of the list
void ASTNode_add_last(ASTNode_LinkedList *list, ASTNode *data);

// Get the last element and remove it
ASTNode *Node_pop(ASTNode_LinkedList *list);

#endif // !NODE_LIKNEDLIST_H_
