#pragma once

#ifndef UTILS_H_
#define UTILS_H_
#define DEFER(fn) __attribute__((cleanup(fn)))

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

#include "arena.h"
#include <stdio.h>
#include <stdlib.h>
#endif

char *load_file_text(Arena* arena, const char *filename);

char *slice(Arena* arena, const char *source, size_t start, size_t end);