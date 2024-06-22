#include "parser.h"

// TODO: Implement semantic analysis

const char *UNARY_OPERATORS[] = {"+", "-", "~", "not"};

Node_LinkedList ParseStatements(Lexer *lexer) {
    Node_LinkedList stmts = Node_CreateLinkedList();
    for (; lexer->token_idx < lexer->tokens.size; ++lexer->token_idx) {
        Node *node = ParseStatement(lexer);
        if (node == NULL) {
            continue;
        }
        if (node->type == END_BLOCK) break;
        TraverseTree(node, node->depth);
        Node_AddLast(&stmts, node);
    }
    return stmts;
}

Node *ParseStatement(Lexer *lexer) {
    Node *node = NULL;
    Token const *token = Token_Get(&lexer->tokens, lexer->token_idx);
    Token const *next = Token_Get(&lexer->tokens, lexer->token_idx + 1);

    switch (token->type) {
        case IDENTIFIER: {
            if (strcmp(next->lexeme, "=") == 0) {
                ++lexer->token_idx;
                node = CreateNode(ASSIGNMENT);
                Assign *assign = node->assign_stmt;
                Name *var = (CreateNode(VARIABLE))->variable;
                var->id = token->lexeme;
                var->ctx = STORE;
                assign->target = var;
                assign->value = ParseExpression(lexer);
            }
        }
            break;
        case KEYWORD: {
            if (strcmp(token->lexeme, "if") == 0) {
                node = ParseIfStatement(lexer);
            }

            if (strcmp(token->lexeme, "import") == 0) {
                node = CreateNode(IMPORT);
                node->import_stmt->modules = CollectUntil(lexer, NEWLINE);
            }

            if (strcmp(token->lexeme, "while") == 0) {
                node = ParseWhileStatement(lexer);
            }

            if (strcmp(token->lexeme, "for") == 0) {
                node = ParseForStatement(lexer);
            }
        }
            break;
        case NEWLINE:
            if (next->ident != token->ident) return CreateNode(END_BLOCK);
            break;
        default:
            break;
    }
    return node;
}

Node *ParseForStatement(Lexer *lexer) {
    Node *node = CreateNode(FOR);
    Token const *token = Token_Get(&lexer->tokens, ++lexer->token_idx);
    node->for_stmt->target.variable = CreateNameExpr();
    node->for_stmt->target.variable->id = token->lexeme;
    node->for_stmt->target.variable->ctx = STORE;
    token = Token_Get(&lexer->tokens, ++lexer->token_idx);
    if (token->type != KEYWORD && strcmp(token->lexeme, "in") != 0) {
        fprintf(stderr, "Syntax Error: Keyword \"in\" expect but found \"%s\" instead. line: %zu, col: %zu",
                token->lexeme, token->line, token->col);
        return NULL;
    }
    node->for_stmt->iter = ParseExpression(lexer);
    lexer->token_idx += 3;
    node->for_stmt->body = ParseStatements(lexer);
    return node;
}

Node *ParseWhileStatement(Lexer *lexer) {
    Node *node = CreateNode(WHILE);
    node->while_stmt->test = ParseExpression(lexer);
    lexer->token_idx += 3;
    node->while_stmt->body = ParseStatements(lexer);
    // TODO: Parse else clause
    return node;
}

Node *ParseIfStatement(Lexer *lexer) {
    Node *node = CreateNode(IF);
    IfStmt *ifStmt = node->if_stmt;

    Token const *token = Token_Get(&lexer->tokens, lexer->token_idx);
    if (strcmp(token->lexeme, "if") != 0) {
        // TODO: Error handling
        return NULL;
    }

    ifStmt->test = ParseExpression(lexer);
    lexer->token_idx += 3;
    ifStmt->body = ParseStatements(lexer);
    Node *last = node;
    // Parse the 'elif' and 'else' blocks
    for (; lexer->token_idx < lexer->tokens.size; lexer->token_idx++) {
        token = Token_Get(&lexer->tokens, lexer->token_idx);
        if (strcmp(token->lexeme, "elif") == 0) {
            Node *elifNode = CreateNode(IF);
            IfStmt *elifStmt = elifNode->if_stmt;
            elifStmt->test = ParseExpression(lexer);
            lexer->token_idx += 3;
            elifStmt->body = ParseStatements(lexer);
            Node_AddLast(&ifStmt->orelse, elifNode);
            last = elifNode;
        } else if (strcmp(token->lexeme, "else") == 0) {
            lexer->token_idx += 3;
            last->if_stmt->orelse = ParseStatements(lexer);
            break;
        }
    }

    return node;
}

Token_ArrayList CollectUntil(Lexer *lexer, TokenType type) {
    Token_ArrayList result = Token_CreateArrayList(10);
    ++lexer->token_idx;
    Token *token = Token_Get(&lexer->tokens, lexer->token_idx);
    lexer->token_idx++;
    for (; lexer->token_idx < lexer->tokens.size && token->type != type; ++lexer->token_idx) {
        if (strcmp(token->lexeme, ",") != 0) Token_Push(&result, token);
        token = Token_Get(&lexer->tokens, lexer->token_idx);
    }

    return result;
}

Node *ParseExpression(Lexer *lexer) {
    Tokens expression = CollectExpression(&lexer->tokens, lexer->token_idx + 1);
    lexer->token_idx += expression.size;
    expression = InfixToPostfix(&expression);
    return ShuntingYard(&expression);
}

ImportStmt *CreateImportStmt() {
    ImportStmt *importStmt = AllocateContext(sizeof(ImportStmt));
    if (importStmt == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for import statement\n");
        return NULL;
    }

    return importStmt;
}

IfStmt *CreateIfStmt() {
    IfStmt *if_expr = AllocateContext(sizeof(IfStmt));
    if (if_expr == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for import if statement\n");
        return NULL;
    }

    if_expr->test = AllocateContext(sizeof(Node));
    if (if_expr->test == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for test expression for if statement\n");
        return NULL;
    }

    if_expr->test = NULL;
    if_expr->body = Node_CreateLinkedList();
    if_expr->orelse = Node_CreateLinkedList();
    return if_expr;
}

Tokens CollectExpression(Tokens const *tokens, size_t from) {
    Token *token = Token_Get(tokens, from);
    Tokens expression = Token_CreateArrayList(20);
    TokenType blacklist[] = {NEWLINE, ENDMARKER};
    size_t scope = token->ident;
    for (size_t j = from + 1; (!BlacklistTokens(token->type, blacklist, ARRAYSIZE(blacklist)) &&
                               strcmp(token->lexeme, ":") != 0); ++j) {
        Token_Push(&expression, token);
        token = Token_Get(tokens, j);
        if (token->ident != scope) break;
    }
    return expression;
}

bool BlacklistTokens(TokenType type, const TokenType blacklist[], size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (blacklist[i] == type) {
            return true;
        }
    }
    return false;
}

Literal *CreateLiteral(char *value) {
    Literal *literal = AllocateContext(sizeof(Literal));
    if (literal == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for import statement\n");
        return NULL;
    }
    if (IsFloat(value) && strchr(value, '.') != NULL) literal->type = FLOAT;
    else if (IsInteger(value)) literal->type = INT;
    else literal->type = TEXT;
    literal->value = value;
    return literal;
}

Assign *CreateAssignStmt() {
    Assign *assignStmt = AllocateContext(sizeof(Assign));
    if (assignStmt == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for assignment statement\n");
        return NULL;
    }

    assignStmt->target = NULL;
    return assignStmt;
}

Name *CreateNameExpr() {
    Name *variable = AllocateContext(sizeof(Name));
    if (variable == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for variable expression\n");
        return NULL;
    }

    variable->id = NULL;
    return variable;
}

UnaryOperation *CreateUnaryOp(char *op) {
    UnaryOperation *operation = AllocateContext(sizeof(UnaryOperation));
    if (operation == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for unary operation expression\n");
        return NULL;
    }

    operation->operator = op;
    return operation;
}

Node *CreateNode(NodeType type) {
    Node *node = AllocateContext(sizeof(Node));
    if (node == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for AST node\n");
        return NULL;
    }

    node->type = type;
    node->depth = 1;
    switch (type) {
        case IMPORT:
            node->import_stmt = CreateImportStmt();
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
            node->list->elts = Node_CreateLinkedList();
            return node;
        case IF:
            node->if_stmt = CreateIfStmt();
            return node;
        case WHILE:
            node->while_stmt = CreateWhileStmt();
            return node;
        case FOR:
            node->for_stmt = CreateForStmt();
            return node;
        case END_BLOCK:
            return node; // This node represents end of statement list. It will not actually be consumed by succeeding components
        default:
            fprintf(stderr, "WARNING: Unrecognized node type %s\n", NodeTypeToString(type));
            return node;
    }
}

ForStmt *CreateForStmt() {
    ForStmt *for_stmt = AllocateContext(sizeof(ForStmt));
    if (for_stmt == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for \"for\" statement\n");
        return NULL;
    }

    for_stmt->body = Node_CreateLinkedList();
    for_stmt->orelse = Node_CreateLinkedList();
    return for_stmt;
}

WhileStmt *CreateWhileStmt() {
    WhileStmt *while_stmt = AllocateContext(sizeof(WhileStmt));
    if (while_stmt == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for import if statement\n");
        return NULL;
    }

    while_stmt->test = AllocateContext(sizeof(Node));
    if (while_stmt->test == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for test expression for if statement\n");
        return NULL;
    }

    while_stmt->test = NULL;
    while_stmt->body = Node_CreateLinkedList();
    while_stmt->orelse = Node_CreateLinkedList();
    return while_stmt;
}

Tokens InfixToPostfix(Tokens const *tokens) {
    Tokens stack = Token_CreateArrayList(10);
    Tokens postfix = Token_CreateArrayList(10);

    for (size_t i = 0; i < tokens->size; i++) {
        Token *token = Token_Get(tokens, i);
        if (token->type == IDENTIFIER || token->type == NUMERIC) {
            Token_Push(&postfix, token);
        } else if (strcmp(token->lexeme, "(") == 0) {
            Token_Push(&stack, token);
        } else if (strcmp(token->lexeme, ")") == 0) {
            Token const *last = Token_Get(&stack, stack.size - 1);
            while (stack.size > 0 && strcmp(last->lexeme, "(") != 0) {
                Token_Push(&postfix, Token_Pop(&stack));
            }
            if (stack.size > 0 && strcmp(last->lexeme, "(") == 0) {
                Token_Pop(&stack); // Pop the open bracket
            }
        } else {
            Token *last = Token_Get(&stack, stack.size - 1);
            while (stack.size > 0 && Precedence(last->lexeme) >= Precedence(token->lexeme) &&
                   strcmp(last->lexeme, "(") != 0) {
                Token_Push(&postfix, Token_Pop(&stack));
                last = stack.size > 0 ? Token_Get(&stack, stack.size - 1) : last;
            }
            Token_Push(&stack, token);
        }
    }

    while (stack.size > 0) {
        Token_Push(&postfix, Token_Pop(&stack));
    }
    return postfix;
}

Node *ShuntingYard(Tokens const *tokens) {
    Node_LinkedList stack = Node_CreateLinkedList();

    for (size_t i = 0; i < tokens->size; i++) {
        Token *token = Token_Get(tokens, i);
        switch (token->type) {
            case IDENTIFIER: {
                Node *var = CreateNode(VARIABLE);
                var->variable->ctx = LOAD;
                var->variable->id = token->lexeme;
                Node_AddFirst(&stack, var);
            }
                break;
            case TEXT:
            case NUMERIC: {
                Node *literal = CreateNode(LITERAL);
                literal->literal = CreateLiteral(token->lexeme);
                Node_AddFirst(&stack, literal);
            }
                break;
            case OPERATOR: {
                Node *right = Node_Pop(&stack);
                Node *left = Node_Pop(&stack);

                if (right != NULL && left != NULL) {
                    Node *bin = CreateBinOp(token, left, right);
                    Node_AddFirst(&stack, bin);
                    continue;
                }

                if (Any((void **) UNARY_OPERATORS, ARRAYSIZE(UNARY_OPERATORS), token->lexeme, (CompareFn) StrEQ) &&
                    right != NULL) {
                    Node *node = CreateNode(UNARY_OPERATION);
                    node->unOp = CreateUnaryOp(token->lexeme);
                    node->unOp->operand = right;
                    Node_AddFirst(&stack, node);
                }
            }
                break;
            case RSQB: {
                Node *value = NULL;
                Node *node = CreateNode(LIST);
                while ((value = Node_Pop(&stack)) && value->type != LSQB) {
                    Node_AddFirst(&node->list->elts, value);
                }
                Node_AddFirst(&stack, node);
            }
                break;
            default:
                break;
        }
    }
    Node *root = Node_Pop(&stack);
    return root;
}

Node *CreateBinOp(Token *token, Node *left, Node *right) {
    Node *node = CreateNode(BINARY_OPERATION);
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

void PrintNode(Node *node) {
    int const SCALE_FACTOR = 2;
    if (node == NULL) return;
    const char *type = NodeTypeToString(node->type);
    char *spaces = Repeat(" ", node->depth * SCALE_FACTOR);
    switch (node->type) {
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
            char *br_spaces = Slice(spaces, 0, SCALE_FACTOR * (node->depth - (node->depth > 1)));
            printf("%s{ \n%s\033[0;36mtarget\033[0m: ", node->depth == 1 ? "" : br_spaces, spaces);
            PrintVar(node->assign_stmt->target, node->depth);
            printf(", \n%s\033[0;36mexpression\033[0m: ", spaces);
            PrintNode(node->assign_stmt->value);
            printf(", \n%s\033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36mdepth\033[0m: \033[0;31m%zu\033[0m\n %s}",
                   spaces, type, node->depth, node->depth == 1 ? "" : br_spaces);
            printf(",\n");
        }
            break;
        case VARIABLE:
            PrintVar(node->variable, node->depth);
            break;
        case LITERAL:
            printf("{ \033[0;36mvalue\033[0m: \033[0;33m\"%s\"\033[0m, \033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36mdepth\033[0m: \033[0;31m%zu\033[0m }",
                   node->literal->value, NodeTypeToString(node->type), node->depth);
            break;
        case LIST:
            PrintList(node, spaces);
            break;
        case IF:
            printf("{ \n%s\033[0;36mtest\033[0m: ", spaces);
            PrintNode(node->if_stmt->test);
            printf(", \n%s\033[0;36mbody\033[0m: [\n", spaces);
            Node_ForEach(&node->if_stmt->body, PrintNode);
            printf("%s]", spaces);
            printf(",\n%s\033[0;36melse\033[0m: [\n", spaces);
            Node_ForEach(&node->if_stmt->orelse, PrintNode);
            printf("%s]", spaces);
            printf(", \n%s\033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36mdepth\033[0m: \033[0;31m%zu\033[0m \n%s}",
                   spaces, type, node->depth, Slice(spaces, 0, 1 * (node->depth - (node->depth > 1))));
            puts("");
            break;
        case WHILE:
            printf("{ \n%s\033[0;36mtest\033[0m: ", spaces);
            PrintNode(node->while_stmt->test);
            printf(", \n%s\033[0;36mbody\033[0m: [\n", spaces);
            Node_ForEach(&node->while_stmt->body, PrintNode);
            printf("%s]", spaces);
            printf(",\n%s\033[0;36melse\033[0m: [\n", spaces);
            Node_ForEach(&node->while_stmt->orelse, PrintNode);
            printf("%s]", spaces);
            printf(", \n%s\033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36mdepth\033[0m: \033[0;31m%zu\033[0m \n%s}",
                   spaces, type, node->depth, Slice(spaces, 0, 1 * (node->depth - (node->depth > 1))));
            puts("");
            break;
        case FOR:
            printf("{ \n%s\033[0;36mtarget\033[0m: ", spaces);
            PrintVar(node->for_stmt->target.variable, node->depth);
            printf(", \n%s\033[0;36miter\033[0m: ", spaces);
            PrintNode(node->for_stmt->iter);
            printf(", \n%s\033[0;36mbody\033[0m: [\n", spaces);
            Node_ForEach(&node->for_stmt->body, PrintNode);
            Node_ForEach(&node->for_stmt->orelse, PrintNode);
            printf("%s]", spaces);
            printf(", \n%s\033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36mdepth\033[0m: \033[0;31m%zu\033[0m \n%s}",
                   spaces, type, node->depth, Slice(spaces, 0, 1 * (node->depth - (node->depth > 1))));
            puts("");
            break;
        default: {
            fprintf(stderr, "WARNING: Not implemented for type: \"%s\"\n", type);
        }
            break;
    }
}

void PrintList(Node const *node, char *spaces) {
    const char *type = NodeTypeToString(node->type);
    printf("{ \n%s\033[0;36melements\033[0m: [ \n", spaces);
    printf("    %s", spaces);
    Node_Node *current = node->list->elts.head;
    while (current != NULL) {
        PrintNode(current->data);
        printf(", ");
        current = current->next;
    }
    printf("\n%s]", spaces);
    printf(", \n%s\033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36m\033[0;36mdepth\033[0m\033[0m: \033[0;31m%zu\033[0m \n%s}",
           spaces, type, node->depth, spaces);
}

void TraverseTree(Node *node, size_t depth) {
    if (node == NULL) return;

    node->depth = depth;
    switch (node->type) {
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
            Node_Node *current = node->if_stmt->body.head;
            while (current != NULL) {
                TraverseTree(current->data, node->depth + 1);
                current = current->next;
            }
            current = node->if_stmt->orelse.head;
            while (current != NULL) {
                TraverseTree(current->data, node->depth + 1);
                current = current->next;
            }
            break;
        case LIST:
            current = node->list->elts.head;
            while (current != NULL) {
                TraverseTree(current->data, node->depth + 1);
                current = current->next;
            }
            break;
        case WHILE:
            TraverseTree(node->while_stmt->test, depth + 1);
            current = node->while_stmt->body.head;
            while (current != NULL) {
                TraverseTree(current->data, node->depth + 1);
                current = current->next;
            }
            break;
        default:
            break;
    }
}

void PrintVar(Name *variable, size_t depth) {
    printf("{ \033[0;36midentifer\033[0m: \033[0;33m%s\033[0m, \033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36m\033[0;36mctx\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36mdepth\033[0m: \033[0;31m%zu\033[0m }",
           variable->id, NodeTypeToString(VARIABLE), CtxToString(variable), depth);
}

void PrintImportStmt(Node const *stmt) {
    const char *type = NodeTypeToString(IMPORT);
    printf("{ \033[0;36mmodules\033[0m: [ \n");
    Token_ForEach(&stmt->import_stmt->modules, PrintToken);
    printf("]");
    printf(", \033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36m\033[0;36mdepth\033[0m\033[0m: \033[0;31m%zu\033[0m }",
           type, stmt->depth);
}

const char *NodeTypeToString(NodeType type) {
    switch (type) {
        case IMPORT:
            return "IMPORT";
        case PROGRAM:
            return "PROGRAM";
        case VARIABLE:
            return "VARIABLE";
        case ASSIGNMENT:
            return "ASSIGNMENT";
        case LITERAL:
            return "LITERAL";
        case UNARY_OPERATION:
            return "UNARY OPERATION";
        case BINARY_OPERATION:
            return "BINARY OPERATION";
        case LIST:
            return "LIST";
        case IF:
            return "IF";
        case WHILE:
            return "WHILE";
        case FOR:
            return "FOR";
        default:
            return "UNKNOWN";
    }
}

const char *CtxToString(Name const *var) {
    switch (var->ctx) {
        case LOAD:
            return "LOAD";
        case STORE:
            return "STORE";
        case DEL:
            return "DEL";
        default:
            return "UNKNOWN";
    }
}

size_t Precedence(const char *op) {
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) {
        return 1;
    } else if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
        return 2;
    } else if (strcmp(op, "^") == 0) {
        return 3;
    } else if (strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
               strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0 ||
               strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
        return 0;
    } else return -1;

}
