#ifndef UTILS_H_
#define UTILS_H_

#pragma once

#define ALIGNED_8 8
#define ALIGNED_16 16
#define ALIGNED_64 64
#define ALIGNED_32 32
#define ALIGNED_128 128

#define DEFER(fn) __attribute__((cleanup(fn)))

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

#define MAX_TRACELOG_MSG_LENGTH 256 // Max length of one trace-log message

#include "arena.h"
#include <stdio.h>
#include <stdlib.h>

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

char *load_file_text(Arena *allocator, const char *filename);

char *slice(Arena *allocator, const char *source, size_t start, size_t end);

void trace_log(TraceLogLevel logType, const char *text, ...);

#endif