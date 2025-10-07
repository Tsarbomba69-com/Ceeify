#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "arena.h" // your real third-party arena
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---- Configuration ----
#ifdef ARENA_DEBUG
#define ARENA_DEBUG_MODE 1
#else
#define ARENA_DEBUG_MODE 0
#endif

// ---- Memory statistics structure ----
typedef struct ArenaStats {
  size_t total_allocated;
  size_t total_freed;
  size_t current_usage;
  size_t allocation_count;
  size_t realloc_count;
  size_t free_count;
} ArenaStats;

// ---- Extended arena wrapper ----
typedef struct Allocator {
  Arena base;       // the real arena
  ArenaStats stats; // stats data
  const char *tag;  // optional tag/name for debug
} Allocator;

// ---- Global leak tracking ----
#if ARENA_DEBUG_MODE
#include <assert.h>
#include <inttypes.h>
#include <pthread.h>

static struct {
  size_t total_allocated;
  size_t total_freed;
  size_t current_usage;
  size_t allocators_active;
  pthread_mutex_t lock;
} g_arena_global_stats = {0, 0, 0, 0, PTHREAD_MUTEX_INITIALIZER};

static inline void global_stats_add_allocator(void) {
  pthread_mutex_lock(&g_arena_global_stats.lock);
  g_arena_global_stats.allocators_active++;
  pthread_mutex_unlock(&g_arena_global_stats.lock);
}

static inline void global_stats_remove_allocator(void) {
  pthread_mutex_lock(&g_arena_global_stats.lock);
  if (g_arena_global_stats.allocators_active > 0)
    g_arena_global_stats.allocators_active--;
  pthread_mutex_unlock(&g_arena_global_stats.lock);
}

static inline void global_stats_update(int64_t diff) {
  pthread_mutex_lock(&g_arena_global_stats.lock);

  if (diff > 0) {
    size_t add = (size_t)diff;
    g_arena_global_stats.total_allocated += add;
    g_arena_global_stats.current_usage += add;
  } else if (diff < 0) {
    size_t sub = (size_t)(-diff);
    if (sub > g_arena_global_stats.current_usage) {
      // Clamp to zero if mismatch occurs
      g_arena_global_stats.total_freed += g_arena_global_stats.current_usage;
      g_arena_global_stats.current_usage = 0;
    } else {
      g_arena_global_stats.total_freed += sub;
      g_arena_global_stats.current_usage -= sub;
    }
  }

  pthread_mutex_unlock(&g_arena_global_stats.lock);
}

typedef struct {
  void *ptr;
  size_t size;
  const char *file;
  int line;
} AllocInfo;

#define MAX_TRACKED_ALLOCS 2048
static AllocInfo allocs[MAX_TRACKED_ALLOCS];
static size_t alloc_count = 0;

static void track_alloc(void *p, size_t s, const char *file, int line) {
  if (!p)
    return;

  if (alloc_count >= MAX_TRACKED_ALLOCS) {
    fprintf(stderr, "[ARENA DEBUG] Allocation tracker overflow!\n");
    return;
  }

  allocs[alloc_count++] = (AllocInfo){p, s, file, line};
}

static void track_free(void *p, const char *file, int line) {
  if (!p)
    return;

  for (size_t i = 0; i < alloc_count; ++i) {
    if (allocs[i].ptr == p) {
      // shift others down
      memmove(&allocs[i], &allocs[i + 1],
              (alloc_count - i - 1) * sizeof(AllocInfo));
      alloc_count--;
      return;
    }
  }

  fprintf(stderr, "[ARENA DEBUG] Freeing untracked pointer %p at %s:%d\n", p,
          file, line);
}

static inline void dump_allocs(void) {
  if (alloc_count == 0) {
    printf("[ARENA DEBUG] No outstanding allocations.\n");
    return;
  }

  printf("[ARENA DEBUG] Outstanding allocations (%zu):\n", alloc_count);
  for (size_t i = 0; i < alloc_count; ++i) {
    printf("  #%zu ptr=%p size=%zu from %s:%d\n", i, allocs[i].ptr,
           allocs[i].size, allocs[i].file, allocs[i].line);
  }
}

/* Leak report */
static inline void allocator_global_report_leaks(void) {
  pthread_mutex_lock(&g_arena_global_stats.lock);
  if (g_arena_global_stats.current_usage != 0) {
    fprintf(stderr,
            "\n[GlobalArenaLeakReport]\n"
            "  allocators_active: %zu\n"
            "  total_allocated:   %zu bytes\n"
            "  total_freed:       %zu bytes\n"
            "  current_usage:     %zu bytes (POSSIBLE LEAK)\n",
            g_arena_global_stats.allocators_active,
            g_arena_global_stats.total_allocated,
            g_arena_global_stats.total_freed,
            g_arena_global_stats.current_usage);
  } else {
    fprintf(stderr,
            "\n[GlobalArenaLeakReport] No leaks detected. "
            "(total allocs: %zu, frees: %zu)\n",
            g_arena_global_stats.total_allocated,
            g_arena_global_stats.total_freed);
  }
  pthread_mutex_unlock(&g_arena_global_stats.lock);
}
#endif // ARENA_DEBUG_MODE

// ---- Internal helpers ----
static inline void allocator_stats_log(const Allocator *arena, const char *msg,
                                       const char *file, int line) {
#if ARENA_DEBUG_MODE
  const char *tag = arena->tag ? arena->tag : "(unnamed)";
  fprintf(stderr,
          "[ArenaDebug] [%s] %s at %s:%d\n"
          "  total=%zu bytes | current=%zu | allocs=%zu | frees=%zu | "
          "reallocs=%zu\n",
          tag, msg, file, line, arena->stats.total_allocated,
          arena->stats.current_usage, arena->stats.allocation_count,
          arena->stats.free_count, arena->stats.realloc_count);
#else
  (void)arena;
  (void)msg;
  (void)file;
  (void)line;
#endif
}

// ---- Wrapped allocation functions ----
static inline void *allocator_alloc_dbg(Allocator *arena, size_t size,
                                        const char *file, int line) {
  void *ptr = arena_alloc(&arena->base, size);
#if ARENA_DEBUG_MODE
  if (ptr) {
    track_alloc(arena->base.begin, size, file, line);
    arena->stats.total_allocated += size;
    arena->stats.current_usage += size;
    arena->stats.allocation_count++;
    global_stats_update((int64_t)size);
    allocator_stats_log(arena, "alloc", file, line);
  }
#else
  (void)file;
  (void)line;
#endif
  return ptr;
}

static inline void *allocator_realloc_dbg(Allocator *arena, void *old_ptr,
                                          size_t old_size, size_t new_size,
                                          const char *file, int line) {
  void *ptr = arena_realloc(&arena->base, old_ptr, old_size, new_size);
#if ARENA_DEBUG_MODE
  if (ptr) {
    int64_t diff = (int64_t)new_size - (int64_t)old_size;
    arena->stats.realloc_count++;
    if (diff > 0)
      arena->stats.total_allocated += (size_t)diff;
    else
      arena->stats.total_freed += (size_t)(-diff);
    arena->stats.current_usage += diff;
    if ((int64_t)arena->stats.current_usage < 0)
      arena->stats.current_usage = 0;
    global_stats_update(diff);
    allocator_stats_log(arena, "realloc", file, line);
  }
#else
  (void)file;
  (void)line;
#endif
  return ptr;
}

static inline void allocator_free_dbg(Allocator *arena, const char *file,
                                      int line) {
  arena_free(&arena->base);
#if ARENA_DEBUG_MODE
  track_free(arena->base.begin, file, line);
  global_stats_update(-(int64_t)arena->stats.current_usage);
  arena->stats.free_count++;
  arena->stats.current_usage = 0;
  allocator_stats_log(arena, "free", file, line);
  global_stats_remove_allocator();
#else
  (void)file;
  (void)line;
#endif
}

static inline char *allocator_sprintf_dbg(Allocator *arena, const char *format,
                                          const char *file, int line, ...) {
#if ARENA_DEBUG_MODE
  va_list args;
  va_start(args, line);
  int n = vsnprintf(NULL, 0, format, args);
  va_end(args);

  if (n < 0)
    return NULL;

  char *result = (char *)allocator_alloc_dbg(arena, (size_t)n + 1, file, line);

  va_start(args, line);
  vsnprintf(result, (size_t)n + 1, format, args);
  va_end(args);

  return result;
#else
  (void)file;
  va_list args;
  va_start(args, line);
  char *result = arena_sprintf(&arena->base, format, args);
  va_end(args);
  return result;
#endif
}

// ---- Convenience macros ----
#define allocator_alloc(arena, size)                                           \
  allocator_alloc_dbg((arena), (size), __FILE__, __LINE__)

#define allocator_realloc(arena, old, old_size, new_size)                      \
  allocator_realloc_dbg((arena), (old), (old_size), (new_size), __FILE__,      \
                        __LINE__)

#define allocator_free(arena) allocator_free_dbg((arena), __FILE__, __LINE__)

#define allocator_sprintf(arena, fmt, ...)                                     \
  allocator_sprintf_dbg((arena), (fmt), __FILE__, __LINE__, __VA_ARGS__)

// ---- Init/Reset ----
static inline void allocator_init(Allocator *allocator, const char *tag) {
  memset(allocator, 0, sizeof(*allocator));
  allocator->tag = tag;
#if ARENA_DEBUG_MODE
  global_stats_add_allocator();
#endif
}

static inline void allocator_reset(Allocator *arena) {
  arena_free(&arena->base);
#if ARENA_DEBUG_MODE
  global_stats_update(-(int64_t)arena->stats.current_usage);
  memset(&arena->stats, 0, sizeof(ArenaStats));
#endif
}

#endif // ALLOCATOR_H