#include "stringbuilder.h"

bool sb_ensure_cap(StringBuilder* sb, size_t additional) {
    if (sb->length + additional + 1 > sb->capacity) {
        size_t new_capacity = sb->capacity * 2 + additional;
        char* new_buffer = ReallocateContext(sb->buffer, sb->capacity, new_capacity);
        if (new_buffer == NULL) {
            return false;
        }
        sb->buffer = new_buffer;
        sb->capacity = new_capacity;
    }
    return true;
}

StringBuilder sb_create(size_t cap) {
    char *buffer = AllocateContext(cap);
    if (buffer == NULL) {
        return (StringBuilder){0};
    }
    buffer[0] = '\0';
    return (StringBuilder) { .buffer = buffer, .length = 0, .capacity = cap };
}

void sb_append(StringBuilder *sb, const char *str) {
    size_t len = strlen(str);
    if (!sb_ensure_cap(sb, len)) {
        perror("ERROR: Could not reallocate memory for string buffer!");
        return;
    }

    strcpy(sb->buffer + sb->length, str);
    sb->length += len;
}
