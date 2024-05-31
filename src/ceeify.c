#include "lexer.h"
#include "parser.h"

// Write script to generate typed arraylist
// Abstract shared types (Token, Node) away fot the script to work

int main(void)
{
	char* source = LoadFileText("./portal.py");
	if (source == NULL) return EXIT_FAILURE;
	Lexer lexer = CreateLexer(source);
	Tokens tokens = Tokenize(&lexer);
	printf("Total Tokens = \033[0;31m%zu\033[0m\n", tokens.size);
	puts("");
	printf("Token list = [\n");
	ArrayListForEach(&tokens, PrintToken);
	printf("]\n");
	PrintArena();
	Node* root = ParseBlock(&tokens);
	// Parse(&tokens);
	PrintArena();
	FreeContext();
	PrintArena();
	return EXIT_SUCCESS;
}