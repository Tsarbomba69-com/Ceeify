#include "utils.h"

char* LoadFileText(const char* fileName)
{
	char* text = NULL;
	if (fileName == NULL)
	{
		printf("FATAL: File name provided is not valid\n");
		return NULL;
	}

	FILE* file = fopen(fileName, "rt");
	if (file == NULL)
	{
		printf("FATAL: [%s] Failed to open text file\n", fileName);
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	unsigned int size = (unsigned int)ftell(file);
	fseek(file, 0, SEEK_SET);

	if (size <= 0)
	{
		printf("FATAL: [%s] Failed to read text file\n", fileName);
		return NULL;
	}

	text = (char*)malloc((size + 1) * sizeof(char));

	if (text == NULL)
	{
		printf("FATAL: [%s] Failed to allocated memory for file reading\n", fileName);
		return NULL;
	}

	size_t count = fread(text, sizeof(char), size, file);
	char* tmp = NULL;
	if (count < size) tmp = (char*)realloc(text, count + 1);
	if (tmp == NULL)
	{
		printf("FATAL: [%s] Failed to reallocated memory for file reading\n", fileName);
		return NULL;
	}
	text = tmp;
	text[count] = '\0';
	fclose(file);
	return text;
}
