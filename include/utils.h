#pragma once
#ifndef UTILS_H
#define UTILS_H
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

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

ArrayList CreateArrayList(size_t initialCapacity);

void ArrayListPush(ArrayList* list, void* value);

void ArrayListPrint(ArrayList* list, Action printer);

void ArrayListClear(ArrayList* list, Action freeInnerResourcesCallback);
#endif // !UTILS_H



