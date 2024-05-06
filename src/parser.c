#include "parser.h"


void Parse(ArrayList* tokens)
{
	ASTNode* node = NULL;
	for (size_t i = 0; i < tokens->size; i++)
	{
		Token* token = &tokens[i];
		switch (token->type)
		{
		case KEYWORD:
			node = ParseKeyword(token);
			break;
		default:
			break;
		}
	}
}

ASTNode* ParseKeyword(Token* token)
{
	/*if (strcmp(token->lexeme, "import"))
		return CreateStmtNode();*/
}