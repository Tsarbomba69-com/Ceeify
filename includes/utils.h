#ifndef UTILS_H_
#define UTILS_H_

#pragma once

#include <slog.h>

#define DEFER(fn) __attribute__((cleanup(fn)))

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

#define ANY(arr, size, el, cmp_expr, result)                                   \
  {                                                                            \
    (result) = false;                                                          \
    for (size_t index = 0; index < (size); index++) {                          \
      if (cmp_expr) {                                                          \
        (result) = true;                                                       \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
  }

enum {
  MAX_TRACELOG_MSG_LENGTH = 256 // Max length of one trace-log message
};

#include "allocator.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  LOG_ALL = 0, // Display all logs
  LOG_TRACE,   // Trace logging, intended for internal use only
  LOG_DEBUG,   // Debug logging, used for internal debugging, it should be
  LOG_INFO,    // Info logging, used for program execution info
  LOG_WARNING, // Warning logging, used on recoverable failures
  LOG_ERROR,   // Error logging, used on unrecoverable failures
  LOG_FATAL,   // Fatal logging, used to abort program: exit(EXIT_FAILURE)
  LOG_NONE     // Disable logging
} TraceLogLevel;

char *load_file_text(Allocator *allocator, const char *filename);

bool save_file_text(const char *filename, char *text);

char *slice(Allocator *allocator, const char *source, size_t start, size_t end);

int safe_fprintf(FILE *f, const char *fmt, ...);

static inline int safe_memcpy(void *dest, size_t destsz, const void *src,
                              size_t count) {
  if (!dest || !src)
    return EINVAL;
  if (count > destsz)
    return ERANGE;

  memcpy(dest, src, count);
  return 0;
}

#define ASSERT(cond, msg)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      slog_error("\"%s\" %s failed at %s:%d", #cond, msg, __FILE__, __LINE__); \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#if defined(_MSC_VER)
/* MSVC: __assume is a hint to the optimizer, does not terminate execution */
#define UNREACHABLE(msg)                                                       \
  do {                                                                         \
    slog_fatal("UNREACHABLE: %s", (msg));                                      \
    __assume(0);                                                               \
    abort();                                                                   \
  } while (0)
#elif defined(__clang__) || defined(__GNUC__)
/* GCC / Clang have builtin_unreachable() and __builtin_unreachable() is a hint
 */
#define UNREACHABLE(msg)                                                       \
  do {                                                                         \
    slog_fatal("UNREACHABLE: %s", (msg));                                      \
                                                                               \
    __builtin_unreachable();                                                   \
    abort();                                                                   \
  } while (0)
#else
/* Fallback: no compiler hint available, just print and abort */
#define UNREACHABLE(msg)                                                       \
  do {                                                                         \
    slog_fatal("UNREACHABLE: %s", (msg));                                      \
    abort();                                                                   \
  } while (0)
#endif

#endif
