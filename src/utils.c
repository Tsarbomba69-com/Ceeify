#include "utils.h"

static TraceLogLevel min_log_level = LOG_INFO; // Minimum log type level

void cleanup_file(void *p) {
  FILE **fp = (FILE **)p;
  if (*fp) {
    trace_log(LOG_INFO, "Closing file: %p", (void *)*fp);
    fclose(*fp);
  }
}

char *load_file_text(Arena *arena, const char *filename) {
  char *text = NULL;

  if (filename == NULL) {
    trace_log(LOG_FATAL, "File name provided is not valid");
    return NULL;
  }

  DEFER(cleanup_file) FILE *file = fopen(filename, "rt");
  if (file == NULL) {
    trace_log(LOG_FATAL, "[%s] Failed to open text file", filename);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (size <= 0) {
    trace_log(LOG_FATAL, "[%s] Failed to read text file", filename);
    return NULL;
  }

  text = (char *)arena_alloc(arena, (size + 1) * sizeof(char));

  if (text == NULL) {
    trace_log(LOG_FATAL, "[%s] Failed to allocate memory for file reading",
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

void trace_log(TraceLogLevel logType, const char *text, ...) {
  // Message has level below current threshold, don't emit
  if (logType < min_log_level) {
    return;
  }

  va_list args;
  va_start(args, text);
  char buffer[MAX_TRACELOG_MSG_LENGTH] = {0};

  switch (logType) {
  case LOG_TRACE:
    strcpy(buffer, "TRACE: ");
    break;
  case LOG_DEBUG:
    strcpy(buffer, "DEBUG: ");
    break;
  case LOG_INFO:
    strcpy(buffer, "INFO: ");
    break;
  case LOG_WARNING:
    strcpy(buffer, "WARNING: ");
    break;
  case LOG_ERROR:
    strcpy(buffer, "ERROR: ");
    break;
  case LOG_FATAL:
    strcpy(buffer, "FATAL: ");
    break;
  default:
    break;
  }

  uint8_t textSize = (uint8_t)strlen(text);
  memcpy(buffer + strlen(buffer), text,
         (textSize < (MAX_TRACELOG_MSG_LENGTH - 12))
             ? textSize
             : (MAX_TRACELOG_MSG_LENGTH - 12));
  strcat(buffer, "\n");
  vprintf(buffer, args);
  fflush(stdout);
  va_end(args);

  if (logType == LOG_FATAL)
    exit(EXIT_FAILURE); // If fatal logging, exit program
}