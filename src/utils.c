#include "utils.h"

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
	unsigned int size = (unsigned int)ftell(file);
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

	memcpy(result, source + start, end - start);
	result[length] = '\0';
	return result;
}

ArrayList CreateArrayList(size_t capacity)
{
	ArrayList list = { 0 };
	list.elements = (void**)malloc(capacity * sizeof(void*));
	list.size = 0;
	list.capacity = capacity;
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

void ArrayListPrint(ArrayList* list, Action printer)
{
	printf("[\n\t");
	for (int i = 0; i < list->size; i++)
	{
		void* element = list->elements[i];
		if (element != NULL) printer(element);
		else printf("NULL\n");
		if (i != list->size - 1) printf(",\n\t");
	}
	puts("\n]");
}

void ArrayListClear(ArrayList* list, Action destroy)
{
	for (int i = 0; i < list->size; i++)
	{
		void* element = list->elements[i];
		destroy(element);
	}
	list->size = 0;
}
