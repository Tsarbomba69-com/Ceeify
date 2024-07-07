#include "parser.h"

const char *UNARY_OPERATORS[] = {"+", "-", "~", "not"};

const char *COMPARISON_OPERATORS[] = {"==", "!=", ">", "<", ">=", "<="};

Node_LinkedList ParseStatements(Parser *parser) {
    Node_LinkedList stmts = Node_CreateLinkedList();
    for (; parser->lexer.token_idx < parser->lexer.tokens.size; ++parser->lexer.token_idx) {
        Node *node = ParseStatement(parser);
        if (node == NULL) {
            continue;
        }
        if (node->type == END_BLOCK) break;
        TraverseTree(node, node->depth);
        Node_AddLast(&stmts, node);
    }
    return stmts;
}

Node *ParseStatement(Parser *parser) {
    Node *node = NULL;
    Token const *token = Token_Get(&parser->lexer.tokens, parser->lexer.token_idx);
    Token const *next = Token_Get(&parser->lexer.tokens, parser->lexer.token_idx + 1);

    switch (token->type) {
        case IDENTIFIER: {
            if (strcmp(next->lexeme, "=") == 0) {
                ++parser->lexer.token_idx;
                node = CreateNode(ASSIGNMENT);
                Assign *assign = node->assignStmt;
                Name *var = (CreateNode(VARIABLE))->variable;
                var->line = token->line;
                var->col = token->col;
                Node *val = ParseExpression(parser);
                var->type = InferType(val, parser->context);
                var->ctx = STORE;
                var->id = token->lexeme;
                assign->target = var;
                assign->value = val;
                Symbol *symbol = StackSymbolsLookup(parser->context, var->id);
                if (symbol == NULL) {
                    symbol = CreateSymbol(var->type, VAR);
                    symbol->line = token->line;
                    symbol->col = token->col;
                    Symbol_Insert(&parser->context->scope, var->id, symbol);
                }
            }
        }
            break;
        case KEYWORD: {
            if (strcmp(token->lexeme, "if") == 0) {
                node = ParseIfStatement(parser);
            }

            if (strcmp(token->lexeme, "import") == 0) {
                node = CreateNode(IMPORT);
                node->importStmt->modules = CollectUntil(parser, NEWLINE);
            }

            if (strcmp(token->lexeme, "while") == 0) {
                node = ParseWhileStatement(parser);
            }

            if (strcmp(token->lexeme, "for") == 0) {
                node = ParseForStatement(parser);
            }
        }
            break;
        case NEWLINE:
            if (next->ident != token->ident) {
                parser->context = parser->context->parent == NULL ? parser->context : parser->context->parent;
                return CreateNode(END_BLOCK);
            }
            break;
        default:
            break;
    }
    return node;
}

Node *ParseForStatement(Parser *parser) {
    Node *node = CreateNode(FOR);
    Token const *token = Token_Get(&parser->lexer.tokens, ++parser->lexer.token_idx);
    size_t line = token->line;
    size_t col = token->col;
    node->forStmt->target.variable = CreateNameExpr();
    node->forStmt->target.variable->id = token->lexeme;
    node->forStmt->target.variable->ctx = STORE;
    token = Token_Get(&parser->lexer.tokens, ++parser->lexer.token_idx);
    if (token->type != KEYWORD && strcmp(token->lexeme, "in") != 0) {
        fprintf(stderr, "Syntax Error: Keyword \"in\" expect but found \"%s\" instead. line: %zu, col: %zu",
                token->lexeme, token->line, token->col);
        return NULL;
    }

    node->forStmt->iter = ParseExpression(parser);
    node->forStmt->target.variable->type = InferType(node->forStmt->iter, parser->context);
    node->forStmt->target.variable->type = node->forStmt->iter->type == LIST_EXPR ?
                                           InferListType(node->forStmt->iter->list->elts, parser->context)
                                                                                  : node->forStmt->target.variable->type;
    Symbol *symbol = StackSymbolsLookup(parser->context, node->forStmt->target.variable->id);
    if (symbol == NULL) {
        symbol = CreateSymbol(node->forStmt->target.variable->type, VAR);
        symbol->line = token->line;
        symbol->col = token->col;
        Symbol *ctx = CreateSymbol(VOID, BLOCK);
        ctx->line = line;
        ctx->col = col;
        ctx->parent = parser->context;
        Symbol_Insert(&ctx->scope, node->forStmt->target.variable->id, symbol);
        Symbol_Insert(&parser->context->scope, TextFormat("%zu:%zu_for", line, col), ctx);
        parser->context = ctx;
    }
    parser->lexer.token_idx += 3;
    node->forStmt->body = ParseStatements(parser);
    return node;
}

Symbol *StackSymbolsLookup(Symbol *namespace, const char *id) {
    for (Symbol *current = namespace; current != NULL; current = current->parent) {
        Symbol *symbol = Symbol_Search(&current->scope, id);
        if (symbol != NULL) {
            return symbol;
        }
    }
    return NULL;
}

Node *ParseWhileStatement(Parser *parser) {
    Node *node = CreateNode(WHILE);
    node->whileStmt->test = ParseExpression(parser);
    Token const *token = Token_Get(&parser->lexer.tokens, parser->lexer.token_idx);
    Symbol *ctx = CreateSymbol(VOID, BLOCK);
    ctx->line = token->line;
    ctx->col = token->col;
    ctx->parent = parser->context;
    Symbol_Insert(&parser->context->scope, TextFormat("%zu:%zu_while", ctx->line, ctx->col), ctx);
    parser->context = ctx;
    parser->lexer.token_idx += 3;
    node->whileStmt->body = ParseStatements(parser);
    // TODO: Parse else clause
    return node;
}

Node *ParseIfStatement(Parser *parser) {
    Node *node = CreateNode(IF);
    IfStmt *ifStmt = node->ifStmt;
    ifStmt->test = ParseExpression(parser);
    Token const *token = Token_Get(&parser->lexer.tokens, parser->lexer.token_idx);
    Symbol *ctx = CreateSymbol(VOID, BLOCK);
    ctx->line = token->line;
    ctx->col = token->col;
    ctx->parent = parser->context;
    Symbol_Insert(&parser->context->scope, TextFormat("%zu:%zu_if", ctx->line, ctx->col), ctx);
    parser->context = ctx;
    parser->lexer.token_idx += 3;
    ifStmt->body = ParseStatements(parser);
    Node *last = node;
    // Parse the 'elif' and 'else' blocks
    for (; parser->lexer.token_idx < parser->lexer.tokens.size; parser->lexer.token_idx++) {
        token = Token_Get(&parser->lexer.tokens, parser->lexer.token_idx);
        if (strcmp(token->lexeme, "elif") == 0) {
            Node *elifNode = CreateNode(IF);
            IfStmt *elifStmt = elifNode->ifStmt;
            elifStmt->test = ParseExpression(parser);
            parser->lexer.token_idx += 3;
            elifStmt->body = ParseStatements(parser);
            Node_AddLast(&ifStmt->orelse, elifNode);
            last = elifNode;
        } else if (strcmp(token->lexeme, "else") == 0) {
            parser->lexer.token_idx += 3;
            last->ifStmt->orelse = ParseStatements(parser);
            break;
        }
    }

    return node;
}

Token_ArrayList CollectUntil(Parser *parser, TokenType type) {
    Token_ArrayList result = Token_CreateArrayList(10);
    ++parser->lexer.token_idx;
    Token *token = Token_Get(&parser->lexer.tokens, parser->lexer.token_idx);
    parser->lexer.token_idx++;
    for (; parser->lexer.token_idx < parser->lexer.tokens.size && token->type != type; ++parser->lexer.token_idx) {
        if (strcmp(token->lexeme, ",") != 0) Token_Push(&result, token);
        token = Token_Get(&parser->lexer.tokens, parser->lexer.token_idx);
    }

    return result;
}

Node *ParseExpression(Parser *parser) {
    Tokens expression = CollectExpression(&parser->lexer.tokens, parser->lexer.token_idx + 1);
    parser->lexer.token_idx += expression.size;
    expression = InfixToPostfix(&expression);
    return ShuntingYard(&expression, parser->context);
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
    else literal->type = STR;
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
            node->importStmt = CreateImportStmt();
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
        case COMPARE:
            return node;
        case LIST_EXPR:
            node->list = AllocateContext(sizeof(List));
            node->list->elts = Node_CreateLinkedList();
            return node;
        case IF:
            node->ifStmt = CreateIfStmt();
            return node;
        case WHILE:
            node->whileStmt = CreateWhileStmt();
            return node;
        case FOR:
            node->forStmt = CreateForStmt();
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

Node *ShuntingYard(Tokens const *tokens, Symbol *namespace) {
    Node_LinkedList stack = Node_CreateLinkedList();

    for (size_t i = 0; i < tokens->size; i++) {
        Token *token = Token_Get(tokens, i);
        switch (token->type) {
            case IDENTIFIER: {
                Symbol const *targetSymbol = StackSymbolsLookup(namespace, token->lexeme);
                if (targetSymbol == NULL) {
                    fprintf(stderr, "NameError: name \"%s\" is not defined.\n line: %zu, col: %zu\n", token->lexeme);
                    exit(EXIT_FAILURE);
                }
                Node *var = CreateNode(VARIABLE);
                var->variable->ctx = LOAD;
                var->variable->id = token->lexeme;
                var->variable->type = targetSymbol->type;
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
                    if (Any((void **) COMPARISON_OPERATORS, ARRAYSIZE(COMPARISON_OPERATORS), token->lexeme,
                            (CompareFn) StrEQ)) {
                        Node *comp = NULL;
                        if (left->type == COMPARE) {
                            Node_AddFirst(&left->compare->comparators, right);
                            Token_Push(&left->compare->ops, token);
                            comp = left;
                        } else {
                            comp = CreateCompare(token, left, right);
                        }
                        Node_AddFirst(&stack, comp);
                        continue;
                    }

                    Node *bin = CreateBinOp(token, left, right);
                    bin->binOp->type = InferType(bin, namespace);
                    Node_AddFirst(&stack, bin);
                    continue;
                }

                if (Any((void **) UNARY_OPERATORS, ARRAYSIZE(UNARY_OPERATORS), token->lexeme, (CompareFn) StrEQ) &&
                    right != NULL) {
                    Node *node = CreateNode(UNARY_OPERATION);
                    node->unOp = CreateUnaryOp(token->lexeme);
                    node->unOp->operand = right;
                    node->unOp->type = InferType(node, namespace);
                    Node_AddFirst(&stack, node);
                }
            }
                break;
            case RSQB: {
                Node *value = NULL;
                Node *node = CreateNode(LIST_EXPR);
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
    node->binOp = AllocateContext(sizeof(BinaryOperation));
    if (node->binOp == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for binary operation\n");
        return NULL;
    }
    node->binOp->left = left;
    node->binOp->right = right;
    node->binOp->operator = token->lexeme;
    return node;
}

Node *CreateCompare(Token *token, Node *left, Node *right) {
    Node *node = CreateNode(COMPARE);
    node->compare = AllocateContext(sizeof(Compare));
    if (node->binOp == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for comparison operation\n");
        return NULL;
    }

    node->compare->left = left;
    node->compare->comparators = Node_CreateLinkedList();
    node->compare->ops = Token_CreateArrayList(5);
    Token_Push(&node->compare->ops, token);
    Node_AddFirst(&node->compare->comparators, right);
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
            Print(node->binOp->operator);
            printf("\n%s\033[0;36mleft\033[0m: ", spaces);
            PrintNode(node->binOp->left);
            printf(",\n%s\033[0;36mright\033[0m: ", spaces);
            PrintNode(node->binOp->right);
            printf(", \n%s\033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36mdepth\033[0m: \033[0;31m%zu\033[0m \n%s}",
                   spaces, type, node->depth, Slice(spaces, 0, 2 * SCALE_FACTOR * (node->depth - (node->depth > 1))));
            break;
        case ASSIGNMENT: {
            char *br_spaces = Slice(spaces, 0, SCALE_FACTOR * (node->depth - (node->depth > 1)));
            printf("%s{ \n%s\033[0;36mtarget\033[0m: ", node->depth == 1 ? "" : br_spaces, spaces);
            PrintVar(node->assignStmt->target, node->depth);
            printf(", \n%s\033[0;36mexpression\033[0m: ", spaces);
            PrintNode(node->assignStmt->value);
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
        case LIST_EXPR:
            PrintList(node, spaces);
            break;
        case IF:
            printf("{ \n%s\033[0;36mtest\033[0m: ", spaces);
            PrintNode(node->ifStmt->test);
            printf(", \n%s\033[0;36mbody\033[0m: [\n", spaces);
            Node_ForEach(&node->ifStmt->body, PrintNode);
            printf("%s]", spaces);
            printf(",\n%s\033[0;36melse\033[0m: [\n", spaces);
            Node_ForEach(&node->ifStmt->orelse, PrintNode);
            printf("%s]", spaces);
            printf(", \n%s\033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36mdepth\033[0m: \033[0;31m%zu\033[0m \n%s}",
                   spaces, type, node->depth, Slice(spaces, 0, 1 * (node->depth - (node->depth > 1))));
            puts("");
            break;
        case WHILE:
            printf("{ \n%s\033[0;36mtest\033[0m: ", spaces);
            PrintNode(node->whileStmt->test);
            printf(", \n%s\033[0;36mbody\033[0m: [\n", spaces);
            Node_ForEach(&node->whileStmt->body, PrintNode);
            printf("%s]", spaces);
            printf(",\n%s\033[0;36melse\033[0m: [\n", spaces);
            Node_ForEach(&node->whileStmt->orelse, PrintNode);
            printf("%s]", spaces);
            printf(", \n%s\033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36mdepth\033[0m: \033[0;31m%zu\033[0m \n%s}",
                   spaces, type, node->depth, Slice(spaces, 0, 1 * (node->depth - (node->depth > 1))));
            puts("");
            break;
        case FOR:
            printf("{ \n%s\033[0;36mtarget\033[0m: ", spaces);
            PrintVar(node->forStmt->target.variable, node->depth);
            printf(", \n%s\033[0;36miter\033[0m: ", spaces);
            PrintNode(node->forStmt->iter);
            printf(", \n%s\033[0;36mbody\033[0m: [\n", spaces);
            Node_ForEach(&node->forStmt->body, PrintNode);
            Node_ForEach(&node->forStmt->orelse, PrintNode);
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
            TraverseTree(node->binOp->left, depth + 1);
            TraverseTree(node->binOp->right, depth + 1);
            break;
        case ASSIGNMENT:
            TraverseTree(node->assignStmt->value, node->depth + 1);
            break;
        case IF:
            TraverseTree(node->ifStmt->test, depth + 1);
            Node_Node *current = node->ifStmt->body.head;
            while (current != NULL) {
                TraverseTree(current->data, node->depth + 1);
                current = current->next;
            }
            current = node->ifStmt->orelse.head;
            while (current != NULL) {
                TraverseTree(current->data, node->depth + 1);
                current = current->next;
            }
            break;
        case LIST_EXPR:
            current = node->list->elts.head;
            while (current != NULL) {
                TraverseTree(current->data, node->depth + 1);
                current = current->next;
            }
            break;
        case WHILE:
            TraverseTree(node->whileStmt->test, depth + 1);
            current = node->whileStmt->body.head;
            while (current != NULL) {
                TraverseTree(current->data, node->depth + 1);
                current = current->next;
            }
            break;
        default:
            break;
    }
}

cJSON *SerializeProgram(Node_LinkedList *program) {
    cJSON *root = cJSON_CreateArray();
    Node_Node *current = program->head;
    while (current != NULL) {
        cJSON_AddItemToArray(root, SerializeNode(current->data));
        current = current->next;
    }
    return root;
}

cJSON *SerializeNode(Node *node) {
    if (node == NULL) return NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", NodeTypeToString(node->type));
    cJSON_AddNumberToObject(root, "depth", node->depth);

    switch (node->type) {
        case LITERAL:
            cJSON_AddStringToObject(root, "dataType", DataTypeToString(node->literal->type));
            cJSON_AddStringToObject(root, "value", node->literal->value);
            break;
        case IMPORT: {
            cJSON *modules = cJSON_CreateArray();
            cJSON_AddItemToObject(root, "modules", modules);
            for (size_t i = 0; i < node->importStmt->modules.size; ++i) {
                Token *token = Token_Get(&node->importStmt->modules, i);
                cJSON_AddItemToArray(modules, SerializeToken(token));
            }
        }
            break;
        case VARIABLE:
            cJSON_AddItemToObject(root, "variable", SerializeName(node->variable));
            break;
        case UNARY_OPERATION:
            cJSON_AddStringToObject(root, "dataType", DataTypeToString(node->unOp->type));
            cJSON_AddStringToObject(root, "operator", node->unOp->operator);
            cJSON_AddItemToObject(root, "operand", SerializeNode(node->unOp->operand));
            break;
        case BINARY_OPERATION:
            cJSON_AddStringToObject(root, "dataType", DataTypeToString(node->binOp->type));
            cJSON_AddStringToObject(root, "operator", node->binOp->operator);
            cJSON_AddItemToObject(root, "left", SerializeNode(node->binOp->left));
            cJSON_AddItemToObject(root, "right", SerializeNode(node->binOp->right));
            break;
        case ASSIGNMENT:
            cJSON_AddItemToObject(root, "target", SerializeName(node->assignStmt->target));
            cJSON_AddItemToObject(root, "value", SerializeNode(node->assignStmt->value));
            break;
        case COMPARE:
            cJSON_AddItemToObject(root, "left", SerializeNode(node->compare->left));
            cJSON *ops = cJSON_CreateArray();
            cJSON_AddItemToObject(root, "ops", ops);
            cJSON_AddItemToObject(root, "comparators", SerializeProgram(&node->compare->comparators));
            for (size_t i = 0; i < node->compare->ops.size; ++i) {
                Token *token = Token_Get(&node->compare->ops, i);
                cJSON_AddItemToArray(ops, SerializeToken(token));
            }
            break;
        case IF:
            cJSON_AddItemToObject(root, "test", SerializeNode(node->ifStmt->test));
            cJSON_AddItemToObject(root, "body", SerializeProgram(&node->ifStmt->body));
            cJSON_AddItemToObject(root, "orelse", SerializeProgram(&node->ifStmt->orelse));
            break;
        case LIST_EXPR:
            cJSON_AddItemToObject(root, "list", SerializeProgram(&node->list->elts));
            break;
        case WHILE:
            cJSON_AddItemToObject(root, "test", SerializeNode(node->whileStmt->test));
            cJSON_AddItemToObject(root, "body", SerializeProgram(&node->whileStmt->body));
            cJSON_AddItemToObject(root, "orelse", SerializeProgram(&node->whileStmt->orelse));
            break;
        case FOR:
            cJSON_AddItemToObject(root, "target", SerializeName(node->assignStmt->target));
            cJSON_AddItemToObject(root, "iter", SerializeNode(node->forStmt->iter));
            cJSON_AddItemToObject(root, "body", SerializeProgram(&node->forStmt->body));
            cJSON_AddItemToObject(root, "orelse", SerializeProgram(&node->forStmt->orelse));
            break;
        default:
            break;
    }
    return root;
}

cJSON *SerializeToken(Token *token) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", TokenTypeToString(token->type));
    cJSON_AddStringToObject(root, "lexeme", token->lexeme);
    cJSON_AddNumberToObject(root, "line", token->line);
    cJSON_AddNumberToObject(root, "col", token->col);
    return root;
}

cJSON *SerializeName(Name *variable) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "identifier", variable->id);
    cJSON_AddStringToObject(root, "dataType", DataTypeToString(variable->type));
    cJSON_AddStringToObject(root, "ctx", CtxToString(variable));
    return root;
}

void PrintVar(Name *variable, size_t depth) {
    printf("{ \033[0;36midentifer\033[0m: \033[0;33m%s\033[0m, \033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36m\033[0;36mctx\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m, \033[0;36mdepth\033[0m: \033[0;31m%zu\033[0m }",
           variable->id, NodeTypeToString(VARIABLE), CtxToString(variable), depth);
}

void PrintImportStmt(Node const *stmt) {
    const char *type = NodeTypeToString(IMPORT);
    printf("{ \033[0;36mmodules\033[0m: [ \n");
    Token_ForEach(&stmt->importStmt->modules, PrintToken);
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
        case COMPARE:
            return "COMPARE";
        case BINARY_OPERATION:
            return "BINARY OPERATION";
        case LIST_EXPR:
            return "LIST_EXPR";
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

const char *DataTypeToString(DataType type) {
    switch (type) {
        case FLOAT:
            return "FLOAT";
        case INT:
            return "INT";
        case STR:
            return "STR";
        case COMPLEX:
            return "COMPLEX";
        case LIST:
            return "LIST";
        case BOOL:
            return "BOOL";
        case VOID:
            return "VOID";
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

DataType InferType(Node const *node, Symbol *namespace) {
    switch (node->type) {
        case ASSIGNMENT:
            return InferType(node->assignStmt->value, namespace);
        case LITERAL:
            return node->literal->type;
        case BINARY_OPERATION: {
            DataType left = InferType(node->binOp->left, namespace);
            DataType right = InferType(node->binOp->right, namespace);
            bool isComparison = Any((void **) COMPARISON_OPERATORS, ARRAYSIZE(COMPARISON_OPERATORS),
                                    node->binOp->operator,
                                    (CompareFn) StrEQ);
            return isComparison ? BOOL : TypePrecedence(left, right);
        }
        case UNARY_OPERATION:
            return InferType(node->unOp->operand, namespace);
        case VARIABLE:
            return StackSymbolsLookup(namespace, node->variable->id)->type;
        case LIST_EXPR:
            return LIST;
        default:
            fprintf(stderr, "ERROR: Node not supported for data type \"%s\"\n", NodeTypeToString(node->type));
            exit(EXIT_FAILURE);
    }
}

DataType InferListType(Node_LinkedList list, Symbol *namespace) {
    Node_Node *current = list.head;
    DataType hType = InferType(current->data, namespace);
    for (; current != NULL; current = current->next) {
        if (hType != InferType(current->data, namespace)) return OBJECT;
    }
    return hType;
}

DataType TypePrecedence(DataType left, DataType right) {
    if (left == INT && (right == FLOAT || right == COMPLEX)) {
        return right;
    } else if (left == FLOAT && right == COMPLEX) {
        return right;
    }

    return left;
}

Symbol *CreateSymbol(DataType type, SymbolType kind) {
    Symbol *sym = AllocateContext(sizeof(Symbol));
    sym->kind = kind;
    sym->type = type;
    sym->parent = NULL;
    sym->col = 0;
    sym->line = 0;
    sym->scope = Symbol_CreateHashTable(10);
    return sym;
}

cJSON *SerializeSymbolTable(const Symbol_HashTable *namespaces) {
    cJSON *root = cJSON_CreateArray();
    for (size_t i = 0; i < namespaces->capacity; ++i) {
        char_Symbol_Pair *kv = namespaces->buckets[i];
        cJSON *item = SerializeSymbol(kv->value);
        cJSON_AddStringToObject(item, "id", kv->key);
        cJSON_AddItemToArray(root, item);
    }
    return root;
}

cJSON *SerializeSymbol(const Symbol *symbol) {
    if (symbol == NULL) return NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "kind", SymbolTypeToString(symbol->kind));
    cJSON_AddStringToObject(root, "dataType", DataTypeToString(symbol->type));
    cJSON_AddItemToObject(root, "scope", SerializeSymbolTable(&symbol->scope));
    cJSON_AddNumberToObject(root, "line", symbol->line);
    cJSON_AddNumberToObject(root, "col", symbol->col);
    return root;
}

const char *SymbolTypeToString(SymbolType type) {
    switch (type) {
        case CLASS:
            return "CLASS";
        case FUNCTION:
            return "FUNCTION";
        case MODULE:
            return "MODULE";
        case VAR:
            return "VAR";
        case BLOCK:
            return "BLOCK";
        default:
            return "UNKNOWN";
    }
}

