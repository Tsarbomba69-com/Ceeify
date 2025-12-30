#include "parser.h"

const char *COMPARISON_OPERATORS[] = {"==", "!=", ">", "<", ">=", "<="};

const char *AUG_ASSIGN_OPS[] = {"+=", "-=", "*=",  "@=",  "/=",  "%=", "&=",
                                "|=", "^=", "<<=", ">>=", "**=", "//="};

Token *next_token(Parser *parser) {
  if (parser->lexer->token_idx >= parser->lexer->tokens.size) {
    parser->current = NULL;
    parser->next = NULL;
    return NULL;
  }

  parser->current = Token_get(&parser->lexer->tokens, parser->lexer->token_idx);
  parser->lexer->token_idx++;

  if (parser->lexer->token_idx < parser->lexer->tokens.size) {
    parser->next = Token_get(&parser->lexer->tokens, parser->lexer->token_idx);
  } else {
    parser->next = NULL;
  }

  return parser->current;
}

ASTNode *parse_statement(Parser *parser);

ASTNode *parse_if_statement(Parser *parser, ASTNode *if_node);

ASTNode *parse_expression(Parser *parser, int8_t min_precedence);

ASTNode *parse_call(Parser *parser, ASTNode *callee);

bool blacklist_tokens(TokenType type, const TokenType blacklist[], size_t size);

ASTNode *bin_op_new(Parser *parser, Token *operation, ASTNode *left,
                    ASTNode *right);

static inline Parser parser_new(Lexer *lexer) {
  return (Parser){.lexer = lexer,
                  .current = NULL,
                  .next = peek_token(lexer),
                  .ast = ASTNode_new(DEFAULT_CAP)};
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
  case CALL:
    return "CALL";
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
  node->annotation = NULL;
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
    if (node->annotation) {
      cJSON_AddItemToObject(root, "annotation",
                            serialize_node(node->annotation));
    }
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
    cJSON_AddItemToObject(root, "body", serialize_program(&node->funcdef.body));
    break;
  case RETURN:
    cJSON_AddItemToObject(root, "token", serialize_token(node->token));
    cJSON_AddItemToObject(root, "ret", serialize_node(node->ret));
    break;
  case CALL:
    cJSON_AddItemToObject(root, "token", serialize_token(node->token));
    cJSON_AddItemToObject(root, "args", serialize_program(&node->call.args));
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

int8_t get_infix_precedence(const char *op) {
  if (strcmp(op, "and") == 0)
    return 10;
  if (strcmp(op, "or") == 0)
    return 5;

  // Comparisons
  if (strcmp(op, "<") == 0 || strcmp(op, ">") == 0 || strcmp(op, "<=") == 0 ||
      strcmp(op, ">=") == 0 || strcmp(op, "==") == 0 || strcmp(op, "!=") == 0)
    return 20;

  // Bitwise OR
  if (strcmp(op, "|") == 0)
    return 21;

  // Bitwise XOR
  if (strcmp(op, "^") == 0)
    return 22;

  // Bitwise AND
  if (strcmp(op, "&") == 0)
    return 23;

  // Shift Operators
  if (strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0)
    return 24;

  // Additive
  if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0)
    return 30;

  // Multiplicative
  if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0)
    return 40;

  // Exponentiation (Right-associative)
  if (strcmp(op, "**") == 0)
    return 50;

  if (strcmp(op, "(") == 0)
    return 70;

  return 0;
}

int8_t get_prefix_precedence(const char *op) {
  if (strcmp(op, "not") == 0)
    return 60;
  if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0)
    return 60;

  // Bitwise NOT
  if (strcmp(op, "~") == 0)
    return 65;

  return 0;
}

static inline bool is_prefix_operator(Token *t) {
  if (t->type == OPERATOR) {
    if (strcmp(t->lexeme, "+") == 0 || strcmp(t->lexeme, "-") == 0 ||
        strcmp(t->lexeme, "~") == 0) {
      return true;
    }
  }
  return t->type == KEYWORD && strcmp(t->lexeme, "not") == 0;
}

static inline bool is_comparison_operator(Token *t) {
  for (size_t j = 0; j < ARRAYSIZE(COMPARISON_OPERATORS); j++) {
    if (strcmp(COMPARISON_OPERATORS[j], t->lexeme) == 0) {
      return true;
    }
  }

  return false;
}

ASTNode_LinkedList parse_argument_list(Parser *parser);

// NUD (Null Denotation) - Parses a token that starts an expression
ASTNode *nud(Parser *parser) {
  Token *token = parser->current;
  switch (token->type) {
  case NUMBER:
  case STRING:
    return node_new(parser, token, LITERAL);

  case IDENTIFIER:
    if (parser->next->type == LPAR) {
      ASTNode *callee = node_new(parser, token, VARIABLE);
      return parse_call(parser, callee);
    }
    return node_new(parser, token, VARIABLE);
  case LPAR: {
    next_token(parser);
    ASTNode *expr = parse_expression(parser, 0);
    next_token(parser);
    return expr;
  }
  case KEYWORD:
  case OPERATOR:
    if (is_prefix_operator(token)) {
      ASTNode *node = node_new(parser, token, UNARY_OPERATION);
      int8_t rbp = get_prefix_precedence(token->lexeme);
      node->bin_op.right = parse_expression(parser, rbp);
      return node;
    }
    break;

  default:
    break;
  }

  syntax_error(
      "expected start of expression (literal, variable, or prefix operator)",
      parser->lexer->filename, token);
  return NULL;
}

ASTNode *led(Parser *parser, ASTNode *left) {
  Token *op_token = parser->current;
  if (op_token->type == LPAR) {
    return parse_call(parser, left);
  }

  int8_t lbp = get_infix_precedence(op_token->lexeme);
  ASTNode *right = NULL;
  // Right-associativity for Exponentiation (e.g., a ** b ** c -> a ** (b ** c))
  int8_t rbp = (strcmp(op_token->lexeme, "**") == 0) ? lbp - 1 : lbp;

  if (is_comparison_operator(op_token)) {
    ASTNode *comp = NULL;

    if (left->type == COMPARE) {
      comp = left;
    } else {
      comp = node_new(parser, NULL, COMPARE);
      comp->compare.left = left;
      comp->compare.comparators =
          ASTNode_new_with_allocator(&parser->ast.allocator, 3);
      comp->compare.ops = Token_new_with_allocator(&parser->ast.allocator, 3);
    }

    next_token(parser);
    right = parse_expression(parser, lbp + 1);
    ASTNode_add_last(&comp->compare.comparators, right);
    Token_push(&comp->compare.ops, op_token);
    return comp;
  }
  // Regular binary operation
  next_token(parser);
  right = parse_expression(parser, rbp);
  ASTNode *node = bin_op_new(parser, op_token, left, right);
  return node;
}

ASTNode *parse_expression(Parser *parser, int8_t rbp) {
  ASTNode *left = nud(parser);

  while (parser->next &&
         (parser->next->type == OPERATOR || parser->next->type == KEYWORD) &&
         rbp < get_infix_precedence(parser->next->lexeme)) {
    if (left->type == COMPARE && !is_comparison_operator(parser->next)) {
      break;
    }

    next_token(parser);
    left = led(parser, left);
  }

  return left;
}

ASTNode_LinkedList parse_argument_list(Parser *parser) {
  ASTNode_LinkedList args =
      ASTNode_new_with_allocator(&parser->ast.allocator, 4);

  // Check for empty argument list: f()
  if (parser->next && parser->next->type == RPAR) {
    return args;
  }

  next_token(parser);

  do {
    ASTNode *arg = parse_expression(parser, 0);
    ASTNode_add_last(&args, arg);
    next_token(parser);

    // Check the token immediately after the expression
    if (parser->current == NULL)
      break;

    if (parser->current->type == COMMA) {
      next_token(parser); // Consume COMMA
    } else if (parser->current->type == RPAR ||
               parser->current->type == ENDMARKER) {
      break; // Done with arguments
    } else {
      continue;
    }
  } while (parser->current && parser->current->type != RPAR);

  return args;
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

ASTNode_LinkedList parse_identifier_list(Parser *parser, Token *token) {
  ASTNode_LinkedList targets =
      ASTNode_new_with_allocator(&parser->ast.allocator, 1);
  ASTNode *var = node_new(parser, token, VARIABLE);
  ASTNode_add_last(&targets, var);
  Token *next = next_token(parser);

  for (; next != NULL && next->type == COMMA; next = next_token(parser)) {
    if (token == NULL || token->type != IDENTIFIER) {
      syntax_error("expected identifier after comma", parser->lexer->filename,
                   token);
    }

    token = next_token(parser);
    var = node_new(parser, token, VARIABLE);
    ASTNode_add_last(&targets, var);
  }

  return targets;
}

Token *consume(Parser *parser, TokenType expected) {
  Token *token = next_token(parser);
  if (!token || token->type != expected) {
    syntax_error("unexpected token", parser->lexer->filename, token);
  }
  return token;
}

ASTNode *parse_call(Parser *parser, ASTNode *callee) {
  ASTNode *node = node_new(parser, callee->token, CALL);
  node->call.func = callee;
  next_token(parser);
  node->call.args = parse_argument_list(parser);
  // next_token(parser);
  return node;
}

ASTNode *parse_while_statement(Parser *parser, ASTNode *while_node) {
  ASTNode *condition = parse_expression(parser, 0);
  while_node->ctrl_stmt.test = condition;
  while_node->ctrl_stmt.body =
      ASTNode_new_with_allocator(&parser->ast.allocator, 4);
  while_node->ctrl_stmt.orelse =
      ASTNode_new_with_allocator(&parser->ast.allocator, 4);
  next_token(parser);

  if (parser->next == NULL || parser->next->type != NEWLINE) {
    syntax_error("expected newline after ':' in 'while' statement",
                 parser->lexer->filename, parser->next);
    return NULL;
  }

  next_token(parser);
  while (next_token(parser) != NULL) {
    ASTNode *stmt = parse_statement(parser);

    if (stmt == NULL || stmt->type == END_BLOCK) {
      break;
    }

    ASTNode_add_last(&while_node->ctrl_stmt.body, stmt);
  }

  next_token(parser);
  if (parser->current && parser->current->type == KEYWORD &&
      strcmp(parser->current->lexeme, "else") == 0) {
    next_token(parser);
    if (parser->current == NULL || parser->current->type != COLON) {
      syntax_error("expected ':' after 'else'", parser->lexer->filename,
                   parser->current);
      return NULL;
    }

    if (parser->next == NULL || parser->next->type != NEWLINE) {
      syntax_error("expected newline after ':' in 'else' statement",
                   parser->lexer->filename, parser->next);
      return NULL;
    }

    // Parse the 'else' block
    next_token(parser);
    while (next_token(parser) != NULL) {
      ASTNode *stmt = parse_statement(parser);
      if (stmt == NULL || stmt->type == END_BLOCK)
        break;
      ASTNode_add_last(&while_node->ctrl_stmt.orelse, stmt);
    }
  }

  return while_node;
}

ASTNode *parse_if_statement(Parser *parser, ASTNode *if_node) {
  ASTNode *condition = parse_expression(parser, 0);
  if_node->ctrl_stmt.test = condition;
  if_node->ctrl_stmt.body =
      ASTNode_new_with_allocator(&parser->ast.allocator, 4);
  if_node->ctrl_stmt.orelse =
      ASTNode_new_with_allocator(&parser->ast.allocator, 4);
  next_token(parser);

  if (!parser->current || !parser->next || parser->current->type != COLON ||
      parser->next->type != NEWLINE) {
    syntax_error("expected newline after ':' in 'if' statement",
                 parser->lexer->filename, parser->next);
    return NULL;
  }

  next_token(parser);
  while (next_token(parser) != NULL) {
    ASTNode *stmt = parse_statement(parser);

    if (stmt == NULL || stmt->type == END_BLOCK) {
      break;
    }

    ASTNode_add_last(&if_node->ctrl_stmt.body, stmt);
  }

  next_token(parser);
  if (parser->current && parser->current->type == KEYWORD &&
      strcmp(parser->current->lexeme, "elif") == 0) {
    next_token(parser);
    ASTNode *elif_node = node_new(parser, parser->current, IF);
    ASTNode *parsed_elif = parse_if_statement(parser, elif_node);
    ASTNode_add_last(&if_node->ctrl_stmt.orelse, parsed_elif);
  } else if (parser->current && parser->current->type == KEYWORD &&
             strcmp(parser->current->lexeme, "else") == 0) {
    next_token(parser);

    if (parser->current == NULL || parser->current->type != COLON) {
      syntax_error("expected ':' after 'else'", parser->lexer->filename,
                   parser->current);
      return NULL;
    }

    // Expect NEWLINE
    next_token(parser);

    if (parser->current == NULL || parser->current->type != NEWLINE) {
      syntax_error("expected newline after ':' in 'else' statement",
                   parser->lexer->filename, parser->current);
      return NULL;
    }

    // Parse the 'else' block
    while (next_token(parser) != NULL) {
      ASTNode *stmt = parse_statement(parser);
      if (stmt == NULL || stmt->type == END_BLOCK)
        break;
      ASTNode_add_last(&if_node->ctrl_stmt.orelse, stmt);
    }
  }
  return if_node;
}

ASTNode *parse_function_declaration(Parser *parser, ASTNode *func_node) {
  Token *token = next_token(parser);
  if (token == NULL || token->type != IDENTIFIER) {
    syntax_error("expected function name after 'def'", parser->lexer->filename,
                 token);
    return NULL;
  }

  ASTNode *name_node = node_new(parser, token, VARIABLE);
  func_node->funcdef.name = name_node;
  func_node->funcdef.returns = NULL;

  token = next_token(parser);
  if (token == NULL || token->type != LPAR) {
    syntax_error("expected '(' after function name", parser->lexer->filename,
                 token);
    return NULL;
  }

  func_node->funcdef.params =
      ASTNode_new_with_allocator(&parser->ast.allocator, 4);

  // Parse parameters
  token = next_token(parser);
  while (token != NULL && token->type != RPAR) {
    if (token->type == IDENTIFIER) {
      ASTNode *param_node = node_new(parser, token, VARIABLE);

      if (parser->next && parser->next->type == COLON) {
        next_token(parser); // Consume identifier
        next_token(parser); // Consume COLON
        param_node->annotation = parse_expression(parser, 0);
      }

      ASTNode_add_last(&func_node->funcdef.params, param_node);
    } else if (token->type != COMMA) {
      syntax_error("expected parameter name or ','", parser->lexer->filename,
                   token);
      return NULL;
    }
    token = next_token(parser);
  }

  token = next_token(parser);
  if (token && token->type == RARROW) {
    next_token(parser);
    func_node->funcdef.returns = parse_expression(parser, 0);
    token = next_token(parser);
  }

  if (!token || token->type != COLON) {
    syntax_error("expected ':' after function parameters",
                 parser->lexer->filename, token);
    return NULL;
  }

  token = next_token(parser);
  func_node->funcdef.body =
      ASTNode_new_with_allocator(&parser->ast.allocator, 4);

  // Parse function body
  while ((token = next_token(parser)) != NULL) {
    ASTNode *stmt = parse_statement(parser);
    if (stmt == NULL || stmt->type == END_BLOCK)
      break;

    ASTNode_add_last(&func_node->funcdef.body, stmt);
  }

  return func_node;
}

ASTNode *parse_statement(Parser *parser) {
  Token *token = parser->current;
  if (token == NULL)
    return NULL;

  switch (token->type) {
  case NUMBER: {
    return parse_expression(parser, 0);
  }
  case IDENTIFIER: {
    if (parser->next && parser->next->type == COLON) {
      ASTNode *var = node_new(parser, token, VARIABLE);
      next_token(parser);
      next_token(parser);

      // Parse the type (e.g., "int", "List", etc.)
      var->annotation = parse_expression(parser, 0);

      if (parser->next && strcmp(parser->next->lexeme, "=") == 0) {
        next_token(parser); // Move to '='
        Token *assign_token = parser->current;
        next_token(parser); // Move to value
        ASTNode *value = parse_expression(parser, 0);
        ASTNode *assign_node = node_new(parser, assign_token, ASSIGNMENT);
        assign_node->assign.targets =
            ASTNode_new_with_allocator(&parser->ast.allocator, 1);
        ASTNode_add_last(&assign_node->assign.targets, var);
        assign_node->assign.value = value;
        return assign_node;
      }

      ASTNode *assign_node = node_new(parser, NULL, ASSIGNMENT);
      assign_node->assign.targets =
          ASTNode_new_with_allocator(&parser->ast.allocator, 1);
      ASTNode_add_last(&assign_node->assign.targets, var);
      assign_node->assign.value = NULL;
      return assign_node;
    }

    if (parser->next && is_augassign_op(parser->next->lexeme)) {
      ASTNode_LinkedList targets = parse_identifier_list(parser, token);
      ASTNode *node = node_new(parser, parser->current, AUG_ASSIGNMENT);
      next_token(parser);
      ASTNode *expr = parse_expression(parser, 0);
      ASTNode *target = ASTNode_pop(&targets);
      node->aug_assign =
          (AugAssign){.target = target, .op = parser->current, .value = expr};
      return node;
    }

    if (parser->next && (strcmp(parser->next->lexeme, "=") == 0 ||
                         strcmp(parser->next->lexeme, ",") == 0)) {
      ASTNode_LinkedList targets = parse_identifier_list(parser, token);
      ASTNode *node = node_new(parser, parser->current, ASSIGNMENT);
      next_token(parser);
      ASTNode *expr = parse_expression(parser, 0);
      node->assign = (Assign){.targets = targets, .value = expr};
      return node;
    }

    return parse_expression(parser, 0);
  } break;
  case KEYWORD: {
    if (strcmp(token->lexeme, "import") == 0) {
      ASTNode *node = node_new(parser, token, IMPORT);
      token = next_token(parser);
      node->import = parse_identifier_list(parser, token);
      return node;
    }

    if (strcmp(token->lexeme, "if") == 0) {
      ASTNode *node = node_new(parser, token, IF);
      token = next_token(parser);
      return parse_if_statement(parser, node);
    }

    if (strcmp(token->lexeme, "elif") == 0 ||
        strcmp(token->lexeme, "else") == 0) {
      // Signal end of current block - elif/else should be handled by parent if
      return node_new(parser, token, END_BLOCK);
    }

    if (strcmp(token->lexeme, "while") == 0) {
      ASTNode *node = node_new(parser, token, WHILE);
      token = next_token(parser);
      return parse_while_statement(parser, node);
    }

    if (strcmp(token->lexeme, "def") == 0) {
      ASTNode *node = node_new(parser, token, FUNCTION_DEF);
      return parse_function_declaration(parser, node);
    }

    if (strcmp(token->lexeme, "return") == 0) {
      ASTNode *node = node_new(parser, token, RETURN);
      next_token(parser);

      if (parser->next != NULL) {
        node->ret = parse_expression(parser, 0);
      } else {
        node->ret = NULL;
      }
      return node;
    }
  } break;
  case NEWLINE: {
    Token *next = parser->next;

    if (next->ident != token->ident) {
      return node_new(parser, token, END_BLOCK);
    }

    next_token(parser);
    return parse_statement(parser);
  } break;
  case LPAR:
    return parse_expression(parser, 0);
  default:
    break;
  }

  return NULL;
}

Parser parse(Lexer *lexer) {
  Parser parser = parser_new(lexer);

  while (next_token(&parser) != NULL) {
    ASTNode *stmt = parse_statement(&parser);
    if (stmt == NULL || stmt->type == END_BLOCK)
      break;
    ASTNode_add_last(&parser.ast, stmt);
  }

  return parser;
}

void parser_free(Parser *parser) {
  if (!parser)
    return;

  ASTNode_free(&parser->ast);
  Token_free(&parser->lexer->tokens);
  parser->lexer = NULL;
}
