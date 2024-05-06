#include "parser.h"


void Parse(ArrayList* tokens)
{
	for (size_t i = 0; i < tokens->size; i++)
	{
		Token* token = (Token*)ArrayListGet(tokens, i);
		switch (token->type)
		{
		case KEYWORD: {
			if (strcmp(token->lexeme, "import") == 0)
			{
				printf("\n\n%s\n", token->lexeme);
			}
		} break;
		case IDENTIFIER: {
			Token* tk = (Token*)ArrayListGet(tokens, i + 1);
		} break;
		default:
			break;
		}
	}
}