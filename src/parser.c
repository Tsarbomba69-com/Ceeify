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
				ImportStmt* importStmt = CreateImportStmt();
				Token* next = (Token*)ArrayListGet(tokens, i + 1);
				size_t pos = i;
				switch (next->type)
				{
				case IDENTIFIER: {
					importStmt->moduleNames[importStmt->moduleNamesCount++] = Slice(next->lexeme, 0, strlen(next->lexeme));

					PrintImportStmt(importStmt);
				} break;
				default:
					fprintf(stderr, "SyntaxError: invalid syntax");
					return;
				}
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

ImportStmt* CreateImportStmt()
{
	ImportStmt* importStmt = (ImportStmt*)malloc(sizeof(ImportStmt));
	if (importStmt == NULL)
	{
		fprintf(stderr, "ERROR: Could not allocate memory for import statement\n");
		return;
	}

	importStmt->moduleNamesCount = 0;
	importStmt->type = IMPORT;
	return importStmt;
}

void PrintImportStmt(ImportStmt* stmt)
{
	const char* type = NodeTypeToString(stmt->type);
	char* moduleNames = Join(", ", stmt->moduleNames, stmt->moduleNamesCount);
	printf("{ \033[0;36mmoduleNames\033[0m: \033[0;33m[%s]\033[0m, \033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m }", moduleNames, type);
	free(moduleNames);
}

const char* NodeTypeToString(NodeType type)
{
	switch (type)
	{
	case IMPORT: return "IMPORT";
	default: return "UNKNOWN";
	}
}