#include "parser.h"

// TODO: Try writting a second Parse function, implemented using Bottom-up algorithm
// TODO: Implement semantic analysis


// Current token index
static size_t i = 0;

const char* BIN_OPERATORS[] = {
	"+", "-", "*", "/", "%", ">", "<", "==", "&", "|", "^", "~", "&&", "||",
	"//", "==", "!=", "**", ">=", "<=", "&&", "||", "+=", "-=", "*=", "/=", "%=", "//=", "**=", "<<", ">>"
};

const char* UNARY_OPERATORS[] = { "+", "-", "~", "not" };

// WARNING: Soon to be deprecated
void Parse(Tokens* tokens)
{
	Program* program = AllocateArrayList(100);
	Program* context = program;
	ArrayList stack = CreateArrayList(10);
	ArrayListPush(&stack, context);
	for (size_t i = 0; i < tokens->size; i++)
	{
		Token* token = ArrayListGet(tokens, i);
		Node* node = ArrayListGet(context, context->size - 1);
		Token* prev = ArrayListGet(tokens, i - 1);
		Token* next = ArrayListGet(tokens, i + 1);
		Program* frame = ArrayListGet(&stack, stack.size - 1);
		switch (token->type)
		{
		case KEYWORD: {
			if (strcmp(token->lexeme, "import") == 0)
			{
				Node* importStmt = CreateNode(IMPORT);
				ArrayListPush(context, importStmt);
				break;
			}

			if (strcmp(token->lexeme, "if") == 0) {
				Node* if_node = CreateNode(IF);
				ArrayListPush(context, if_node);
				context = if_node->if_stmt->body;
				ArrayListPush(&stack, context);
				break;
			}
		} break;
		case OPERATOR: {
			if (node != NULL && node->type == ASSIGNMENT && strcmp(next->lexeme, "-") == 0) {
				Node* operand = CreateNode(UNARY_OPERATION);
				operand->unOp = CreateUnaryOp(next->lexeme);
				node->assign_stmt->value = operand;
			}
		} break;
		case IDENTIFIER: {
			if (node != NULL && node->type == IF) {
				if (Any(BIN_OPERATORS, ARRAYSIZE(BIN_OPERATORS), next->lexeme, StrEQ)) {
					Tokens expression = CollectExpression(tokens, i);
					i += expression.size - 1;
					expression = InfixToPostfix(&expression);
					if (node->if_stmt->test == NULL)
						node->if_stmt->test = ShantingYard(&expression);
					break;
				}
			}

			if (strcmp(next->lexeme, "=") == 0) {
				Node* ass = CreateNode(ASSIGNMENT);
				Assign* assign = ass->assign_stmt;
				Name* var = (CreateNode(VARIABLE))->variable;
				var->id = token->lexeme;
				var->ctx = STORE;
				assign->target = var;
				assign->value = NULL;
				ArrayListPush(context, ass);
				break;
			}

			if (node != NULL && node->type == ASSIGNMENT && node->assign_stmt->target->ctx == STORE) {
				Assign* assign = node->assign_stmt;

				if (Any(BIN_OPERATORS, ARRAYSIZE(BIN_OPERATORS), next->lexeme, StrEQ)) {
					Tokens expression = CollectExpression(tokens, i);
					i += expression.size - 1;
					expression = InfixToPostfix(&expression);
					assign->value = ShantingYard(&expression);
					break;
				}

				Node* var = CreateNode(VARIABLE);
				var->variable->ctx = LOAD;
				var->variable->id = token->lexeme;

				if (assign->value != NULL && assign->value->type == UNARY_OPERATION)
					assign->value->unOp->operand = var;
				else
					assign->value = var;
				break;
			}

			if (node != NULL && next != NULL &&
				node->type == IMPORT && (strcmp(next->lexeme, ",") == 0 || next->type == NEWLINE)) {
				ImportStmt* importStmt = node->import_stm;
				ArrayListPush(importStmt->modules, Slice(token->lexeme, 0, strlen(token->lexeme)));
			}
		} break;
		case INTEGER: {
			if (node == NULL) {
				node = ArrayListGet(program, program->size - 1);
				if (node->type == IF) {
					if (Any(BIN_OPERATORS, ARRAYSIZE(BIN_OPERATORS), next->lexeme, StrEQ)) {
						Tokens expression = CollectExpression(tokens, i);
						i += expression.size - 1;
						expression = InfixToPostfix(&expression);
						if (node->if_stmt->test == NULL)
							node->if_stmt->test = ShantingYard(&expression);
					}
				}
				break;
			}

			if (node->type == ASSIGNMENT && node->assign_stmt->target->ctx == STORE) {
				Assign* assign = node->assign_stmt;

				if (Any(BIN_OPERATORS, ARRAYSIZE(BIN_OPERATORS), next->lexeme, StrEQ)) {
					Tokens expression = CollectExpression(tokens, i);
					i += expression.size - 1;
					expression = InfixToPostfix(&expression);
					assign->value = ShantingYard(&expression);
					break;
				}
			}

			if (node != NULL && node->type == ASSIGNMENT) {
				Node* assignV = node->assign_stmt->value;
				Node* literal = CreateNode(LITERAL);
				literal->literal = CreateLiteral(token->lexeme);
				if (assignV != NULL && assignV->type == UNARY_OPERATION) assignV->unOp->operand = literal;
				else node->assign_stmt->value = literal;
			}
		} break;
		case STRING: {
			if (node != NULL && node->type == ASSIGNMENT) {
				Node* literal = CreateNode(LITERAL);
				literal->literal = CreateLiteral(token->lexeme);
				node->assign_stmt->value = literal;
			}
		} break;
		case DELIMITER: {
			if (strcmp(token->lexeme, "[") == 0) {
				Tokens elements = Token_CreateArrayList(20);
				for (size_t j = i; strcmp(token->lexeme, "]") != 0; ++j) {
					ArrayListPush(&elements, token);
					token = ArrayListGet(tokens, j);
					i = j - 1;
				}
				if (node != NULL && node->type == ASSIGNMENT) {
					node->assign_stmt->value = CreateListNode(&elements);
				}
			}
		} break;
		case DEDENT: // TODO: There should be a last context to keep track deep nest. Context stack perhaps
			context = ArrayListPop(&stack);
			break;
		default:
			break;
		}
	}
	puts("");
	printf("Abstract Syntax Tree = [\n");
	ArrayListForEach(program, PrintNode);
	printf("]\n");
}

Node_LinkedList ParseStatements(Tokens* tokens) {

}

// WARNING: Advances tokens index
Node* ParseStatement(Tokens* tokens) {
	Node* node = NULL;
	for (i; i < tokens->size; ++i) {
		Token* token = Token_Get(tokens, i);
		Token* next = Token_Get(tokens, i + 1);
		if (token->type == IDENTIFIER) {
			if (strcmp(next->lexeme, "=") == 0) {
				++i;
				node = CreateNode(ASSIGNMENT);
				Assign* assign = node->assign_stmt;
				Name* var = (CreateNode(VARIABLE))->variable;
				var->id = token->lexeme;
				var->ctx = STORE;
				assign->target = var;
				assign->value = ParseExpression(tokens);
				return node;
			}
		}

		if (strcmp(token->lexeme, "if") == 0) {
			node = CreateNode(IF);
			node->if_stmt->test = ParseExpression(tokens);
			node->if_stmt->body = ParseStatement(tokens);
			return node;
		}
	}

	return node;
}

// WARNING: Advances tokens index
Node* ParseExpression(Tokens* tokens) {
	Tokens expression = CollectExpression(tokens, i + 1);
	i += expression.size - 1;
	expression = InfixToPostfix(&expression);
	return ShantingYard(&expression);
}

ImportStmt* CreateImportStmt()
{
	ImportStmt* importStmt = AllocateContext(sizeof(ImportStmt));
	if (importStmt == NULL)
	{
		fprintf(stderr, "ERROR: Could not allocate memory for import statement\n");
		return;
	}

	importStmt->modules = AllocateArrayList(10);
	return importStmt;
}

IfStmt* CreateIfStmt()
{
	IfStmt* if_expr = AllocateContext(sizeof(IfStmt));
	if (if_expr == NULL)
	{
		fprintf(stderr, "ERROR: Could not allocate memory for import if statement\n");
		return NULL;
	}

	if_expr->test = AllocateContext(sizeof(Node));
	if (if_expr->test == NULL)
	{
		fprintf(stderr, "ERROR: Could not allocate memory for test expression for if statement\n");
		return NULL;
	}

	if_expr->test = NULL;
	if_expr->body = AllocateArrayList(10);
	if_expr->orelse = AllocateArrayList(10);
	return if_expr;
}

Tokens CollectExpression(Tokens* tokens, size_t from)
{
	Token* token = ArrayListGet(tokens, from);
	Tokens expression = Token_CreateArrayList(20);
	TokenType blacklist[] = { INDENT, DEDENT, NEWLINE, ENDMARKER };
	size_t size = ARRAYSIZE(blacklist);
	for (size_t j = from; (!BlacklistTokens(token->type, blacklist, ARRAYSIZE(blacklist)) && strcmp(token->lexeme, ":") != 0); ++j) {
		ArrayListPush(&expression, token);
		token = ArrayListGet(tokens, j);
	}
	return expression;
}

bool BlacklistTokens(TokenType type, TokenType blacklist[], size_t size) {
	for (size_t i = 0; i < size; i++) {
		if (blacklist[i] == type) {
			return true;
		}
	}
	return false;
}

Node* CreateListNode(Tokens* elements)
{
	Node* node = CreateNode(LIST);
	node->list->elts = CreateArrayList(20);
	for (size_t i = 0; i < elements->size; i++)
	{
		Token* token = ArrayListGet(elements, i);
		Node* el = NULL;
		switch (token->type)
		{
		case INTEGER:
		case STRING:
		case FLOAT: {
			el = CreateNode(LITERAL);
			el->literal = CreateLiteral(token->lexeme);
		} break;
		case DELIMITER:
			break;
		default:
			fprintf(stderr, "WARNING: Not implemented!");
			break;
		}
		if (el != NULL) ArrayListPush(&node->list->elts, el);
	}
	return node;
}

Literal* CreateLiteral(char* value)
{
	Literal* literal = AllocateContext(sizeof(Literal));
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
	Assign* assignStmt = AllocateContext(sizeof(Assign));
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
	Name* variable = AllocateContext(sizeof(Name));
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
	UnaryOperation* operation = AllocateContext(sizeof(UnaryOperation));
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
	Node* node = AllocateContext(sizeof(Node));
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
		node->import_stm = CreateImportStmt();
		return node;
	case ASSIGNMENT:
		node->assign_stmt = CreateAssignStmt();
		return node;
	case VARIABLE:
		node->variable = CreateNameExpr();
		return node;
	case UNARY_OPERATION:
	case LITERAL:
	case BINARY_OPERATION:
		return node;
	case LIST:
		node->list = AllocateContext(sizeof(List));
		return node;
	case IF:
		node->if_stmt = CreateIfStmt();
		return node;
	default:
		fprintf(stderr, "WARNING: Unrecognized node type %s\n", NodeTypeToString(type));
		return node;
	}
}

Tokens InfixToPostfix(Tokens* tokens)
{
	Tokens stack = Token_CreateArrayList(10);
	Tokens postfix = Token_CreateArrayList(10);

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
			while (stack.size > 0 && Precedence(last->lexeme) >= Precedence(token->lexeme) && strcmp(last->lexeme, "(") != 0)
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
	Node_LinkedList stack = Node_CreateLinkedList();

	for (size_t i = 0; i < tokens->size; i++)
	{
		// TODO: Parse unary operation
		// TODO: Parse array literals
		Token* token = Token_Get(tokens, i);
		switch (token->type) {
		case IDENTIFIER: {
			Node* var = CreateNode(VARIABLE);
			var->variable->ctx = LOAD;
			var->variable->id = token->lexeme;
			Node_AddFirst(&stack, var);
		} break;
		case INTEGER:
		case FLOAT: {
			Node* literal = CreateNode(LITERAL);
			literal->literal = CreateLiteral(token->lexeme);
			Node_AddFirst(&stack, literal);
		} break;
		case OPERATOR: {
			Node* right = Node_Pop(&stack);
			Node* left = Node_Pop(&stack);

			if (right != NULL && left != NULL) {
				Node* bin = CreateBinOp(token, left, right);
				Node_AddFirst(&stack, bin);
				continue;
			}

			if (Any(UNARY_OPERATORS, ARRAYSIZE(UNARY_OPERATORS), token->lexeme, StrEQ) && right != NULL) {
				Node* node = CreateNode(UNARY_OPERATION);
				node->unOp = CreateUnaryOp(token->lexeme);
				node->unOp->operand = right;
				Node_AddFirst(&stack, node);
			}
		} break;
		case INDENT:
		case DEDENT:
		case DELIMITER:
			break;
		default: {
		} break;
		}
	}
	Node* root = Node_Pop(&stack);
	return root;
}

Node* CreateBinOp(Token* token, Node* left, Node* right)
{
	Node* node = CreateNode(BINARY_OPERATION);
	node->bin_op = AllocateContext(sizeof(BinaryOperation));
	if (node->bin_op == NULL) {
		fprintf(stderr, "ERROR: Could not allocate memory for binary operation\n");
		return NULL;
	}
	node->bin_op->left = left;
	node->bin_op->right = right;
	node->bin_op->operator = token->lexeme;
	return node;
}

void PrintNode(Node* node)
{
	const SCALE_FACTOR = 2;
	if (node == NULL) return;
	TraverseTree(node, node->depth);
	char* type = NodeTypeToString(node->type);
	char* spaces = Repeat(" ", node->depth * SCALE_FACTOR);
	switch (node->type)
	{
	case IMPORT:
		PrintImportStmt(node);
		printf(",\n");
		break;
	case UNARY_OPERATION:
		printf("{ \n%s\033[0;36moperator\033[0m: ", spaces);
		Print(node->unOp->operator);
		printf("\n%s\033[0;36moperand\033[0m: ", spaces);
		PrintNode(node->unOp->operand);
		printf(", \n%s\033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m \n%s}",
			spaces, type, Slice(spaces, 0, SCALE_FACTOR * (node->depth - (node->depth > 1))));
		break;
	case BINARY_OPERATION:
		printf("{ \n%s\033[0;36moperator\033[0m: ", spaces);
		Print(node->bin_op->operator);
		printf("\n%s\033[0;36mleft\033[0m: ", spaces);
		PrintNode(node->bin_op->left);
		printf(",\n%s\033[0;36mright\033[0m: ", spaces);
		PrintNode(node->bin_op->right);
		printf(", \n%s\033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36mdepth\033[0m: \033[0;31m%zu\033[0m \n%s}",
			spaces, type, node->depth, Slice(spaces, 0, 2 * SCALE_FACTOR * (node->depth - (node->depth > 1))));
		break;
	case ASSIGNMENT: {
		char* br_spaces = Slice(spaces, 0, SCALE_FACTOR * (node->depth - (node->depth > 1)));
		printf("%s{ \n%s\033[0;36mtarget\033[0m: ", node->depth == 1 ? "" : br_spaces, spaces);
		PrintVar(node->assign_stmt->target, node->depth);
		printf(", \n%s\033[0;36mexpression\033[0m: ", spaces);
		PrintNode(node->assign_stmt->value, node->depth);
		printf(", \n%s\033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36mdepth\033[0m: \033[0;31m%zu\033[0m\n %s}", spaces, type, node->depth, node->depth == 1 ? "" : br_spaces);
		printf(",\n");
	} break;
	case VARIABLE:
		PrintVar(node->variable, node->depth);
		break;
	case LITERAL:
		printf("{ \033[0;36mvalue\033[0m: \033[0;33m\"%s\"\033[0m, \033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36mdepth\033[0m: \033[0;31m%zu\033[0m }", node->literal->value, NodeTypeToString(node->type), node->depth);
		break;
	case LIST:
		PrintList(node, spaces);
		break;
	case IF:
		printf("{ \n%s\033[0;36mtest\033[0m: ", spaces);
		PrintNode(node->if_stmt->test);
		printf(", \n%s\033[0;36mbody\033[0m: [\n", spaces);
		ArrayListForEach(node->if_stmt->body, PrintNode);
		printf("%s]\n", spaces);
		printf(",\n%s\033[0;36melse\033[0m: [\n", spaces);
		ArrayListForEach(node->if_stmt->orelse, PrintNode);
		printf("%s]\n", spaces);
		printf(", \n%s\033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36mdepth\033[0m: \033[0;31m%zu\033[0m \n%s}",
			spaces, type, node->depth, Slice(spaces, 0, 1 * (node->depth - (node->depth > 1))));
		break;
	default: {
		fprintf(stderr, "WARNING: Not implemented for type: \"%s\"\n", type);
	} break;
	}
}

void PrintList(Node* node, char* spaces) {
	const char* type = NodeTypeToString(node->type);
	printf("{ \n%s\033[0;36melements\033[0m: [ \n", spaces);
	printf("    %s", spaces);
	for (size_t i = 0; i < node->list->elts.size; i++)
	{
		Node* el = ArrayListGet(&node->list->elts, i);
		PrintNode(el);
		printf(", ");
	}
	printf("\n%s]", spaces);
	printf(", \n%s\033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36m\033[0;36mdepth\033[0m\033[0m: \033[0;31m%zu\033[0m \n%s}", spaces, type, node->depth, spaces);
}

void TraverseTree(Node* node, size_t depth)
{
	if (node == NULL) return;

	node->depth = depth;
	switch (node->type)
	{
	case UNARY_OPERATION:
		TraverseTree(node->unOp->operand, depth);
		break;
	case BINARY_OPERATION:
		TraverseTree(node->bin_op->left, depth + 1);
		TraverseTree(node->bin_op->right, depth + 1);
		break;
	case ASSIGNMENT:
		TraverseTree(node->assign_stmt->value, node->depth + 1);
		break;
	case IF:
		TraverseTree(node->if_stmt->test, depth + 1);
		for (size_t i = 0; i < node->if_stmt->body->size; i++)
		{
			Node* el = ArrayListGet(node->if_stmt->body, i);
			TraverseTree(el, node->depth + 1);
		}
		for (size_t i = 0; i < node->if_stmt->orelse->size; i++)
		{
			Node* el = ArrayListGet(node->if_stmt->orelse, i);
			TraverseTree(el, node->depth + 1);
		}
		break;
	case LIST:
		for (size_t i = 0; i < node->list->elts.size; i++)
		{
			Node* el = ArrayListGet(&node->list->elts, i);
			TraverseTree(el, node->depth);
		}
		break;
	default: {
	} break;
	}
}

void PrintVar(Name* variable, size_t depth)
{
	printf("{ \033[0;36midentifer\033[0m: \033[0;33m%s\033[0m, \033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36m\033[0;36mctx\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36mdepth\033[0m: \033[0;31m%zu\033[0m }", variable->id, NodeTypeToString(VARIABLE), CtxToString(variable), depth);
}

void PrintImportStmt(Node* stmt)
{
	const char* type = NodeTypeToString(IMPORT);
	printf("{ \033[0;36mmodules\033[0m: [ ");
	ArrayListForEach(stmt->import_stm->modules, Print);
	printf("]");
	printf(", \033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36m\033[0;36mdepth\033[0m\033[0m: \033[0;31m%zu\033[0m }", type, stmt->depth);
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
	case LIST: return "LIST";
	case IF: return "IF";
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

size_t Precedence(const char* op) {
	if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) {
		return 1;
	}
	else if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
		return 2;
	}
	else if (strcmp(op, "^") == 0) {
		return 3;
	}
	else if (strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
		strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0 ||
		strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
		return 0;
	}
	else return -1;

}