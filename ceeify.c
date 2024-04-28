#include "lexer.h"

int main(void)
{
	char* source = LoadFileText("./portal.py");
	if (source == NULL) return EXIT_FAILURE;
	Lexer lexer = { source, 0 };
	Tokenize(&lexer);
	return EXIT_SUCCESS;
}