#include "utils.h"

void cleanup_file(void *p) {
  FILE **fp = (FILE **)p;
  if (*fp) {
    slog_info("Closing file: %p", (void *)*fp);
    fclose(*fp);
  }
}

char *load_file_text(Allocator *allocator, const char *filename) {
  char *text = NULL;

  if (filename == NULL) {
    slog_error("File name provided is not valid");
    return NULL;
  }

  DEFER(cleanup_file) FILE *file = fopen(filename, "rt");
  if (file == NULL) {
    slog_error("[%s] Failed to open text file", filename);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (size <= 0) {
    slog_error("[%s] Failed to read text file", filename);
    return NULL;
  }

  text = (char *)allocator_alloc(allocator, (size + 1) * sizeof(char));

  if (text == NULL) {
    slog_error("[%s] Failed to allocate memory for file reading", filename);
    return NULL;
  }

  size_t count = fread(text, sizeof(char), size, file);
  if (count < size)
    text = (char *)allocator_realloc(allocator, text, size, count + 1);

  text[count] = '\0';
  return text;
}

char *slice(Allocator *allocator, const char *source, size_t start,
            size_t end) {
  size_t length = end - start;
  if (length <= 0) {
    slog_error("The length of the slice must be greater than zero");
    return NULL;
  }

  char *result = allocator_alloc(allocator, (length + 1) * sizeof(char));
  if (result == NULL) {
    slog_error("Failed to allocate memory for string slice");
    return NULL;
  }

  memcpy(result, source + start, length);
  result[length] = '\0';
  return result;
}

bool save_file_text(const char *filename, char *text) {
  if (filename == NULL) {
    slog_error("FILE: File name provided is not valid to save");
    return false;
  }

  FILE *file = fopen(filename, "wt");
  if (file == NULL) {
    slog_error("FILE: [%s] Failed to open text file", filename);
    return false;
  }

  int count = fprintf(file, "%s", text);
  if (count == 0) {
    slog_error("FILE: [%s] Failed to write text file", filename);
    fclose(file);
    return false;
  }

  slog_info("SUCCESS: [%s] Text file saved successfully\n", filename);
  fclose(file);
  return true;
}