#include "parser.h"

ASTNode *parse_statement(Parser *parser);

ASTNode *parse_if_statement(Parser *parser, ASTNode *if_node);

const char *COMPARISON_OPERATORS[] = {"==", "!=", ">", "<", ">=", "<="};

const char *AUG_ASSIGN_OPS[] = {"+=", "-=", "*=",  "@=",  "/=",  "%=", "&=",
                                "|=", "^=", "<<=", ">>=", "**=", "//="};

static inline Parser parser_new(Lexer *lexer) {
  return (Parser){.lexer = lexer, .ast = ASTNode_new(DEFAULT_CAP)};
}

static bool is_augassign_op(const char *lexeme) {
  for (uint8_t i = 0; i < ARRAYSIZE(AUG_ASSIGN_OPS); i++) {
    if (strcmp(lexeme, AUG_ASSIGN_OPS[i]) == 0)
      return true;
  }
  return false;
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
  case AUG_ASSIGNMENT:
    return "AUGMENTED ASSIGNMENT";
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
  case FUNCTION_DEF:
    return "FUNCTION DEF";
  case RETURN:
    return "RETURN";
  default:
    return "UNKNOWN";
  }
}

void syntax_error(const char *message, const char *filename, Token *token) {
  size_t line = token ? token->line : 1;
  size_t col = token ? token->col : 1;
  slog_error("%s:%d:%d SyntaxError: %s near '%s'.\n", filename, line, col,
             message, token ? token->lexeme : "EOF");
  exit(EXIT_FAILURE);
}

ASTNode *node_new(Parser *parser, Token *token, NodeType type) {
  ASTNode *node = allocator_alloc(&parser->ast.allocator, sizeof(ASTNode));
  if (node == NULL) {
    slog_error("Could not allocate memory for AST node");
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
  case AUG_ASSIGNMENT:
    cJSON_AddItemToObject(root, "target",
                          serialize_node(node->aug_assign.target));
    cJSON_AddItemToObject(root, "op", serialize_token(node->aug_assign.op));
    cJSON_AddItemToObject(root, "value",
                          serialize_node(node->aug_assign.value));
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
  case WHILE:
    cJSON_AddItemToObject(root, "token", serialize_token(node->token));
    cJSON_AddItemToObject(root, "test", serialize_node(node->ctrl_stmt.test));
    cJSON_AddItemToObject(root, "body",
                          serialize_program(&node->ctrl_stmt.body));
    cJSON_AddItemToObject(root, "orelse",
                          serialize_program(&node->ctrl_stmt.orelse));
    break;
  case FUNCTION_DEF:
    cJSON_AddItemToObject(root, "name", serialize_node(node->funcdef.name));
    cJSON_AddItemToObject(root, "params",
                          serialize_program(&node->funcdef.params));
    cJSON_AddItemToObject(root, "body",
                          serialize_program(&node->funcdef.body));
    break;
  case RETURN:
    cJSON_AddItemToObject(root, "token", serialize_token(node->token));
    cJSON_AddItemToObject(root, "ret", serialize_node(node->ret));
    break;
  default:
    break;
  }
  return root;
}

char *dump_program(ASTNode_LinkedList *program) {
  cJSON *json = serialize_program(program);
  char *dump = cJSON_Print(json);
  cJSON_Delete(json);
  return dump;
}

char *dump_node(ASTNode *node) {
  cJSON *json = serialize_node(node);
  char *dump = cJSON_Print(json);
  cJSON_Delete(json);
  return dump;
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
      Token_new_with_allocator(&parser->ast.allocator, 1);
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

ASTNode_LinkedList parse_identifier_list(Parser *parser, Token *token) {
  ASTNode_LinkedList targets =
      ASTNode_new_with_allocator(&parser->ast.allocator, 1);
  ASTNode *var = node_new(parser, token, VARIABLE);
  ASTNode_add_last(&targets, var);
  Token *next = next_token(parser->lexer);

  for (; next != NULL && next->type == COMMA;
       next = next_token(parser->lexer)) {
    if (token == NULL || token->type != IDENTIFIER) {
      syntax_error("expected identifier after comma", parser->lexer->filename,
                   token);
    }

    token = next_token(parser->lexer);
    var = node_new(parser, token, VARIABLE);
    ASTNode_add_last(&targets, var);
  }

  return targets;
}

Token_ArrayList infix_to_postfix(Allocator *allocator,
                                 Token_ArrayList *tokens) {
  Token_ArrayList stack = Token_new_with_allocator(allocator, 1);
  Token_ArrayList postfix = Token_new_with_allocator(allocator, 1);

  for (size_t i = 0; i < tokens->size; i++) {
    Token *token = Token_get(tokens, i);

    if (token->type == IDENTIFIER || token->type == NUMBER ||
        token->type == STRING) {
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
    case OPERATOR:
    case KEYWORD: {
      if (token->type == KEYWORD && !is_boolean_operator(token))
        break;
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

ASTNode *parse_while_statement(Parser *parser, ASTNode *while_node) {
  ASTNode *condition = parse_expression(parser);
  while_node->ctrl_stmt.test = condition;
  while_node->ctrl_stmt.body =
      ASTNode_new_with_allocator(&parser->ast.allocator, 4);
  while_node->ctrl_stmt.orelse =
      ASTNode_new_with_allocator(&parser->ast.allocator, 4);
  parser->lexer->token_idx--;

  // Expect a COLON after the expression
  Token *token = next_token(parser->lexer);
  if (token == NULL || token->type != COLON) {
    syntax_error("expected ':' after 'while' condition",
                 parser->lexer->filename, token);
    return NULL;
  }

  // Expect a NEWLINE immediately after the colon
  token = next_token(parser->lexer);
  if (token == NULL || token->type != NEWLINE) {
    syntax_error("expected newline after ':' in 'while' statement",
                 parser->lexer->filename, token);
    return NULL;
  }

  while (((token = next_token(parser->lexer)) != NULL)) {
    ASTNode *stmt = parse_statement(parser);

    if (stmt == NULL || stmt->type == END_BLOCK) {
      break;
    }

    ASTNode_add_last(&while_node->ctrl_stmt.body, stmt);
  }

  if (token && token->type == KEYWORD && strcmp(token->lexeme, "else") == 0) {
    token = next_token(parser->lexer);

    if (token == NULL || token->type != COLON) {
      syntax_error("expected ':' after 'else'", parser->lexer->filename, token);
      return NULL;
    }

    // Expect NEWLINE
    token = next_token(parser->lexer);

    if (token == NULL || token->type != NEWLINE) {
      syntax_error("expected newline after ':' in 'else' statement",
                   parser->lexer->filename, token);
      return NULL;
    }

    // Parse the 'else' block
    while ((token = next_token(parser->lexer)) != NULL) {
      ASTNode *stmt = parse_statement(parser);
      if (stmt == NULL || stmt->type == END_BLOCK)
        break;
      ASTNode_add_last(&while_node->ctrl_stmt.orelse, stmt);
    }
  }

  return while_node;
}

ASTNode *parse_if_statement(Parser *parser, ASTNode *if_node) {
  ASTNode *condition = parse_expression(parser);
  if_node->ctrl_stmt.test = condition;
  if_node->ctrl_stmt.body =
      ASTNode_new_with_allocator(&parser->ast.allocator, 4);
  if_node->ctrl_stmt.orelse =
      ASTNode_new_with_allocator(&parser->ast.allocator, 4);
  parser->lexer->token_idx--;

  // Expect a COLON after the expression
  Token *token = next_token(parser->lexer);
  if (token == NULL || token->type != COLON) {
    syntax_error("expected ':' after 'if' condition", parser->lexer->filename,
                 token);
    return NULL;
  }

  // Expect a NEWLINE immediately after the colon
  token = next_token(parser->lexer);
  if (token == NULL || token->type != NEWLINE) {
    syntax_error("expected newline after ':' in 'if' statement",
                 parser->lexer->filename, token);
    return NULL;
  }

  while (((token = next_token(parser->lexer)) != NULL)) {
    ASTNode *stmt = parse_statement(parser);

    if (stmt == NULL || stmt->type == END_BLOCK) {
      break;
    }

    ASTNode_add_last(&if_node->ctrl_stmt.body, stmt);
  }

  token = Token_get(&parser->lexer->tokens, parser->lexer->token_idx - 1);
  if (token && token->type == KEYWORD && strcmp(token->lexeme, "elif") == 0) {
    // parser->lexer->token_idx--;
    ASTNode *elif_node = node_new(parser, token, IF);
    ASTNode *parsed_elif = parse_if_statement(parser, elif_node);
    ASTNode_add_last(&if_node->ctrl_stmt.orelse, parsed_elif);
  } else if (token && token->type == KEYWORD &&
             strcmp(token->lexeme, "else") == 0) {
    token = next_token(parser->lexer);

    if (token == NULL || token->type != COLON) {
      syntax_error("expected ':' after 'else'", parser->lexer->filename, token);
      return NULL;
    }

    // Expect NEWLINE
    token = next_token(parser->lexer);

    if (token == NULL || token->type != NEWLINE) {
      syntax_error("expected newline after ':' in 'else' statement",
                   parser->lexer->filename, token);
      return NULL;
    }

    // Parse the 'else' block
    while ((token = next_token(parser->lexer)) != NULL) {
      ASTNode *stmt = parse_statement(parser);
      if (stmt == NULL || stmt->type == END_BLOCK)
        break;
      ASTNode_add_last(&if_node->ctrl_stmt.orelse, stmt);
    }
  }
  return if_node;
}

ASTNode *parse_function_declaration(Parser *parser, ASTNode *func_node) {
  Token *token = next_token(parser->lexer);
  if (token == NULL || token->type != IDENTIFIER) {
    syntax_error("expected function name after 'def'", parser->lexer->filename,
                 token);
    return NULL;
  }

  ASTNode *name_node = node_new(parser, token, VARIABLE);
  func_node->funcdef.name = name_node;

  // Expect opening parenthesis
  token = next_token(parser->lexer);
  if (token == NULL || token->type != LPAR) {
    syntax_error("expected '(' after function name", parser->lexer->filename,
                 token);
    return NULL;
  }

  func_node->funcdef.params =
      ASTNode_new_with_allocator(&parser->ast.allocator, 4);

  // Parse parameters
  token = next_token(parser->lexer);
  while (token != NULL && token->type != RPAR) {
    if (token->type == IDENTIFIER) {
      ASTNode *param_node = node_new(parser, token, VARIABLE);
      ASTNode_add_last(&func_node->funcdef.params, param_node);
    } else if (token->type != COMMA) {
      syntax_error("expected parameter name or ','", parser->lexer->filename,
                   token);
      return NULL;
    }
    token = next_token(parser->lexer);
  }

  // Expect colon
  token = next_token(parser->lexer);
  if (token == NULL || token->type != COLON) {
    syntax_error("expected ':' after function parameters",
                 parser->lexer->filename, token);
    return NULL;
  }

  // Expect NEWLINE
  token = next_token(parser->lexer);
  if (token == NULL || token->type != NEWLINE) {
    syntax_error("expected newline after ':' in function declaration",
                 parser->lexer->filename, token);
    return NULL;
  }

  func_node->funcdef.body =
      ASTNode_new_with_allocator(&parser->ast.allocator, 4);

  // Parse function body
  while ((token = next_token(parser->lexer)) != NULL) {
    ASTNode *stmt = parse_statement(parser);
    if (stmt == NULL || stmt->type == END_BLOCK)
      break;
    ASTNode_add_last(&func_node->funcdef.body, stmt);
  }

  return func_node;
}

ASTNode *parse_statement(Parser *parser) {
  Token *token =
      Token_get(&parser->lexer->tokens, parser->lexer->token_idx - 1);
  switch (token->type) {
  case NUMBER: {
    return parse_expression(parser);
  }
  case IDENTIFIER: {
    ASTNode_LinkedList targets = parse_identifier_list(parser, token);
    Token *next =
        Token_get(&parser->lexer->tokens, parser->lexer->token_idx - 1);

    if (next != NULL && is_augassign_op(next->lexeme)) {
      parser->lexer->token_idx++;
      ASTNode *node = node_new(parser, next, AUG_ASSIGNMENT);
      ASTNode *expr = parse_expression(parser);
      ASTNode *target = ASTNode_pop(&targets);
      node->aug_assign =
          (AugAssign){.target = target, .op = next, .value = expr};
      return node;
    }

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
      node->import = parse_identifier_list(parser, token);
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

    if (strcmp(token->lexeme, "while") == 0) {
      ASTNode *node = node_new(parser, token, WHILE);
      return parse_while_statement(parser, node);
    }

    if (strcmp(token->lexeme, "def") == 0) {
      ASTNode *node = node_new(parser, token, FUNCTION_DEF);
      return parse_function_declaration(parser, node);
    }

    if (strcmp(token->lexeme, "return") == 0) {
      ASTNode *node = node_new(parser, token, RETURN);

      Token *next = peek_token(parser->lexer);
      if (next != NULL && next->type != NEWLINE) {
        node->ret = parse_expression(parser);
      } else {
        node->ret = NULL;
      }
      return node;
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

int8_t precedence(const char *op) {
  // Unary operators
  if (strcmp(op, "not") == 0)
    return 5;

  // Exponentiation
  if (strcmp(op, "^") == 0)
    return 4;

  // Multiplicative
  if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0)
    return 3;

  // Additive
  if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0)
    return 2;

  // Comparisons
  if (strcmp(op, "<") == 0 || strcmp(op, ">") == 0 || strcmp(op, "<=") == 0 ||
      strcmp(op, ">=") == 0 || strcmp(op, "==") == 0 || strcmp(op, "!=") == 0)
    return 1;

  // Logical AND / OR
  if (strcmp(op, "and") == 0)
    return 0;
  if (strcmp(op, "or") == 0)
    return -1;

  return -99; // Unknown operator
}

void parser_free(Parser *parser) {
  if (!parser)
    return;

  ASTNode_free(&parser->ast);
  Token_free(&parser->lexer->tokens);
  parser->lexer = NULL;
}