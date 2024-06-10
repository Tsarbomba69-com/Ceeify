#include "Token_arraylist.h"
#include "utils.h"

Token_ArrayList Token_CreateArrayList(size_t capacity) {
    Token_ArrayList list = {0};
    list.capacity = capacity;
    Token_AllocateElementes(&list);
    return list;
}

Token_ArrayList *Token_AllocateArrayList(size_t capacity) {
    Token_ArrayList *list = AllocateContext(sizeof(Token_ArrayList));
    if (list == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for \"Token*\" array list");
        return NULL;
    }
    list->capacity = capacity;
    Token_AllocateElementes(list);
    return list;
}

void Token_AllocateElementes(Token_ArrayList *list) {
    list->elements = AllocateContext(list->capacity * sizeof(Token *));
    if (list->elements == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for \"Token*\" array list elements");
    }
    list->size = 0;
}

Token *Token_Pop(Token_ArrayList *list) {
    if (list->size == 0) {
        return NULL; // ArrayList is empty, return NULL
    }

    Token *element = list->elements[list->size - 1]; // Get the last element
    list->size--; // Decrement the size
    return element; // Return the last element
}

void Token_Push(Token_ArrayList *list, Token *value) {
    if (list->size == list->capacity) {
        size_t cap = list->capacity * 2;
        Token **elements = ReallocateContext(list->elements, list->size * sizeof(Token *), cap * sizeof(Token *));
        if (elements == NULL) {
            fprintf(stderr, "ERROR: Failed to resize \"Token*\" array list\n");
            return;
        }
        list->elements = elements;
        list->capacity = cap;
    }
    list->elements[list->size++] = value;
}

bool Token_Any(Token_ArrayList const *list, Token const *el, Token_CompareFn predicate) {
    for (size_t i = 0; i < list->size; i++) {
        Token const *curr = Token_Get(list, i);
        if (predicate(curr, el)) {
            return true;
        }
    }
    return false;
}

void Token_ForEach(Token_ArrayList const *list, Token_Action callback) {
    for (size_t i = 0; i < list->size; i++) {
        Token *element = list->elements[i];
        if (element != NULL) callback(element);
    }
}

