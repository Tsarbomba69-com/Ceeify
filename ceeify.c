#include "lexer.h"

int main(void)
{
	char* source = LoadFileText("./portal.py");
	if (source == NULL) return EXIT_FAILURE;
	Lexer lexer = CreateLexer(source);
	Tokenize(&lexer);
	return EXIT_SUCCESS;
}