#include "string_builder.h"

#define sb_reserve(da, expected_capacity)                                      \
  do {                                                                         \
    if ((expected_capacity) > (da)->capacity) {                                \
      if ((da)->capacity == 0) {                                               \
        (da)->capacity = ARENA_DA_INIT_CAP;                                    \
      }                                                                        \
      while ((expected_capacity) > (da)->capacity) {                           \
        (da)->capacity *= 2;                                                   \
      }                                                                        \
      (da)->items =                                                            \
          (char *)allocator_realloc((da)->allocator, (da)->items, (da)->count, \
                                    (da)->capacity * sizeof(*(da)->items));    \
      ASSERT((da)->items != NULL, "Buy more RAM lol");                         \
    }                                                                          \
  } while (0)

int sb_appendf(StringBuilder *sb, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  sb_reserve(sb, sb->count + n + 1);
  char *dest = sb->items + sb->count;
  va_start(args, fmt);
  vsnprintf(dest, n + 1, fmt, args);
  va_end(args);
  sb->count += n;
  return n;
}