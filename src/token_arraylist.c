#include "token_arraylist.h"

Token_ArrayList Token_arraylist_new(size_t capacity) {
  Token_ArrayList list = {
      .capacity = capacity, .allocator = (Arena){0}, .size = 0};
  list.capacity = capacity;
  list.elements = arena_alloc(&list.allocator, capacity * sizeof(Token *));

  if (list.elements == NULL) {
    trace_log(LOG_ERROR,
              "Could not allocate memory for \"Token*\" array list elements");
  }
  return list;
}

void Token_push(Token_ArrayList *list, Token *value) {
  if (list->size == list->capacity) {
    size_t cap = list->capacity * 2;
    Token **elements =
        arena_realloc(&list->allocator, list->elements,
                      list->size * sizeof(Token *), cap * sizeof(Token *));

    if (elements == NULL) {
      trace_log(LOG_ERROR, "Failed to resize \"Token*\" array list");
      return;
    }

    list->elements = elements;
    list->capacity = cap;
  }

  list->elements[list->size++] = value;
}

Token *Token_pop(Token_ArrayList *list) {
  if (list->size == 0) {
    return NULL; // ArrayList is empty, return NULL
  }

  Token *element = list->elements[list->size - 1]; // Get the last element
  list->size--;                                    // Decrement the size
  return element;                                  // Return the last element
}
