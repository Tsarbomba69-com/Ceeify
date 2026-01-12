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

StringBuilder sb_init(Allocator *allocator, size_t capacity) {
  StringBuilder sb;
  sb.items = allocator_alloc(allocator, capacity * sizeof(char));
  sb.count = 0;
  sb.capacity = capacity;
  sb.allocator = allocator;
  return sb;
}

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

void sb_append_padding(StringBuilder *sb, char pad_char, size_t count) {
  for (size_t i = 0; i < count; i++) {
    sb_appendf(sb, "%c", pad_char);
  }
}

int sb_replace(StringBuilder *sb, const char *old_value, const char *new_value) {
  if (sb == NULL || old_value == NULL || new_value == NULL) {
    return 0;
  }

  size_t old_len = strlen(old_value);
  size_t new_len = strlen(new_value);
  
  if (old_len == 0) {
    return 0;
  }

  // Count occurrences to calculate new capacity needed
  int count = 0;
  char *pos = sb->items;
  while ((pos = strstr(pos, old_value)) != NULL) {
    count++;
    pos += old_len;
  }

  if (count == 0) {
    return 0;
  }

  // Calculate new size and reserve space if needed
  size_t new_count = sb->count + count * (new_len - old_len);
  
  if (new_len > old_len) {
    sb_reserve(sb, new_count + 1);
  }

  // Perform replacements from end to start to avoid overwriting
  if (new_len != old_len) {
    // Need to shift content
    for (int i = count - 1; i >= 0; i--) {
      // Find the i-th occurrence
      pos = sb->items;
      for (int j = 0; j <= i; j++) {
        pos = strstr(pos, old_value);
        if (j < i) pos += old_len;
      }
      
      size_t pos_idx = pos - sb->items;
      size_t tail_len = sb->count - pos_idx - old_len;
      
      // Move tail
      memmove(pos + new_len, pos + old_len, tail_len);
      
      // Copy new value
      memcpy(pos, new_value, new_len);
      
      // Update count
      sb->count += (new_len - old_len);
    }
  } else {
    // Same length, simple replacement
    pos = sb->items;
    while ((pos = strstr(pos, old_value)) != NULL) {
      memcpy(pos, new_value, new_len);
      pos += new_len;
    }
  }

  return count;
}
