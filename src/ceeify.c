#include "lexer.h"
#include "parser.h"

int main(void)
{
	char* source = LoadFileText("./portal.py");
	if (source == NULL) return EXIT_FAILURE;
	Lexer lexer = CreateLexer(source);
	Tokens tokens = Tokenize(&lexer);
	printf("Total Tokens = \033[0;31m%zu\033[0m\n", tokens.size);
	puts("");
	printf("Token list = [\n");
	// ArrayListForEach(&tokens, PrintToken);
	for (size_t i = 0; i < tokens.size; i++)
	{
		// TODO: Activate array index in DEBUG MODE
		printf("%zu ", i);
		PrintToken(tokens.elements[i]);
	}
	printf("]\n");
	Parse(&tokens);
	ArrayListClear(&tokens, DestroyToken);
	return EXIT_SUCCESS;
}