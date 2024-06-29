#include "utils.h"

#define ARENA_IMPLEMENTATION

#include "arena.h"

static Arena default_arena = {0};

void PrintArena() {
    Region *current = default_arena.begin;
    size_t total_elements = 0;

    for (size_t i = 1; current != NULL; ++i) {
        puts("");
        printf("Region[\033[0;31m%zu\033[0m]:\n", i);
        printf("  Size: \033[0;31m%zu\033[0m bytes\n", current->count);
        printf("  Capacity: \033[0;31m%zu\033[0m bytes\n", current->capacity);
        total_elements += current->count;
        current = current->next;
    }

    printf("Total memory used: \033[0;31m%zu\033[0m bytes\n", total_elements);
    puts("");
}

void *AllocateContext(size_t size) {
    assert(&default_arena);
    return arena_alloc(&default_arena, size);
}

void *ReallocateContext(void *oldptr, size_t oldptr_size, size_t size) {
    assert(&default_arena);
    return arena_realloc(&default_arena, oldptr, oldptr_size, size);
}

void FreeContext() {
    arena_free(&default_arena);
}

char *LoadFileText(const char *fileName) {
    char *text = NULL;
    if (fileName == NULL) {
        fprintf(stderr, "FATAL: File name provided is not valid\n");
        return NULL;
    }

    FILE *file = fopen(fileName, "rt");
    if (file == NULL) {
        fprintf(stderr, "FATAL: [%s] Failed to open text file\n", fileName);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size <= 0) {
        fprintf(stderr, "FATAL: [%s] Failed to read text file\n", fileName);
        return NULL;
    }

    text = AllocateContext((size + 1) * sizeof(char));

    if (text == NULL) {
        fprintf(stderr, "FATAL: [%s] Failed to allocate memory for file reading\n", fileName);
        return NULL;
    }

    size_t count = fread(text, sizeof(char), size, file);
    if (count < size)
        text = ReallocateContext(text, size, count + 1);

    text[count] = '\0';
    fclose(file);
    return text;
}

bool SaveFileText(const char *fileName, char *text) {
    if (fileName == NULL) {
        fprintf(stderr, "ERROR: File name provided is not valid to save\n");
        return false;
    }

    FILE *file = fopen(fileName, "wt");
    if (file == NULL) {
        fprintf(stderr, "ERROR: [%s] Failed to open text file\n", fileName);
        return false;
    }

    int count = fprintf(file, "%s", text);
    if (count == 0) {
        fprintf(stderr, "ERROR: [%s] Failed to write text file\n", fileName);
        fclose(file);
        return false;
    }

    fprintf(stdout, "SUCCESS: [%s] Text file saved successfully\n", fileName);
    fclose(file);
    return true;
}

char *Slice(const char *source, size_t start, size_t end) {
    size_t length = end - start;
    if (length <= 0) {
        fprintf(stderr, "ERROR: the length of the slice must be greater than zero\n");
        return NULL;
    }

    char *result = AllocateContext((length + 1) * sizeof(char));
    if (result == NULL) {
        fprintf(stderr, "FATAL: Failed to allocate memory for string slice\n");
        return NULL;
    }

    memcpy(result, source + start, length);
    result[length] = '\0';
    return result;
}

void AllocateElementes(ArrayList *list) {
    list->elements = AllocateContext(list->capacity * sizeof(void *));
    if (list->elements == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for array list elements");
    }
    list->size = 0;
}

ArrayList *AllocateArrayList(size_t capacity) {
    ArrayList *list = AllocateContext(sizeof(ArrayList));
    if (list == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for array list");
        return NULL;
    }
    list->capacity = capacity;
    AllocateElementes(list);
    return list;
}

char *Repeat(const char *str, size_t count) {
    size_t str_len = strlen(str);
    size_t result_len = str_len * count;
    char *result = AllocateContext((result_len + 1) * sizeof(char));

    if (result == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory to repeat \"%s\" %zu times\n", str, count);
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        memcpy(result + (i * str_len), str, str_len);
    }

    result[result_len] = '\0';
    return result;
}

bool Any(void *arr[], size_t size, void *el, CompareFn fn) {
    for (size_t i = 0; i < size; i++) {
        if (fn(arr[i], el)) {
            return true;
        }
    }
    return false;
}

char *Join(char *separator, char **items, size_t count) {
    if (count == 0) {
        fprintf(stderr, "ERROR: Count must be greater than zero\n");
        return NULL;
    }

    size_t separatorLength = strlen(separator);
    size_t totalLength = 0;
    for (size_t i = 0; i < count; i++) {
        totalLength += strlen(items[i]) + 2;
    }

    totalLength += (count - 1) * separatorLength + 1; // Add space for separators and null terminator

    char *result = AllocateContext(totalLength);
    if (result == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for slice buffer\n");
        return NULL;
    }

    if (strcpy_s(result, totalLength, items[0]) != 0) {
        free(result);
        return NULL;
    }

    for (size_t i = 1; i < count; i++) {
        if (strcat_s(result, totalLength, separator) != 0 ||
            strcat_s(result, totalLength, items[i]) != 0) {
            free(result);
            return NULL;
        }
    }

    return result;
}

const char *TextFormat(const char *text, ...) {
#ifndef MAX_TEXTFORMAT_BUFFERS
#define MAX_TEXTFORMAT_BUFFERS      4        // Maximum number of static buffers for text formatting
#endif
#ifndef MAX_TEXT_BUFFER_LENGTH
#define MAX_TEXT_BUFFER_LENGTH   1024        // Maximum size of static text buffer
#endif

    // We create an array of buffers so strings don't expire until MAX_TEXTFORMAT_BUFFERS invocations
    static char buffers[MAX_TEXTFORMAT_BUFFERS][MAX_TEXT_BUFFER_LENGTH] = {0};
    static int index = 0;

    char *currentBuffer = buffers[index];
    memset(currentBuffer, 0, MAX_TEXT_BUFFER_LENGTH);   // Clear buffer before using

    va_list args;
    va_start(args, text);
    int requiredByteCount = vsnprintf(currentBuffer, MAX_TEXT_BUFFER_LENGTH, text, args);
    va_end(args);

    // If requiredByteCount is larger than the MAX_TEXT_BUFFER_LENGTH, then overflow occured
    if (requiredByteCount >= MAX_TEXT_BUFFER_LENGTH) {
        // Inserting "..." at the end of the string to mark as truncated
        char *truncBuffer = buffers[index] + MAX_TEXT_BUFFER_LENGTH - 4; // Adding 4 bytes = "...\0"
        sprintf_s(truncBuffer, 4, "...");
    }

    index += 1;     // Move to next buffer for next function call
    if (index >= MAX_TEXTFORMAT_BUFFERS) index = 0;

    return currentBuffer;
}

bool IsFloat(const char *str) {
    char *endptr;
    strtod(str, &endptr);
    return (endptr != str) && (errno != ERANGE);
}

bool IsInteger(const char *str) {
    char *endptr;
    strtol(str, &endptr, 10);
    return (endptr != str) && (errno != ERANGE);
}
