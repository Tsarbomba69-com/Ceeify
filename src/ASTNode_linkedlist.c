#include "ASTNode_linkedlist.h"

ASTNode_LinkedList ASTNode_new(size_t capacity) {
  assert(capacity > 0 && capacity <= SIZE_MAX);

  ASTNode_LinkedList list = {0};
  list.capacity = capacity;
  list.head = SIZE_MAX;
  list.tail = SIZE_MAX;
  list.free = 0;
  list.size = 0;
  list.elements = allocator_alloc(&list.allocator, capacity * sizeof(ASTNode_Node));

  for (size_t i = 0; i < capacity - 1; i++) {
    list.elements[i].next = i + 1;
  }

  list.elements[capacity - 1].next = SIZE_MAX;
  return list;
}

void expand(ASTNode_LinkedList *list) {
  size_t old_cap = list->capacity;
  size_t new_cap = list->capacity * 2;

  list->elements = allocator_realloc(&list->allocator, list->elements,
                                 list->size * sizeof(ASTNode_Node),
                                 new_cap * sizeof(ASTNode_Node));

  for (size_t i = old_cap; i < new_cap - 1; i++) {
    list->elements[i].next = i + 1;
  }

  list->elements[new_cap - 1].next = SIZE_MAX;

  if (list->free == SIZE_MAX) {
    list->free = old_cap;
  } else {
    size_t current = list->free;

    while (list->elements[current].next != SIZE_MAX) {
      current = list->elements[current].next;
    }

    list->elements[current].next = old_cap;
  }

  list->capacity = new_cap;
}

void ASTNode_add_first(ASTNode_LinkedList *list, ASTNode *data) {
  if (list->size >= list->capacity) {
    expand(list);
  }

  size_t new_head = list->free;
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

  size_t new_tail = list->free;
  list->free = list->elements[new_tail].next;
  list->elements[new_tail].data = data;
  list->elements[new_tail].next = SIZE_MAX;

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

  size_t old_tail = list->tail;
  ASTNode *data = list->elements[old_tail].data;

  if (list->size == 1) {
    list->head = list->tail = SIZE_MAX;
  } else {
    size_t new_tail = list->head;
    while (list->elements[new_tail].next != old_tail) {
      new_tail = list->elements[new_tail].next;
    }
    list->tail = new_tail;
    list->elements[new_tail].next = SIZE_MAX;
  }

  list->elements[old_tail].next = list->free;
  list->free = old_tail;
  list->size--;
  return data;
}

void ASTNode_free(ASTNode_LinkedList *list) {
  allocator_free(&list->allocator);
  list->elements = NULL;
  list->capacity = 0;
  list->size = 0;
  list->head = list->tail = list->free = SIZE_MAX;
}