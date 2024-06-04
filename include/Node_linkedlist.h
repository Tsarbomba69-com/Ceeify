#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

typedef struct Node Node;

typedef struct Node_Node Node_Node;

typedef void (*Node_Action)(Node *);

typedef bool (*Node_CompareFn)(const Node *, const Node *);

#ifndef Node_LINKEDLIST_H
#define Node_LINKEDLIST_H

// node structure
typedef struct Node_Node {
    Node *data;
    Node_Node *next;
} Node_Node;

typedef struct Node_LinkedList {
    Node_Node *head;
    size_t size;
} Node_LinkedList;

// Stack allocated linkedlist constructor
Node_LinkedList Node_CreateLinkedList();

// Heap allocated linkedlist constructor
Node_LinkedList *Node_AllocateLinkedList();

// Heap allocated node constructor
Node_Node *Node_CreateNode(Node *data);

// Adds an element at the beginning of the list
void Node_AddFirst(Node_LinkedList *list, Node *data);

// Adds an element at the end of the list
void Node_AddLast(Node_LinkedList *list, Node *data);

// Traverse linked list
void Node_ForEach(Node_LinkedList *list, Node_Action callback);

// Get the last element and remove it
Node *Node_Pop(Node_LinkedList *list);

#endif // !Node_LINKEDLIST_H
