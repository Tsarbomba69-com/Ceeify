#pragma once
#ifndef UTILS_H
#define UTILS_H
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#ifndef DEBUG
#define DEBUG false
#endif // !DEBUG

#define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))

typedef struct ArrayList {
	void** elements;
	size_t size;
	size_t capacity;
} ArrayList;

typedef void (*Action)(void*);

typedef bool (*CompareFn)(const void*, const void*);

void* AllocateContext(size_t size);

void* ReallocateContext(void* oldptr, size_t oldptr_size, size_t size);

void PrintArena();

void FreeContext();

// Load text data from file (read), returns a '\0' terminated string
char* LoadFileText(const char* fileName);

void UnloadFileText(char* text);

// Take a string slice of the source string. WARNING: The caller is responsible for cleaning the memory
char* Slice(const char* source, size_t start, size_t end);

char* Join(char* separator, char** items, size_t count);

char* Repeat(const char* str, size_t count);

const char* TextFormat(const char* text, ...);

inline bool StrEQ(char* str1, char* str2) {
	return strcmp(str1, str2) == 0;
}
// Tests whether at least one element in the array passes the test implemented by the provided function
bool Any(void* arr[], size_t, void*, CompareFn);

void AllocateElementes(ArrayList* list);

ArrayList CreateArrayList(size_t capacity);
// Creates an array list on the heap
ArrayList* AllocateArrayList(size_t capacity);
// Pushes an element to the end of the array list
void ArrayListPush(ArrayList* list, void* value);
// Get the last element and remove it
void* ArrayListPop(ArrayList* list);

inline void Print(char* str) {
	printf("\033[0;33m\"%s\"\033[0m, ", str);
}

// Get the element stored at the index. Returns NULL if index is out-of-bounds
inline void* ArrayListGet(ArrayList* arrayList, size_t index)
{
	return index < arrayList->size ? arrayList->elements[index] : NULL;
}
// Iterate the array list and apply the callback for each element
void ArrayListForEach(ArrayList* list, Action callback);

void ArrayListClear(ArrayList* list, Action destroy);
#endif // !UTILS_H



