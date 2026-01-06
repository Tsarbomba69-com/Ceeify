#ifndef STRING_BUILDER_H_
#define STRING_BUILDER_H_
#pragma once

#include "utils.h"

typedef struct {
  char *items;
  size_t count;
  size_t capacity;
  Allocator *allocator;
} StringBuilder;

StringBuilder sb_init(Allocator *allocator, size_t capacity);

int sb_appendf(StringBuilder *sb, const char *fmt, ...) PRINTF_FORMAT(2, 3);

#endif // STRING_BUILDER_H_