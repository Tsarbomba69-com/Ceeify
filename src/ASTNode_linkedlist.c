#include "ASTNode_linkedlist.h"

ASTNode_LinkedList ASTNode_linkedlist_new(size_t capacity) {
  assert(capacity > 0 && capacity <= UINT8_MAX);

  ASTNode_LinkedList list = {0};
  list.capacity = capacity;
  list.head = UINT8_MAX;
  list.tail = UINT8_MAX;
  list.free = 0;
  list.size = 0;
  list.elements = (ASTNode_Node *)arena_alloc(&list.allocator,
                                              capacity * sizeof(ASTNode_Node));

  for (size_t i = 0; i < capacity - 1; i++) {
    list.elements[i].next = i + 1;
  }

  list.elements[capacity - 1].next = UINT8_MAX;
  return list;
}

void expand(ASTNode_LinkedList *list) {
  size_t new_cap = list->capacity * 2;
  list->capacity = new_cap;
  list->elements =
      arena_realloc(&list->allocator, list->elements, list->capacity, new_cap);
}

void ASTNode_add_first(ASTNode_LinkedList *list, ASTNode *data) {
  if (list->size >= list->capacity) {
    expand(list);
  }

  uint8_t new_head = list->free;
  list->free = list->elements[new_head].next;
  list->elements[new_head].data = data;
  list->elements[new_head].next = list->head;
  list->head = new_head;

  if (list->size == 0) {
    list->tail = new_head;
  }

  list->size++;
}

void ASTNode_add_last(ASTNode_LinkedList *list, ASTNode *data) {
  if (list->size >= list->capacity) {
    expand(list);
  }

  uint8_t new_tail = list->free;
  list->free = list->elements[new_tail].next;
  list->elements[new_tail].data = data;
  list->elements[new_tail].next = UINT8_MAX;

  if (list->size == 0) {
    list->head = new_tail;
  } else {
    list->elements[list->tail].next = new_tail;
  }

  list->tail = new_tail;
  list->size++;
}

ASTNode *ASTNode_pop(ASTNode_LinkedList *list) {
  if (list->size == 0) {
    return NULL;
  }

  uint8_t old_tail = list->tail;
  ASTNode *data = list->elements[old_tail].data;

  if (list->size == 1) {
    list->head = list->tail = UINT8_MAX;
  } else {
    uint8_t new_tail = list->head;
    while (list->elements[new_tail].next != old_tail) {
      new_tail = list->elements[new_tail].next;
    }
    list->tail = new_tail;
    list->elements[new_tail].next = UINT8_MAX;
  }

  list->elements[old_tail].next = list->free;
  list->free = old_tail;
  list->size--;
  return data;
}

void ASTNode_linkedlist_free(ASTNode_LinkedList *list) {
  arena_free(&list->allocator);
  list->elements = NULL;
  list->capacity = 0;
  list->size = 0;
  list->head = list->tail = list->free = UINT8_MAX;
}