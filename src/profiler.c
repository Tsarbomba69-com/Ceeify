#include "profiler.h"

static uint64_t get_timestamp_us(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

ResourceMetrics get_resource_metrics(void) {
  ResourceMetrics metrics = {0};
  struct rusage usage;

  if (getrusage(RUSAGE_SELF, &usage) == 0) {
    metrics.max_rss_kb = usage.ru_maxrss;
    metrics.cpu_us = (usage.ru_utime.tv_sec * 1000000 + 
                      usage.ru_utime.tv_usec) +
                     (usage.ru_stime.tv_sec * 1000000 + 
                      usage.ru_stime.tv_usec);
  }

  // Try to read I/O stats from /proc (Linux specific)
  FILE *stat_file = fopen("/proc/self/io", "r");
  if (stat_file) {
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), stat_file)) {
      if (strncmp(buffer, "read_bytes:", 11) == 0) {
        sscanf(buffer, "read_bytes: %lu", &metrics.read_bytes);
      } else if (strncmp(buffer, "write_bytes:", 12) == 0) {
        sscanf(buffer, "write_bytes: %lu", &metrics.write_bytes);
      }
    }
    fclose(stat_file);
  }

  return metrics;
}

TraceBuffer trace_buffer_create(Allocator* allocator, size_t initial_capacity) {
  TraceBuffer buffer = {0};

  buffer.events = (TraceEvent*)allocator_alloc(allocator, initial_capacity * sizeof(TraceEvent));
  if (!buffer.events) {
    slog_error("Failed to allocate memory for trace events");
    return buffer;
  }

  buffer.count = 0;
  buffer.capacity = initial_capacity;
  buffer.allocator = allocator;
  return buffer;
}

static void trace_buffer_resize(TraceBuffer *buffer, size_t new_capacity) {
  TraceEvent *new_events = allocator_realloc(buffer->allocator, buffer->events,
                                             buffer->capacity * sizeof(TraceEvent),
                                    new_capacity * sizeof(TraceEvent));
  if (new_events) {
    buffer->events = new_events;
    buffer->capacity = new_capacity;
  }
}

void trace_event_begin(TraceBuffer *buffer, const char *name) {
  if (buffer->count >= buffer->capacity) {
    trace_buffer_resize(buffer, buffer->capacity * 2);
  }

  TraceEvent *event = &buffer->events[buffer->count++];
  event->name = arena_strdup(&buffer->allocator->base, name);
  event->phase = 'B';
  event->timestamp_us = get_timestamp_us();
  event->duration_us = 0;
  event->metrics = get_resource_metrics();
}

void trace_event_end(TraceBuffer *buffer, const char *name) {
  // Find matching begin event
  for (int i = buffer->count - 1; i >= 0; i--) {
    if (buffer->events[i].phase == 'B' && 
        strcmp(buffer->events[i].name, name) == 0) {
      
      if (buffer->count >= buffer->capacity) {
        trace_buffer_resize(buffer, buffer->capacity * 2);
      }

      TraceEvent *end_event = &buffer->events[buffer->count++];
      end_event->name = arena_strdup(&buffer->allocator->base, name);
      end_event->phase = 'E';
      end_event->timestamp_us = get_timestamp_us();
      end_event->metrics = get_resource_metrics();
      end_event->duration_us = end_event->timestamp_us - 
                               buffer->events[i].timestamp_us;
      return;
    }
  }

  slog_error("No matching begin event found for: %s", name);
}

char *trace_buffer_to_json(TraceBuffer *buffer) {
  cJSON *root = cJSON_CreateArray();
  if (!root) {
    slog_error("Failed to create JSON array");
    return NULL;
  }

  for (size_t i = 0; i < buffer->count; i++) {
    TraceEvent *event = &buffer->events[i];
    cJSON *obj = cJSON_CreateObject();
    
    cJSON_AddStringToObject(obj, "name", event->name);
    cJSON_AddStringToObject(obj, "ph", 
                           event->phase == 'B' ? "B" : "E");
    cJSON_AddNumberToObject(obj, "ts", event->timestamp_us);
    
    if (event->phase == 'E') {
      cJSON_AddNumberToObject(obj, "dur", event->duration_us);
    }

    // Add resource metrics
    cJSON *args = cJSON_CreateObject();
    cJSON_AddNumberToObject(args, "max_rss_kb", event->metrics.max_rss_kb);
    cJSON_AddNumberToObject(args, "read_bytes", event->metrics.read_bytes);
    cJSON_AddNumberToObject(args, "write_bytes", event->metrics.write_bytes);
    cJSON_AddNumberToObject(args, "cpu_us", event->metrics.cpu_us);
    cJSON_AddItemToObject(obj, "args", args);

    cJSON_AddItemToArray(root, obj);
  }

  char *json_string = cJSON_Print(root);
  cJSON_Delete(root);
  return json_string;
}

void trace_buffer_destroy(TraceBuffer *buffer) {
  if (!buffer) return;
  allocator_free(buffer->allocator);
}