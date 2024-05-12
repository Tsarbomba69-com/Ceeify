#pragma once
#ifndef UTILS_H
#define UTILS_H
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))


typedef struct {
	void** elements;
	size_t size;
	size_t capacity;
} ArrayList;

typedef void (*Action)(void*);

// Load text data from file (read), returns a '\0' terminated string
char* LoadFileText(const char* fileName);

void UnloadFileText(char* text);

// Take a string slice of the source string. WARNING: The caller is responsible for cleaning the memory
char* Slice(const char* source, size_t start, size_t end);

char* Join(char* separator, char** items, size_t count);

const char* TextFormat(const char* text, ...);

ArrayList CreateArrayList(size_t capacity);

void ArrayListPush(ArrayList* list, void* value);

inline void Print(char* str) {
	printf("\033[0;33m\"%s\"\033[0m", str);
}

// Get the element stored at the index. Returns NULL if index is out-of-bounds
inline void* ArrayListGet(ArrayList* arrayList, size_t index)
{
	return index < arrayList->size ? arrayList->elements[index] : NULL;
}

void ArrayListPrint(ArrayList* list, Action printer);

void ArrayListClear(ArrayList* list, Action destroy);
#endif // !UTILS_H



