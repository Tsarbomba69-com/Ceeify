#pragma once

#ifndef TOKEN_ARRAYLIST_H_
#define TOKEN_ARRAYLIST_H_

#include "arena.h"
#include "utils.h"
#include <stdio.h>

typedef struct Token Token;

typedef struct Token_ArrayList {
  Token **elements;
  size_t size;
  size_t capacity;
  Arena allocator;
} Token_ArrayList;

Token_ArrayList Token_arraylist_new(size_t capacity);

// Pushes an element to the end of the array list
void Token_push(Token_ArrayList *list, Token *value);

// Get the last element and remove it
Token *Token_pop(Token_ArrayList *list);

// Get the element stored at the index. Returns NULL if index is out-of-bounds
static inline Token *Token_get(Token_ArrayList const *arrayList, size_t index) {
  return index < arrayList->size ? arrayList->elements[index] : NULL;
}

#endif // !TOKEN_ARRAYLIST_H_