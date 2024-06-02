#include "Token_arraylist.h"
#include "arena.h"

static Arena token_arena = { 0 };

// WARNING: You can implement your own memory management API. Using libc API as default---------

Token** Token_AllocateContext(size_t size)
{
	assert(&token_arena);
	return arena_alloc(&token_arena, size);
}

void Token_FreeContext(Token** obj_ptr) {
	arena_free(&token_arena);
}

Token** Token_ReallocateContext(Token** oldptr, size_t oldptr_size, size_t size)
{
	assert(&token_arena);
	return arena_realloc(&token_arena, oldptr, oldptr_size, size);
}

// -----------------------------------------------------------------------------------------------

Token_ArrayList Token_CreateArrayList(size_t capacity)
{
	Token_ArrayList list = { 0 };
	list.capacity = capacity;
	Token_AllocateElementes(&list);
	return list;
}

Token_ArrayList* Token_AllocateArrayList(size_t capacity) {
	Token_ArrayList* list = Token_AllocateContext(sizeof(Token_ArrayList));
	if (list == NULL) {
		fprintf(stderr, "ERROR: Could not allocate memory for \"Token*\" array list");
		return NULL;
	}
	list->capacity = capacity;
	Token_AllocateElementes(list);
	return list;
}

void Token_AllocateElementes(Token_ArrayList* list)
{
	list->elements = Token_AllocateContext(list->capacity * sizeof(Token));
	if (list->elements == NULL) {
		fprintf(stderr, "ERROR: Could not allocate memory for \"Token*\" array list elements");
	}
	list->size = 0;
}

Token* Token_ArrayListPop(Token_ArrayList* list) {
	if (list->size == 0) {
		return NULL; // ArrayList is empty, return NULL
	}

	Token* element = list->elements[list->size - 1]; // Get the last element
	list->size--; // Decrement the size
	return element; // Return the last element
}

void Token_ArrayListPush(Token_ArrayList* list, Token* value)
{
	if (list->size == list->capacity) {
		size_t cap = list->capacity * 2;
		Token** elements = Token_ReallocateContext(list->elements, list->size * sizeof(Token*), cap * sizeof(Token*));
		if (elements == NULL) {
			fprintf(stderr, "ERROR: Failed to resize \"Token*\" array list\n");
			return;
		}
		list->elements = elements;
		list->capacity = cap;
	}
	list->elements[list->size++] = value;
}

bool Token_ArrayListAny(Token_ArrayList* list, Token* el, CompareFn predicate)
{
	for (size_t i = 0; i < list->size; i++) {
	    Token* curr = Token_ArrayListGet(list, i);
		if (predicate(curr, el)) {
			return true;
		}
	}
	return false;
}

void Token_ArrayListForEach(Token_ArrayList* list, Action callback)
{
	for (size_t i = 0; i < list->size; i++)
	{
		Token* element = list->elements[i];
		if (element != NULL) callback(element);
	}
}

