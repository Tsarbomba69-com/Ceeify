#include "parser.h"

const char* BIN_OPERATORS[] = { "+", "-", "*", "/", "%", ">", "<", "==", "&", "|", "^", "~", "&&", "||" };

void Parse(ArrayList* tokens)
{
	Program program = CreateArrayList(100);
	for (size_t i = 0; i < tokens->size; i++)
	{
		Token* token = ArrayListGet(tokens, i);
		Node* node = ArrayListGet(&program, program.size - 1);
		Token* prev = ArrayListGet(tokens, i - 1);
		Token* next = ArrayListGet(tokens, i + 1);

		switch (token->type)
		{
		case KEYWORD: {
			if (strcmp(token->lexeme, "import") == 0)
			{
				Node* importStmt = CreateNode(IMPORT);
				ArrayListPush(&program, importStmt);
			}
		} break;
		case OPERATOR: {
			if (node != NULL && node->type == ASSIGNMENT && strcmp(next->lexeme, "-") == 0) {
				Node* operand = CreateNode(UNARY_OPERATION);
				operand->unOp = CreateUnaryOp(next->lexeme);
				operand->depth += node->depth;
				node->assignStmt->value = operand;
			}
		} break;
		case IDENTIFIER: {
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
				var->depth += node->depth;
				if (Contains(BIN_OPERATORS, ARRAYSIZE(BIN_OPERATORS), next->lexeme, StrEQ)) {
					Tokens expression = CreateArrayList(20);
					for (size_t j = i + 1; token->type != NEWLINE; ++j) {
						ArrayListPush(&expression, token);
						token = ArrayListGet(tokens, j);
						i = j;
					}
					expression = InfixToPostfix(&expression);
					assign->value = ShantingYard(&expression);
					break;
				}
				if (assign->value != NULL && assign->value->type == UNARY_OPERATION)
					assign->value->unOp->operand = var;
				else
					assign->value = var;
				break;
			}

			if (node != NULL && next != NULL &&
				node->type == IMPORT && (strcmp(next->lexeme, ",") == 0 || next->type == NEWLINE)) {
				ImportStmt* importStmt = node->importStm;
				ArrayListPush(&importStmt->modules, Slice(token->lexeme, 0, strlen(token->lexeme)));
			}
		} break;
		case INTEGER: {
			if (node != NULL && node->type == ASSIGNMENT) {
				Node* assignV = node->assignStmt->value;
				Node* literal = CreateNode(LITERAL);
				literal->literal = CreateLiteral(token->lexeme);
				literal->depth += node->depth;
				if (assignV != NULL && assignV->type == UNARY_OPERATION) assignV->unOp->operand = literal;
				else node->assignStmt->value = literal;
			}
		} break;
		case STRING: {
			if (node != NULL && node->type == ASSIGNMENT) {
				Node* literal = CreateNode(LITERAL);
				literal->literal = CreateLiteral(token->lexeme);
				literal->depth += node->depth;
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

UnaryOperation* CreateUnaryOp(char* op)
{
	UnaryOperation* operation = malloc(sizeof(operation));
	if (operation == NULL)
	{
		fprintf(stderr, "ERROR: Could not allocate memory for unary operation expression\n");
		return;
	}

	operation->operator = op;
	return operation;
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
	node->depth = 1;
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
	case UNARY_OPERATION:
	case LITERAL:
	case BINARY_OPERATION:
		return node;
	default:
		fprintf(stderr, "WARNING: Unrecognized node type %s\n", NodeTypeToString(type));
		return node;
	}
}

Tokens InfixToPostfix(Tokens* tokens)
{
	Tokens stack = CreateArrayList(10);
	Tokens postfix = CreateArrayList(10);

	for (size_t i = 0; i < tokens->size; i++)
	{
		Token* token = ArrayListGet(tokens, i);
		if (token->type == IDENTIFIER || token->type == INTEGER || token->type == FLOAT)
		{
			ArrayListPush(&postfix, token);
		}
		else if (strcmp(token->lexeme, "(") == 0)
		{
			ArrayListPush(&stack, token);
		}
		else if (strcmp(token->lexeme, ")") == 0)
		{
			Token* last = ArrayListGet(&stack, stack.size - 1);
			while (stack.size > 0 && strcmp(last->lexeme, "(") != 0)
			{
				ArrayListPush(&postfix, ArrayListPop(&stack));
			}
			if (stack.size > 0 && strcmp(last->lexeme, "(") == 0)
			{
				ArrayListPop(&stack); // Pop the open bracket
			}
		}
		else
		{
			Token* last = ArrayListGet(&stack, stack.size - 1);
			while (stack.size > 0 && Precedence(last->lexeme[0]) >= Precedence(token->lexeme[0]) && strcmp(last->lexeme, "(") != 0)
			{
				ArrayListPush(&postfix, ArrayListPop(&stack));
				if (stack.size > 0)
				{
					last = ArrayListGet(&stack, stack.size - 1);
				}
			}
			ArrayListPush(&stack, token);
		}
	}

	while (stack.size > 0)
	{
		ArrayListPush(&postfix, ArrayListPop(&stack));
	}
	return postfix;
}

Node* ShantingYard(Tokens* tokens)
{
	ArrayList stack = CreateArrayList(10);
	size_t max_depth = 0;
	for (size_t i = 0; i < tokens->size; i++)
	{
		Token* token = ArrayListGet(tokens, i);
		switch (token->type) {
		case IDENTIFIER: {
			Node* var = CreateNode(VARIABLE);
			var->variable->ctx = LOAD;
			var->variable->id = token->lexeme;
			var->depth = max_depth + 1;
			max_depth = var->depth;
			ArrayListPush(&stack, var);
		} break;
		case INTEGER:
		case FLOAT: {
			Node* literal = CreateNode(LITERAL);
			literal->literal = CreateLiteral(token->lexeme);
			literal->depth = max_depth + 1;
			max_depth = literal->depth;
			ArrayListPush(&stack, literal);
		} break;
		case DELIMITER:
			break;
		default: {
			Node* right = ArrayListPop(&stack);
			Node* left = ArrayListPop(&stack);
			Node* bin = CreateBinOp(token, left, right);
			bin->depth = max_depth;
			max_depth -= max_depth > 0;
			ArrayListPush(&stack, bin);
		} break;
		}
	}
	Node* root = ArrayListPop(&stack);
	root->depth = max_depth;
	return root;
}

Node* CreateBinOp(Token* token, Node* left, Node* right)
{
	Node* node = CreateNode(BINARY_OPERATION);
	node->depth = left->depth - 1;
	node->binOp = malloc(sizeof(node->binOp));
	if (node->binOp == NULL) {
		fprintf(stderr, "ERROR: Could not allocate memory for binary operation\n");
		return NULL;
	}
	node->binOp->left = left;
	node->binOp->right = right;
	node->binOp->operator = token->lexeme;
	return node;
}

void PrintNode(Node* node)
{
	if (node == NULL) return;
	char* type = NodeTypeToString(node->type);
	char* spaces = Repeat(" ", node->depth * 4);
	switch (node->type)
	{
	case IMPORT:
		PrintImportStmt(node->importStm);
		printf(",\n");
		break;
	case UNARY_OPERATION:
		printf("{ \n%s\033[0;36moperator\033[0m: ", spaces);
		Print(node->unOp->operator);
		printf("\n%s\033[0;36moperand\033[0m: ", spaces);
		PrintNode(node->unOp->operand);
		printf(", \n%s\033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m \n%s}",
			spaces, type, Slice(spaces, 0, 4 * (node->depth - (node->depth > 1))));
		break;
	case BINARY_OPERATION:
		printf("{ \n%s\033[0;36moperator\033[0m: ", spaces);
		Print(node->binOp->operator);
		printf("\n%s\033[0;36mleft\033[0m: ", spaces);
		PrintNode(node->binOp->left);
		printf(",\n%s\033[0;36mright\033[0m: ", spaces);
		PrintNode(node->binOp->right);
		printf(", \n%s\033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m \n%s}",
			spaces, type, Slice(spaces, 0, 4 * (node->depth - (node->depth > 1))));
		break;
	case ASSIGNMENT:
		printf("{ \n%s\033[0;36mtarget\033[0m: ", spaces);
		PrintVar(node->assignStmt->target);
		printf(", \n%s\033[0;36mexpression\033[0m: ", spaces);
		PrintNode(node->assignStmt->value);
		printf(", \n%s\033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m \n}", spaces, type);
		printf(",\n");
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
	case UNARY_OPERATION: return "UNARY OPERATION";
	case BINARY_OPERATION: return "BINARY OPERATION";
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

size_t Precedence(char op) {
	switch (op) {
	case '+':
	case '-':
		return 1;
	case '*':
	case '/':
		return 2;
	case '^':
		return 3;
	default:
		return -1;
	}
}