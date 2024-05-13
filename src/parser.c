#include "parser.h"


void Parse(ArrayList* tokens)
{
	Program program = CreateArrayList(100);
	for (size_t i = 0; i < tokens->size; i++)
	{
		Token* token = ArrayListGet(tokens, i);
		switch (token->type)
		{
		case KEYWORD: {
			if (strcmp(token->lexeme, "import") == 0)
			{
				Node* importStmt = CreateNode(IMPORT);
				ArrayListPush(&program, importStmt);
			}
		} break;
		case IDENTIFIER: {
			Node* node = ArrayListGet(&program, program.size - 1);
			Token* prev = ArrayListGet(tokens, i - 1);
			Token* next = ArrayListGet(tokens, i + 1);

			if ((node == NULL || prev->type == NEWLINE) && strcmp(next->lexeme, "=") == 0) {
				node = CreateNode(ASSIGNMENT);
				Assign* assign = node->assignStmt;
				Name* var = (CreateNode(VARIABLE))->variable;
				var->id = token->lexeme;
				var->ctx = STORE;
				assign->target = var;
				assign->value = NULL;
				ArrayListPush(&program, node);
				break;
			}

			if (node != NULL && node->type == ASSIGNMENT && node->assignStmt->target->ctx == STORE) {
				Assign* assign = node->assignStmt;
				Node* var = CreateNode(VARIABLE);
				var->variable->ctx = LOAD;
				var->variable->id = token->lexeme;
				assign->value = var;
				break;
			}

			if (node != NULL && next != NULL &&
				node->type == IMPORT && (strcmp(next->lexeme, ",") == 0 || next->type == NEWLINE)) {
				ImportStmt* importStmt = node->importStm;
				ArrayListPush(&importStmt->modules, Slice(token->lexeme, 0, strlen(token->lexeme)));
			}
		} break;
		case STRING: {
			Node* node = ArrayListGet(&program, program.size - 1);

			if (node != NULL && node->type == ASSIGNMENT) {
				Node* literal = CreateNode(LITERAL);
				literal->literal = CreateLiteral(token->lexeme);
				node->assignStmt->value = literal;
			}
		} break;
		default:
			break;
		}
	}
	puts("");
	printf("Abstract Syntax Tree = [\n");
	ArrayListForEach(&program, PrintNode);
	printf("]\n");
}

ImportStmt* CreateImportStmt()
{
	ImportStmt* importStmt = malloc(sizeof(importStmt));
	if (importStmt == NULL)
	{
		fprintf(stderr, "ERROR: Could not allocate memory for import statement\n");
		return;
	}

	importStmt->modules = CreateArrayList(10);
	return importStmt;
}

Literal* CreateLiteral(char* value)
{
	Literal* literal = malloc(sizeof(literal));
	if (literal == NULL)
	{
		fprintf(stderr, "ERROR: Could not allocate memory for import statement\n");
		return;
	}

	literal->value = value;
	return literal;
}

Assign* CreateAssignStmt()
{
	Assign* assignStmt = malloc(sizeof(assignStmt));
	if (assignStmt == NULL)
	{
		fprintf(stderr, "ERROR: Could not allocate memory for assignment statement\n");
		return;
	}

	assignStmt->target = NULL;
	return assignStmt;
}

Name* CreateNameExpr()
{
	Name* variable = malloc(sizeof(variable));
	if (variable == NULL)
	{
		fprintf(stderr, "ERROR: Could not allocate memory for variable expression\n");
		return;
	}

	variable->id = NULL;
	return variable;
}

Node* CreateNode(NodeType type)
{
	Node* node = malloc(sizeof(node));
	if (node == NULL)
	{
		fprintf(stderr, "ERROR: Could not allocate memory for AST node\n");
		return NULL;
	}

	node->type = type;
	switch (type)
	{
	case IMPORT:
		node->importStm = CreateImportStmt();
		return node;
	case ASSIGNMENT:
		node->assignStmt = CreateAssignStmt();
		return node;
	case VARIABLE:
		node->variable = CreateNameExpr();
		return node;
	case LITERAL:
		return node;
	default:
		fprintf(stderr, "WARNING: Unrecognized node type %s\n", NodeTypeToString(type));
		return node;
	}
}

void PrintNode(Node* node)
{
	if (node == NULL) return;
	char* type = NodeTypeToString(node->type);
	switch (node->type)
	{
	case IMPORT:
		PrintImportStmt(node->importStm);
		break;
	case ASSIGNMENT:
		PrintVar(node->assignStmt->target);
		printf(" = ");
		PrintNode(node->assignStmt->value);
		break;
	case VARIABLE:
		PrintVar(node->variable);
		break;
	case LITERAL:
		printf("{ \033[0;36mvalue\033[0m: \033[0;33m\"%s\"\033[0m, \033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m }", node->literal->value, NodeTypeToString(node->type));
		break;
	default: {
		fprintf(stderr, "WARNING: Not implemented for \"%s\"\n", type);
	} break;
	}
	printf(",\n");
}

void PrintVar(Name* variable)
{
	printf("{ \033[0;36midentifer\033[0m: \033[0;33m%s\033[0m, \033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36m\033[0;36mctx\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m }", variable->id, NodeTypeToString(VARIABLE), CtxToString(variable));
}

void PrintImportStmt(ImportStmt* stmt)
{
	const char* type = NodeTypeToString(IMPORT);
	printf("{ \033[0;36mmodules\033[0m: [ ");
	ArrayListForEach(&stmt->modules, Print);
	printf("]");
	printf(", \033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m }", type);
}

const char* NodeTypeToString(NodeType type)
{
	switch (type)
	{
	case IMPORT: return "IMPORT";
	case PROGRAM: return "PROGRAM";
	case VARIABLE: return "VARIABLE";
	case ASSIGNMENT: return "ASSIGNMENT";
	case LITERAL: return "LITERAL";
	default: return "UNKNOWN";
	}
}

const char* CtxToString(Name* var)
{
	switch (var->ctx)
	{
	case LOAD: return "LOAD";
	case STORE: return "STORE";
	case DEL: return "DEL";
	default: return "UNKNOWN";
	}
}