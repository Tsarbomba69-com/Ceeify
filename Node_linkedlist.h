#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "parser.h"

#ifndef Node_LINKEDLIST_H
#define Node_LINKEDLIST_H

typedef struct Node Node;

// node structure
typedef struct {
    Node* data;
    struct Node_Node* next;
} Node_Node;

typedef struct {
    Node_Node* head;
    size_t size;
} Node_LinkedList;

// Stack allocated linkedlist constructor
Node_LinkedList Node_CreateLinkedList();
// Heap allocated linkedlist constructor
Node_LinkedList* Node_AllocateLinkedList();
// Heap allocated node constructor
Node_Node* Node_CreateNode(Node* data);
// Adds an element at the beginning of the list
void Node_AddFirst(Node_LinkedList* list, Node* data);
// 
#endif // !Node_LINKEDLIST_H