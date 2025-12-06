#ifndef PROFILER_H
#define PROFILER_H
#pragma once
#define _POSIX_C_SOURCE 199309L
#include <stdint.h>
#include <time.h>
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cJSON.h>
#include "allocator.h"

typedef struct {
  uint64_t max_rss_kb;      // Maximum resident set size in KB
  uint64_t read_bytes;      // Bytes read from disk
  uint64_t write_bytes;     // Bytes written to disk
  uint64_t cpu_us;          // CPU time in microseconds
  uint64_t wall_time_us;    // Wall clock time in microseconds
} ResourceMetrics;

typedef struct {
  char *name;
  char phase;               // 'B' for begin, 'E' for end
  uint64_t timestamp_us;    // Microseconds since epoch
  uint64_t duration_us;     // Duration in microseconds
  ResourceMetrics metrics;
} TraceEvent;

typedef struct {
  TraceEvent *events;
  size_t count;
  size_t capacity;
  Allocator* allocator;
} TraceBuffer;

// Initialize trace buffer
TraceBuffer trace_buffer_create(Allocator* allocator, size_t initial_capacity);

// Record a trace event
void trace_event_begin(TraceBuffer *buffer, const char *name);
void trace_event_end(TraceBuffer *buffer, const char *name);

// Get current resource metrics
ResourceMetrics get_resource_metrics(void);

// Export to JSON (Chrome trace format)
char *trace_buffer_to_json(TraceBuffer *buffer);

// Cleanup
void trace_buffer_destroy(TraceBuffer *buffer);

#endif // PROFILER_H