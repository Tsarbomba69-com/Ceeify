#pragma once
#ifndef UTILS_H
#define UTILS_H
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

char* LoadFileText(const char* fileName);		// Load text data from file (read), returns a '\0' terminated string

void UnloadFileText(char* text);
#endif // !UTILS_H



