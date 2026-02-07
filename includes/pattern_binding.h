typedef struct PatternBindings {
  const char **names;
  size_t count;
  size_t capacity;
  Allocator *allocator;
} PatternBindings;

static void pb_init(PatternBindings *pb, Allocator *allocator) {
  pb->count = 0;
  pb->capacity = 4;
  pb->names = allocator_alloc(allocator, pb->capacity * sizeof(char *));
  pb->allocator = allocator;
}

static bool pb_contains(PatternBindings *pb, const char *name) {
  for (size_t i = 0; i < pb->count; i++) {
    if (strcmp(pb->names[i], name) == 0)
      return true;
  }
  return false;
}

static void pb_add(PatternBindings *pb, const char *name) {
  if (pb->count == pb->capacity) {
    pb->capacity *= 2;
    pb->names = allocator_realloc(pb->allocator, pb->names, pb->count,
                                  pb->capacity * sizeof(char *));
  }
  pb->names[pb->count++] = name;
}