#include "utils.h"

// TODO: Introduce memory arena. Source: https://github.com/tsoding/arena

char* LoadFileText(const char* fileName)
{
	char* text = NULL;
	if (fileName == NULL)
	{
		fprintf(stderr, "FATAL: File name provided is not valid\n");
		return NULL;
	}

	FILE* file = fopen(fileName, "rt");
	if (file == NULL)
	{
		fprintf(stderr, "FATAL: [%s] Failed to open text file\n", fileName);
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (size <= 0)
	{
		fprintf(stderr, "FATAL: [%s] Failed to read text file\n", fileName);
		return NULL;
	}

	text = (char*)malloc((size + 1) * sizeof(char));

	if (text == NULL)
	{
		fprintf(stderr, "FATAL: [%s] Failed to allocate memory for file reading\n", fileName);
		return NULL;
	}

	size_t count = fread(text, sizeof(char), size, file);
	char* tmp = NULL;
	if (count < size)
		tmp = (char*)realloc(text, count + 1);
	if (tmp == NULL)
	{
		fprintf(stderr, "FATAL: [%s] Failed to reallocate memory for file reading\n", fileName);
		return NULL;
	}
	text = tmp;
	text[count] = '\0';
	fclose(file);
	return text;
}

void UnloadFileText(char* text)
{
	free(text);
}

char* Slice(const char* source, size_t start, size_t end)
{
	size_t length = end - start;
	if (length <= 0)
	{
		fprintf(stderr, "ERROR: the length of the slice must be greater than zero\n");
		return NULL;
	}

	char* result = (char*)malloc((length + 1) * sizeof(char));
	if (result == NULL)
	{
		fprintf(stderr, "FATAL: Failed to allocate memory for string slice\n");
		return NULL;
	}

	memcpy(result, source + start, length);
	result[length] = '\0';
	return result;
}

ArrayList CreateArrayList(size_t capacity)
{
	ArrayList list = { 0 };
	list.capacity = capacity;
	AllocateElementes(&list);
	return list;
}

void AllocateElementes(ArrayList* list)
{
	list->elements = (void**)malloc(list->capacity * sizeof(void*));
	if (list->elements == NULL) {
		fprintf(stderr, "ERROR: Could not allocate memory for array list elements");
	}
	list->size = 0;
}

ArrayList* AllocateArrayList(size_t capacity) {
	ArrayList* list = malloc(sizeof(ArrayList));
	if (list == NULL) {
		fprintf(stderr, "ERROR: Could not allocate memory for array list");
		return NULL;
	}
	list->capacity = capacity;
	AllocateElementes(list);
	return list;
}

void ArrayListPush(ArrayList* list, void* value)
{
	if (list->size == list->capacity) {
		size_t cap = list->capacity * 2;
		void** elements = (void**)realloc(list->elements, cap * sizeof(void*));
		if (elements == NULL) {
			fprintf(stderr, "ERROR: Failed to resize array list\n");
			return;
		}
		list->elements = elements;
		list->capacity = cap;
	}
	list->elements[list->size++] = value;
}

void* ArrayListPop(ArrayList* list) {
	if (list->size == 0) {
		return NULL; // ArrayList is empty, return NULL
	}

	void* element = list->elements[list->size - 1]; // Get the last element
	list->size--; // Decrement the size
	return element; // Return the last element
}

char* Repeat(const char* str, size_t count) {
	size_t str_len = strlen(str);
	size_t result_len = str_len * count;
	char* result = (char*)malloc((result_len + 1) * sizeof(char));

	if (result == NULL)
	{
		fprintf(stderr, "ERROR: Could not allocate memory to repeat \"%s\" %zu times\n", str, count);
		return NULL;
	}

	for (size_t i = 0; i < count; i++) {
		memcpy(result + (i * str_len), str, str_len);
	}

	result[result_len] = '\0';
	return result;
}

bool Any(void* arr[], size_t size, void* el, CompareFn fn)
{
	for (size_t i = 0; i < size; i++) {
		if (fn(arr[i], el)) {
			return true;
		}
	}
	return false;
}

char* Join(char* separator, char** items, size_t count)
{
	if (count == 0)
	{
		fprintf(stderr, "ERROR: Count must be greater than zero\n");
		return NULL;
	}

	size_t separatorLength = strlen(separator);
	size_t totalLength = 0;
	for (size_t i = 0; i < count; i++) {
		totalLength += strlen(items[i]) + 2;
	}

	totalLength += (count - 1) * separatorLength + 1; // Add space for separators and null terminator

	char* result = (char*)malloc(totalLength);
	if (result == NULL)
	{
		fprintf(stderr, "ERROR: Could not allocate memory for slice buffer\n");
		return NULL;
	}

	if (strcpy_s(result, totalLength, items[0]) != 0)
	{
		free(result);
		return NULL;
	}

	for (size_t i = 1; i < count; i++)
	{
		if (strcat_s(result, totalLength, separator) != 0 ||
			strcat_s(result, totalLength, items[i]) != 0)
		{
			free(result);
			return NULL;
		}
	}

	return result;
}

const char* TextFormat(const char* text, ...)
{
#ifndef MAX_TEXTFORMAT_BUFFERS
#define MAX_TEXTFORMAT_BUFFERS      4        // Maximum number of static buffers for text formatting
#endif
#ifndef MAX_TEXT_BUFFER_LENGTH
#define MAX_TEXT_BUFFER_LENGTH   1024        // Maximum size of static text buffer
#endif

	// We create an array of buffers so strings don't expire until MAX_TEXTFORMAT_BUFFERS invocations
	static char buffers[MAX_TEXTFORMAT_BUFFERS][MAX_TEXT_BUFFER_LENGTH] = { 0 };
	static int index = 0;

	char* currentBuffer = buffers[index];
	memset(currentBuffer, 0, MAX_TEXT_BUFFER_LENGTH);   // Clear buffer before using

	va_list args;
	va_start(args, text);
	int requiredByteCount = vsnprintf(currentBuffer, MAX_TEXT_BUFFER_LENGTH, text, args);
	va_end(args);

	// If requiredByteCount is larger than the MAX_TEXT_BUFFER_LENGTH, then overflow occured
	if (requiredByteCount >= MAX_TEXT_BUFFER_LENGTH)
	{
		// Inserting "..." at the end of the string to mark as truncated
		char* truncBuffer = buffers[index] + MAX_TEXT_BUFFER_LENGTH - 4; // Adding 4 bytes = "...\0"
		sprintf_s(truncBuffer, 4, "...");
	}

	index += 1;     // Move to next buffer for next function call
	if (index >= MAX_TEXTFORMAT_BUFFERS) index = 0;

	return currentBuffer;
}

void ArrayListForEach(ArrayList* list, Action callback)
{
	for (size_t i = 0; i < list->size; i++)
	{
#if DEBUG
		printf("%zu ", i);
#endif
		void* element = list->elements[i];
		if (element != NULL) callback(element);
	}
}

void ArrayListClear(ArrayList* list, Action destroy)
{
	for (size_t i = 0; i < list->size; i++)
	{
		void* element = list->elements[i];
		destroy(element);
	}
	list->size = 0;
}
