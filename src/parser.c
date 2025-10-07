#include "parser.h"

ASTNode *parse_statement(Parser *parser);

ASTNode *parse_if_statement(Parser *parser, ASTNode *if_node);

const char *COMPARISON_OPERATORS[] = {"==", "!=", ">", "<", ">=", "<="};

static inline Parser parser_new(Lexer *lexer) {
  return (Parser){.lexer = lexer, .ast = ASTNode_new(DEFAULT_CAP)};
}

const char *node_type_to_string(NodeType type) {
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
    return "LIST EXPR";
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

void syntax_error(const char *message, Token *token) {
  trace_log(LOG_FATAL, "SyntaxError: %s at line %d, near '%s'\n", message,
            token ? token->line : 1, token ? token->lexeme : "EOF");
}

ASTNode *node_new(Parser *parser, Token *token, NodeType type) {
  ASTNode *node = allocator_alloc(&parser->ast.allocator, sizeof(ASTNode));
  if (node == NULL) {
    trace_log(LOG_ERROR, "Could not allocate memory for AST node");
    return NULL;
  }

  node->type = type;
  node->token = token;
  node->depth = 1;
  return node;
}

cJSON *serialize_program(ASTNode_LinkedList *program) {
  cJSON *root = cJSON_CreateArray();

  for (size_t current = program->head; current != SIZE_MAX;
       current = program->elements[current].next) {
    cJSON_AddItemToArray(root, serialize_node(program->elements[current].data));
  }
  return root;
}

cJSON *serialize_node(ASTNode *node) {
  if (node == NULL)
    return NULL;
  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "type", node_type_to_string(node->type));
  cJSON_AddNumberToObject(root, "depth", node->depth);

  switch (node->type) {
  case ASSIGNMENT:
    cJSON_AddItemToObject(root, "targets",
                          serialize_program(&node->assign.targets));
    cJSON_AddItemToObject(root, "value", serialize_node(node->assign.value));
    break;
  case VARIABLE:
  case LITERAL:
    cJSON_AddItemToObject(root, "token", serialize_token(node->token));
    break;
  case BINARY_OPERATION:
    cJSON_AddItemToObject(root, "token", serialize_token(node->token));
    cJSON_AddItemToObject(root, "left", serialize_node(node->bin_op.left));
    cJSON_AddItemToObject(root, "right", serialize_node(node->bin_op.right));
    break;
  case IMPORT:
    cJSON_AddItemToObject(root, "token", serialize_token(node->token));
    cJSON_AddItemToObject(root, "names", serialize_program(&node->import));
    break;
  case COMPARE:
    cJSON_AddItemToObject(root, "left", serialize_node(node->compare.left));
    cJSON *ops = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "ops", ops);
    cJSON_AddItemToObject(root, "comparators",
                          serialize_program(&node->compare.comparators));
    for (size_t i = 0; i < node->compare.ops.size; ++i) {
      Token *token = Token_get(&node->compare.ops, i);
      cJSON_AddItemToArray(ops, serialize_token(token));
    }
    break;
  case IF:
    cJSON_AddItemToObject(root, "token", serialize_token(node->token));
    cJSON_AddItemToObject(root, "test", serialize_node(node->if_stmt.test));
    cJSON_AddItemToObject(root, "body", serialize_program(&node->if_stmt.body));
    cJSON_AddItemToObject(root, "orelse",
                          serialize_program(&node->if_stmt.orelse));
    break;
  default:
    break;
  }
  return root;
}

ASTNode *bin_op_new(Parser *parser, Token *operation, ASTNode *left,
                    ASTNode *right) {
  ASTNode *node = node_new(parser, operation, BINARY_OPERATION);
  node->bin_op = (BinOp){.left = left, .right = right};
  return node;
}

bool blacklist_tokens(TokenType type, const TokenType blacklist[],
                      size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (blacklist[i] == type) {
      return true;
    }
  }
  return false;
}

Token_ArrayList collect_expression(Parser *parser, size_t from) {
  Token *token = Token_get(&parser->lexer->tokens, from);
  Token_ArrayList expression =
      Token_new_with_allocator(&parser->ast.allocator, 20);
  TokenType blacklist[] = {NEWLINE, ENDMARKER, COLON};
  size_t scope = token->ident;

  for (size_t j = from; j < parser->lexer->tokens.size; j++) {
    token = Token_get(&parser->lexer->tokens, j);

    // Check blacklist BEFORE pushing
    if (blacklist_tokens(token->type, blacklist, ARRAYSIZE(blacklist))) {
      break;
    }

    if (token->ident != scope) {
      break;
    }

    Token_push(&expression, token);
  }

  return expression;
}

ASTNode_LinkedList parse_identifier_list(Parser *parser, Token *token,
                                         size_t capacity) {
  ASTNode_LinkedList targets =
      ASTNode_new_with_allocator(&parser->ast.allocator, capacity);
  ASTNode *var = node_new(parser, token, VARIABLE);
  ASTNode_add_last(&targets, var);
  Token *next = next_token(parser->lexer);

  for (; next != NULL && next->type == COMMA;
       next = next_token(parser->lexer)) {
    if (token == NULL || token->type != IDENTIFIER) {
      syntax_error("expected identifier after comma", token);
    }

    token = next_token(parser->lexer);
    var = node_new(parser, token, VARIABLE);
    ASTNode_add_last(&targets, var);
  }

  return targets;
}

Token_ArrayList infix_to_postfix(Allocator *allocator,
                                 Token_ArrayList *tokens) {
  Token_ArrayList stack = Token_new_with_allocator(allocator, 10);
  Token_ArrayList postfix = Token_new_with_allocator(allocator, 10);

  for (size_t i = 0; i < tokens->size; i++) {
    Token *token = Token_get(tokens, i);

    if (token->type == IDENTIFIER || token->type == NUMBER) {
      Token_push(&postfix, token);
    } else if (strcmp(token->lexeme, "(") == 0) {
      Token_push(&stack, token);
    } else if (strcmp(token->lexeme, ")") == 0) {
      Token const *last = Token_get(&stack, stack.size - 1);

      while (stack.size > 0 && strcmp(last->lexeme, "(") != 0) {
        Token_push(&postfix, Token_pop(&stack));
      }

      if (stack.size > 0 && strcmp(last->lexeme, "(") == 0) {
        Token_pop(&stack); // Pop the open bracket
      }
    } else {
      Token *last = Token_get(&stack, stack.size - 1);

      while (stack.size > 0 &&
             precedence(last->lexeme) >= precedence(token->lexeme) &&
             strcmp(last->lexeme, "(") != 0) {
        Token_push(&postfix, Token_pop(&stack));
        last = stack.size > 0 ? Token_get(&stack, stack.size - 1) : last;
      }

      Token_push(&stack, token);
    }
  }

#ifdef __GNUC__
#pragma GCC unroll 100
#endif
  while (stack.size > 0) {
    Token_push(&postfix, Token_pop(&stack));
  }

  return postfix;
}

ASTNode *shunting_yard(Parser *parser, Token_ArrayList *tokens) {
  ASTNode_LinkedList stack = ASTNode_new(DEFAULT_CAP);

  for (size_t i = 0; i < tokens->size; i++) {
    Token *token = Token_get(tokens, i);

    switch (token->type) {
    case STRING:
    case NUMBER: {
      ASTNode *literal = node_new(parser, token, LITERAL);
      ASTNode_add_last(&stack, literal);
    } break;
    case IDENTIFIER: {
      ASTNode *var = node_new(parser, token, VARIABLE);
      ASTNode_add_last(&stack, var);
    } break;
    case OPERATOR: {
      ASTNode *right = ASTNode_pop(&stack);
      ASTNode *left = ASTNode_pop(&stack);

      if (right != NULL && left != NULL) {
        bool exists = false;
        ANY(COMPARISON_OPERATORS, ARRAYSIZE(COMPARISON_OPERATORS),
            token->lexeme,
            strcmp(COMPARISON_OPERATORS[index], token->lexeme) == 0, exists);
        if (exists) {
          ASTNode *comp = NULL;
          if (left->type == COMPARE) {
            ASTNode_add_first(&left->compare.comparators, right);
            Token_push(&left->compare.ops, token);
            comp = left;
          } else {
            comp = node_new(parser, NULL, COMPARE);
            comp->compare.left = left;
            comp->compare.comparators =
                ASTNode_new_with_allocator(&parser->ast.allocator, 3);
            comp->compare.ops =
                Token_new_with_allocator(&parser->ast.allocator, 3);
            ASTNode_add_first(&comp->compare.comparators, right);
            Token_push(&comp->compare.ops, token);
          }
          ASTNode_add_first(&stack, comp);
          continue;
        }

        ASTNode *node = bin_op_new(parser, token, left, right);
        ASTNode_add_last(&stack, node);
        continue;
      }
    } break;
    default:
      break;
    }
  }
  ASTNode *root = ASTNode_pop(&stack);
  ASTNode_free(&stack);
  return root;
}

ASTNode *parse_expression(Parser *parser) {
  Token_ArrayList expression =
      collect_expression(parser, parser->lexer->token_idx - 1);
  parser->lexer->token_idx += expression.size;
  expression = infix_to_postfix(&parser->ast.allocator, &expression);
  return shunting_yard(parser, &expression);
}

ASTNode *parse_if_statement(Parser *parser, ASTNode *if_node) {
  ASTNode *condition = parse_expression(parser);
  if_node->if_stmt.test = condition;
  if_node->if_stmt.body = ASTNode_new_with_allocator(&parser->ast.allocator, 4);
  if_node->if_stmt.orelse =
      ASTNode_new_with_allocator(&parser->ast.allocator, 4);
  parser->lexer->token_idx--;

  // Expect a COLON after the expression
  Token *token = next_token(parser->lexer);
  if (token == NULL || token->type != COLON) {
    syntax_error("expected ':' after 'if' condition", token);
    return NULL;
  }

  // Expect a NEWLINE immediately after the colon
  token = next_token(parser->lexer);
  if (token == NULL || token->type != NEWLINE) {
    syntax_error("expected newline after ':' in 'if' statement", token);
    return NULL;
  }

  while (((token = next_token(parser->lexer)) != NULL)) {
    ASTNode *stmt = parse_statement(parser);

    if (stmt == NULL || stmt->type == END_BLOCK) {
      break;
    }

    ASTNode_add_last(&if_node->if_stmt.body, stmt);
  }

  token = Token_get(&parser->lexer->tokens, parser->lexer->token_idx - 1);
  if (token && token->type == KEYWORD && strcmp(token->lexeme, "elif") == 0) {
    // parser->lexer->token_idx--;
    ASTNode *elif_node = node_new(parser, token, IF);
    ASTNode *parsed_elif = parse_if_statement(parser, elif_node);
    ASTNode_add_last(&if_node->if_stmt.orelse, parsed_elif);
  } else if (token && token->type == KEYWORD &&
             strcmp(token->lexeme, "else") == 0) {
    next_token(parser->lexer); // consume 'else'

    // Expect COLON
    token = next_token(parser->lexer);
    if (token == NULL || token->type != COLON) {
      syntax_error("expected ':' after 'else'", token);
      return NULL;
    }

    // Expect NEWLINE
    token = next_token(parser->lexer);
    if (token == NULL || token->type != NEWLINE) {
      syntax_error("expected newline after ':' in 'else' statement", token);
      return NULL;
    }

    // Parse the 'else' block
    while ((token = next_token(parser->lexer)) != NULL) {
      ASTNode *stmt = parse_statement(parser);
      if (stmt == NULL || stmt->type == END_BLOCK)
        break;
      ASTNode_add_last(&if_node->if_stmt.orelse, stmt);
    }
  }
  return if_node;
}

ASTNode *parse_statement(Parser *parser) {
  Token *token =
      Token_get(&parser->lexer->tokens, parser->lexer->token_idx - 1);
  switch (token->type) {
  case NUMBER: {
    return parse_expression(parser);
  }
  case IDENTIFIER: {
    ASTNode_LinkedList targets = parse_identifier_list(parser, token, 1);
    Token *next =
        Token_get(&parser->lexer->tokens, parser->lexer->token_idx - 1);

    if (next != NULL && strcmp(next->lexeme, "=") == 0) {
      parser->lexer->token_idx++;
      ASTNode *node = node_new(parser, next, ASSIGNMENT);
      ASTNode *expr = parse_expression(parser);
      node->assign = (Assign){.targets = targets, .value = expr};
      return node;
    }

    ASTNode_free(&targets);
    parser->lexer->token_idx--;
    return parse_expression(parser);
  } break;
  case KEYWORD: {
    if (strcmp(token->lexeme, "import") == 0) {
      ASTNode *node = node_new(parser, token, IMPORT);
      token = next_token(parser->lexer);
      node->import = parse_identifier_list(parser, token, 5);
      return node;
    }

    if (strcmp(token->lexeme, "if") == 0) {
      ASTNode *node = node_new(parser, token, IF);
      return parse_if_statement(parser, node);
    }

    if (strcmp(token->lexeme, "elif") == 0 ||
        strcmp(token->lexeme, "else") == 0) {
      // Signal end of current block - elif/else should be handled by parent if
      return node_new(parser, token, END_BLOCK);
    }
  } break;
  case NEWLINE: {
    Token *next = Token_get(&parser->lexer->tokens, parser->lexer->token_idx);

    if (next->ident != token->ident) {
      return node_new(parser, token, END_BLOCK);
    }
  } break;
  default:
    break;
  }

  return NULL;
}

Parser parse(Lexer *lexer) {
  Parser parser = parser_new(lexer);
  Token *token = NULL;

  while ((token = next_token(parser.lexer)) != NULL) {
    ASTNode *stmt = parse_statement(&parser);
    ASTNode_add_last(&parser.ast, stmt);
  }

  return parser;
}

size_t precedence(const char *operator) {
  if (strcmp(operator, "+") == 0 || strcmp(operator, "-") == 0) {
    return 1;
  }

  if (strcmp(operator, "*") == 0 || strcmp(operator, "/") == 0) {
    return 2;
  }

  if (strcmp(operator, "^") == 0) {
    return 3;
  }

  if (strcmp(operator, "<") == 0 || strcmp(operator, ">") == 0 ||
      strcmp(operator, "<=") == 0 || strcmp(operator, ">=") == 0 ||
      strcmp(operator, "==") == 0 || strcmp(operator, "!=") == 0) {
    return 0;
  }

  return -1;
}

void parser_free(Parser *parser) {
  if (!parser)
    return;

  ASTNode_free(&parser->ast);
  Token_free(&parser->lexer->tokens);
  parser->lexer = NULL;
}