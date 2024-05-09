#include "parser.h"


void Parse(ArrayList* tokens)
{
	ArrayList body = CreateArrayList(100);
	Program program = { PROGRAM, body };
	for (size_t i = 0; i < tokens->size; i++)
	{
		Token* token = ArrayListGet(tokens, i);
		switch (token->type)
		{
		case KEYWORD: {
			if (strcmp(token->lexeme, "import") == 0)
			{
				Node* importStmt = CreateNode(IMPORT);
				ArrayListPush(&program.body, importStmt);
			}
		} break;
		case IDENTIFIER: {
			Node* prev = ArrayListGet(&program.body, program.body.size - 1);
			Token* next = ArrayListGet(tokens, i + 1);
			if (prev != NULL && next != NULL &&
				prev->type == IMPORT && (strcmp(next->lexeme, ",") == 0 || next->type == NEWLINE)) {
				ImportStmt* importStmt = prev->importStm;
				importStmt->moduleNames[importStmt->moduleNamesCount++] = Slice(token->lexeme, 0, strlen(token->lexeme));
			}
			else if (prev != NULL) {
				// TODO: Implemente whatever situation
				puts("");
				printf("Abstract Syntax Tree = ");
				ArrayListPrint(&program.body, PrintNode);
				puts("");
				return;
			}
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

Node* CreateNode(NodeType type)
{
	Node* node = malloc(sizeof(node));
	if (node == NULL)
	{
		fprintf(stderr, "ERROR: Could not allocate memory for AST node\n");
		return NULL;
	}

	switch (type)
	{
	case IMPORT:
		node->type = IMPORT;
		node->importStm = CreateImportStmt();
		return node;
	default:
		fprintf(stderr, "WARNING: Unrecognized node type %s\n", NodeTypeToString(type));
		return node;
	}
}

void PrintNode(Node* node)
{
	switch (node->type)
	{
	case IMPORT:
		PrintImportStmt(node->importStm);
		break;
	default:
		break;
	}
}

void PrintImportStmt(ImportStmt* stmt)
{
	const char* type = NodeTypeToString(stmt->type);
	char* moduleNames = Join(", ", stmt->moduleNames, stmt->moduleNamesCount);
	printf("{ \033[0;36mmodules\033[0m: [\033[0;33m%s\033[0m], \033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m }", moduleNames, type);
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