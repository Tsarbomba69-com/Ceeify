#include "ASTNode_linkedlist.h"

ASTNode_LinkedList ASTNode_linkedlist_new() {
  ASTNode_LinkedList list = {0};
  list.size = 0;
  list.head = NULL;
  return list;
}

ASTNode_Node *ASTNode_node_new(ASTNode_LinkedList *list, ASTNode *data) {
  ASTNode_Node *newNode = arena_alloc(&list->allocator, sizeof(ASTNode_Node));
  if (newNode == NULL) {
    trace_log(LOG_ERROR, "Failed to allocate memory for \"Node*\" node");
    return NULL;
  }
  newNode->data = data;
  newNode->next = NULL;
  return newNode;
}

void ASTNode_add_last(ASTNode_LinkedList *list, ASTNode *data) {
  ASTNode_Node *newNode = ASTNode_node_new(list, data);
  list->size++;

  if (list->head == NULL) {
    list->head = newNode;
    return;
  }

  ASTNode_Node *temp = list->head;
  while (temp->next != NULL) {
    temp = temp->next;
  }

  temp->next = newNode;
}

void ASTNode_add_first(ASTNode_LinkedList *list, ASTNode *data) {
  ASTNode_Node *newNode = ASTNode_node_new(list, data);
  newNode->next = list->head;
  list->head = newNode;
  list->size++;
}

ASTNode *Node_pop(ASTNode_LinkedList *list) {
  if (list->head == NULL) {
    return NULL;
  }

  ASTNode *value = list->head->data;
  list->head = list->head->next;
  list->size--;
  return value;
}
