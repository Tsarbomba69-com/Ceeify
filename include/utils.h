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
    void **elements;
    size_t size;
    size_t capacity;
} ArrayList;

typedef void (*Action)(void *);

typedef bool (*CompareFn)(const void *, const void *);

void *AllocateContext(size_t size);

void *ReallocateContext(void *oldptr, size_t oldptr_size, size_t size);

void PrintArena();

void FreeContext();

// Load text data from file (read), returns a '\0' terminated string
char *LoadFileText(const char *fileName);

// Save text data to file (write), string must be '\0' terminated
bool SaveFileText(const char *fileName, char *text);

// Take a string slice of the source string. WARNING: The caller is responsible for cleaning the memory
char *Slice(const char *source, size_t start, size_t end);

char *Join(char *separator, char **items, size_t count);

char *Repeat(const char *str, size_t count);

const char *TextFormat(const char *text, ...);

static inline bool StrEQ(char const *str1, char const *str2) {
    return strcmp(str1, str2) == 0;
}
// Tests whether at least one element in the array passes the test implemented by the provided function
bool Any(void *arr[], size_t, void *, CompareFn);

void AllocateElementes(ArrayList *list);

static inline void Print(char *str) {
    printf("\033[0;33m\"%s\"\033[0m, ", str);
}

bool IsFloat(const char *str);

bool IsInteger(const char *str);

#endif // !UTILS_H



