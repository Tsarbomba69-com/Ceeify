#include "IRNode_arraylist.h"

// WARNING: You can implement your own memory management API. Using libc API as default---------

IRNode_ArrayList IRNode_CreateArrayList(size_t capacity) {
    IRNode_ArrayList list = {0};
    list.capacity = capacity;
    IRNode_AllocateElementes(&list);
    return list;
}

IRNode_ArrayList *IRNode_AllocateArrayList(size_t capacity) {
    IRNode_ArrayList *list = AllocateContext(sizeof(IRNode_ArrayList));
    if (list == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for \"IRNode*\" array list");
        return NULL;
    }
    list->capacity = capacity;
    IRNode_AllocateElementes(list);
    return list;
}

void IRNode_AllocateElementes(IRNode_ArrayList *list) {
    list->elements = AllocateContext(list->capacity * sizeof(IRNode *));
    if (list->elements == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for \"IRNode*\" array list elements");
    }
    list->size = 0;
}

// -----------------------------------------------------------------------------------------------

IRNode *IRNode_Pop(IRNode_ArrayList *list) {
    if (list->size == 0) {
        return NULL; // ArrayList is empty, return NULL
    }

    IRNode *element = list->elements[list->size - 1]; // Get the last element
    list->size--; // Decrement the size
    return element; // Return the last element
}

void IRNode_Push(IRNode_ArrayList *list, IRNode *value) {
    if (list->size == list->capacity) {
        size_t cap = list->capacity * 2;
        IRNode **elements = ReallocateContext(list->elements, list->size * sizeof(IRNode *),
                                                     cap * sizeof(IRNode *));
        if (elements == NULL) {
            fprintf(stderr, "ERROR: Failed to resize \"IRNode*\" array list\n");
            return;
        }
        list->elements = elements;
        list->capacity = cap;
    }
    list->elements[list->size++] = value;
}

bool IRNode_Any(IRNode_ArrayList const *list, IRNode *const el, IRNode_CompareFn predicate) {
    for (size_t i = 0; i < list->size; i++) {
        IRNode *curr = IRNode_Get(list, i);
        if (predicate(curr, el)) {
            return true;
        }
    }
    return false;
}

void IRNode_ForEach(IRNode_ArrayList const *list, IRNode_Action callback) {
    for (size_t i = 0; i < list->size; i++) {
        IRNode *element = list->elements[i];
        if (element != NULL) callback(element);
    }
}

