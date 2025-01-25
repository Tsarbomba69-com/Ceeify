#include "utils.h"

void cleanup_file(void *p) {
  FILE **fp = (FILE **)p;
  if (*fp) {
    printf("Closing file: %p\n", (void *)*fp);
    fclose(*fp);
  }
}

char *load_file_text(Arena *arena, const char *filename) {
  char *text = NULL;

  if (filename == NULL) {
    fprintf(stderr, "FATAL: File name provided is not valid\n");
    return NULL;
  }

  DEFER(cleanup_file) FILE *file = fopen(filename, "rt");
  if (file == NULL) {
    fprintf(stderr, "FATAL: [%s] Failed to open text file\n", filename);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (size <= 0) {
    fprintf(stderr, "FATAL: [%s] Failed to read text file\n", filename);
    return NULL;
  }

  text = (char *)arena_alloc(arena, (size + 1) * sizeof(char));

  if (text == NULL) {
    fprintf(stderr, "FATAL: [%s] Failed to allocate memory for file reading\n",
            filename);
    return NULL;
  }

  size_t count = fread(text, sizeof(char), size, file);
  if (count < size)
    text = (char *)arena_realloc(arena, text, size, count + 1);

  text[count] = '\0';
  return text;
}

char *slice(Arena *arena, const char *source, size_t start, size_t end) {
  size_t length = end - start;
  if (length <= 0) {
    fprintf(stderr,
            "ERROR: the length of the slice must be greater than zero\n");
    return NULL;
  }

  char *result = arena_alloc(arena, (length + 1) * sizeof(char));
  if (result == NULL) {
    fprintf(stderr, "FATAL: Failed to allocate memory for string slice\n");
    return NULL;
  }

  memcpy(result, source + start, length);
  result[length] = '\0';
  return result;
}