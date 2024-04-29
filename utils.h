#pragma once
#ifndef UTILS_H
#define UTILS_H
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))

// Load text data from file (read), returns a '\0' terminated string
char* LoadFileText(const char* fileName);		

void UnloadFileText(char* text);

// Take a string slice of the source string. WARNING: The caller is responsible for cleaning the memory
char* Slice(const char* source, size_t start, size_t end);
#endif // !UTILS_H



