#include "parser.h"

// TODO: Replace strcmp with proper token type checks where possible (e.g., for
// keywords)

const char *COMPARISON_OPERATORS[] = {"==", "!=", ">", "<", ">=", "<="};

const char *AUG_ASSIGN_OPS[] = {"+=", "-=", "*=",  "@=",  "/=",  "%=", "&=",
                                "|=", "^=", "<<=", ">>=", "**=", "//="};

Token *advance(Parser *parser) {
  if (parser->lexer.token_idx >= parser->lexer.tokens.size) {
    parser->current = NULL;
    parser->next = NULL;
    return NULL;
  }

  parser->current = Token_get(&parser->lexer.tokens, parser->lexer.token_idx);
  parser->lexer.token_idx++;

  if (parser->lexer.token_idx < parser->lexer.tokens.size) {
    parser->next = Token_get(&parser->lexer.tokens, parser->lexer.token_idx);
  } else {
    parser->next = NULL;
  }

  return parser->current;
}

ASTNode *parse_statement(Parser *parser);

ASTNode *parse_if_statement(Parser *parser, ASTNode *if_node);

ASTNode *parse_expression(Parser *parser, int8_t min_precedence);

ASTNode *parse_call(Parser *parser, ASTNode *callee);

ASTNode *parse_class_def(Parser *parser, ASTNode *class_node);

ASTNode *parse_match_stmt(Parser *parser);

bool blacklist_tokens(TokenType type, const TokenType blacklist[], size_t size);

ASTNode *bin_op_new(Parser *parser, Token *operation, ASTNode *left,
                    ASTNode *right);

bool is_python_main_check(ASTNode *node);

static inline Parser parser_new(Lexer *lexer) {
  return (Parser){.lexer = *lexer,
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

const char *ctx_to_str(Context ctx) {
  switch (ctx) {
  case STORE:
    return "STORE";
  case LOAD:
    return "LOAD";
  case DEL:
    return "DEL";
  default:
    UNREACHABLE("ctx_to_str");
    break;
  }
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
  case CLASS_DEF:
    return "CLASS DEF";
  case RETURN:
    return "RETURN";
  case CALL:
    return "CALL";
  case ATTRIBUTE:
    return "ATTRIBUTE";
  case MATCH:
    return "MATCH";
  case CASE:
    return "CASE";
  case TUPLE:
    return "TUPLE";
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
  node->child = NULL;
  node->parent = NULL;
  node->ctx = LOAD;
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
  cJSON_AddStringToObject(root, "ctx", ctx_to_str(node->ctx));

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
  case ATTRIBUTE:
    cJSON_AddItemToObject(root, "value", serialize_node(node->attribute.value));
    cJSON_AddStringToObject(root, "attr", node->attribute.attr);
    break;
  case VARIABLE:
  case LITERAL:
    cJSON_AddItemToObject(root, "token", serialize_token(node->token));
    // cJSON_AddStringToObject(root, "ctx", ctx_to_str(node->ctx));
    if (node->child) {
      cJSON_AddItemToObject(root, "annotation", serialize_node(node->child));
    }
    break;
  case BINARY_OPERATION:
    cJSON_AddItemToObject(root, "token", serialize_token(node->token));
    cJSON_AddItemToObject(root, "left", serialize_node(node->bin_op.left));
    cJSON_AddItemToObject(root, "right", serialize_node(node->bin_op.right));
    break;
  case IMPORT:
    cJSON_AddItemToObject(root, "token", serialize_token(node->token));
    cJSON_AddItemToObject(root, "names", serialize_program(&node->collection));
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
  case CLASS_DEF:
    cJSON_AddItemToObject(root, "name", serialize_node(node->def.name));
    cJSON_AddItemToObject(root, "params", serialize_program(&node->def.params));
    cJSON_AddItemToObject(root, "body", serialize_program(&node->def.body));
    break;
  case RETURN:
    cJSON_AddItemToObject(root, "token", serialize_token(node->token));
    cJSON_AddItemToObject(root, "ret", serialize_node(node->child));
    break;
  case CALL:
    cJSON_AddItemToObject(root, "token", serialize_token(node->token));
    cJSON_AddItemToObject(root, "args", serialize_program(&node->call.args));
    break;
  case MATCH:
  case CASE:
    cJSON_AddItemToObject(root, "token", serialize_token(node->token));
    cJSON_AddItemToObject(root, "test", serialize_node(node->ctrl_stmt.test));
    cJSON_AddItemToObject(root, "body",
                          serialize_program(&node->ctrl_stmt.body));
    break;
  case TUPLE:
  case LIST_EXPR:
    cJSON_AddItemToObject(root, "token", serialize_token(node->token));
    cJSON_AddItemToObject(root, "elements",
                          serialize_program(&node->collection));
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

  if (strcmp(op, ".") == 0)
    return 80;

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

ASTNode *parse_assign(Parser *parser, ASTNode *target) {
  advance(parser); // Move to '='
  Token *assign_token = parser->current;
  advance(parser); // Move to value
  ASTNode *value = parse_expression(parser, 0);
  ASTNode *assign_node = node_new(parser, assign_token, ASSIGNMENT);
  assign_node->ctx = STORE;
  assign_node->assign.targets =
      ASTNode_new_with_allocator(&parser->ast.allocator, 1);
  target->ctx = STORE;
  target->parent = assign_node;
  ASTNode_add_last(&assign_node->assign.targets, target);
  assign_node->assign.value = value;
  return assign_node;
}

ASTNode *parse_attribute(Parser *parser, ASTNode *left) {
  advance(parser);
  ASTNode *node = node_new(parser, parser->current, ATTRIBUTE);
  node->attribute.value = left;
  node->attribute.attr =
      arena_strdup(&parser->ast.allocator.base, parser->current->lexeme);
  if (strcmp(parser->next->lexeme, "=") == 0) {
    ASTNode *assign = parse_assign(parser, node);
    node->parent = assign;
    return assign;
  }
  node->ctx = LOAD;
  return node;
}

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
    advance(parser);

    // Check for empty tuple: ()
    if (parser->current && parser->current->type == RPAR) {
      ASTNode *tuple_node = node_new(parser, token, TUPLE);
      tuple_node->collection =
          ASTNode_new_with_allocator(&parser->ast.allocator, 0);
      return tuple_node;
    }

    ASTNode *first_expr = parse_expression(parser, 0);

    // Check if there's a comma after the first expression (indicates tuple)
    if (parser->next && parser->next->type == COMMA) {
      ASTNode *tuple_node = node_new(parser, token, TUPLE);
      tuple_node->collection =
          ASTNode_new_with_allocator(&parser->ast.allocator, 4);
      ASTNode_add_last(&tuple_node->collection, first_expr);

      // Parse remaining tuple elements
      while (parser->next && parser->next->type == COMMA) {
        advance(parser); // consume comma
        advance(parser); // move to next expression

        // Check for trailing comma: (1, 2,)
        if (parser->current->type == RPAR) {
          break;
        }

        ASTNode *elem = parse_expression(parser, 0);
        ASTNode_add_last(&tuple_node->collection, elem);
      }

      advance(parser); // consume RPAR
      return tuple_node;
    }

    // Not a tuple, just a parenthesized expression
    advance(parser);
    return first_expr;
  }
  case KEYWORD:
  case OPERATOR:
    if (is_prefix_operator(token)) {
      advance(parser);
      ASTNode *node = node_new(parser, token, UNARY_OPERATION);
      int8_t rbp = get_prefix_precedence(token->lexeme);
      node->bin_op.right = parse_expression(parser, rbp);
      return node;
    }

    if (strcmp(token->lexeme, "None") == 0) {
      return node_new(parser, token, LITERAL);
    }
    break;

  default:
    break;
  }

  syntax_error(
      "expected start of expression (literal, variable, or prefix operator)",
      parser->lexer.filename, token);
  return NULL;
}

ASTNode *led(Parser *parser, ASTNode *left) {
  Token *op_token = parser->current;
  if (op_token->type == LPAR) {
    return parse_call(parser, left);
  }

  if (strcmp(op_token->lexeme, ".") == 0) {
    return parse_attribute(parser, left);
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

    advance(parser);
    right = parse_expression(parser, lbp + 1);
    ASTNode_add_last(&comp->compare.comparators, right);
    Token_push(&comp->compare.ops, op_token);
    return comp;
  }
  // Regular binary operation
  advance(parser);
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

    advance(parser);
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

  advance(parser);

  do {
    ASTNode *arg = parse_expression(parser, 0);
    ASTNode_add_last(&args, arg);
    advance(parser);

    // Check the token immediately after the expression
    if (parser->current == NULL)
      break;

    if (parser->current->type == COMMA) {
      advance(parser); // Consume COMMA
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

ASTNode_LinkedList parse_identifier_list(Parser *parser, Token *token,
                                         Context ctx) {
  ASTNode_LinkedList targets =
      ASTNode_new_with_allocator(&parser->ast.allocator, 1);
  ASTNode *var = node_new(parser, token, VARIABLE);
  var->ctx = ctx;
  ASTNode_add_last(&targets, var);
  Token *next = advance(parser);

  for (; next != NULL && next->type == COMMA; next = advance(parser)) {
    if (token == NULL || token->type != IDENTIFIER) {
      syntax_error("expected identifier after comma", parser->lexer.filename,
                   token);
    }

    token = advance(parser);
    var = node_new(parser, token, VARIABLE);
    var->ctx = ctx;
    ASTNode_add_last(&targets, var);
  }

  return targets;
}

Token *consume(Parser *parser, TokenType expected) {
  Token *token = advance(parser);
  if (!token || token->type != expected) {
    syntax_error("unexpected token", parser->lexer.filename, token);
  }
  return token;
}

ASTNode *parse_call(Parser *parser, ASTNode *callee) {
  ASTNode *node = node_new(parser, callee->token, CALL);
  node->call.func = callee;
  advance(parser);
  node->call.args = parse_argument_list(parser);
  return node;
}

ASTNode *parse_while_statement(Parser *parser, ASTNode *while_node) {
  ASTNode *condition = parse_expression(parser, 0);
  while_node->ctrl_stmt.test = condition;
  while_node->ctrl_stmt.body =
      ASTNode_new_with_allocator(&parser->ast.allocator, 4);
  while_node->ctrl_stmt.orelse =
      ASTNode_new_with_allocator(&parser->ast.allocator, 4);
  advance(parser);

  if (parser->next == NULL || parser->next->type != NEWLINE) {
    syntax_error("expected newline after ':' in 'while' statement",
                 parser->lexer.filename, parser->next);
    return NULL;
  }

  advance(parser);
  while (advance(parser) != NULL) {
    ASTNode *stmt = parse_statement(parser);

    if (stmt == NULL || stmt->type == END_BLOCK) {
      break;
    }

    ASTNode_add_last(&while_node->ctrl_stmt.body, stmt);
  }

  advance(parser);
  if (parser->current && parser->current->type == KEYWORD &&
      strcmp(parser->current->lexeme, "else") == 0) {
    advance(parser);
    if (parser->current == NULL || parser->current->type != COLON) {
      syntax_error("expected ':' after 'else'", parser->lexer.filename,
                   parser->current);
      return NULL;
    }

    if (parser->next == NULL || parser->next->type != NEWLINE) {
      syntax_error("expected newline after ':' in 'else' statement",
                   parser->lexer.filename, parser->next);
      return NULL;
    }

    // Parse the 'else' block
    advance(parser);
    while (advance(parser) != NULL) {
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
  advance(parser);

  if (!parser->current || !parser->next || parser->current->type != COLON ||
      parser->next->type != NEWLINE) {
    syntax_error("expected newline after ':' in 'if' statement",
                 parser->lexer.filename, parser->next);
    return NULL;
  }

  advance(parser);
  while (advance(parser) != NULL) {
    ASTNode *stmt = parse_statement(parser);

    if (stmt == NULL || stmt->type == END_BLOCK) {
      break;
    }

    ASTNode_add_last(&if_node->ctrl_stmt.body, stmt);
  }

  advance(parser);
  if (parser->current && parser->current->type == KEYWORD &&
      strcmp(parser->current->lexeme, "elif") == 0) {
    advance(parser);
    ASTNode *elif_node = node_new(parser, parser->current, IF);
    ASTNode *parsed_elif = parse_if_statement(parser, elif_node);
    ASTNode_add_last(&if_node->ctrl_stmt.orelse, parsed_elif);
  } else if (parser->current && parser->current->type == KEYWORD &&
             strcmp(parser->current->lexeme, "else") == 0) {
    advance(parser);

    if (parser->current == NULL || parser->current->type != COLON) {
      syntax_error("expected ':' after 'else'", parser->lexer.filename,
                   parser->current);
      return NULL;
    }

    // Expect NEWLINE
    advance(parser);

    if (parser->current == NULL || parser->current->type != NEWLINE) {
      syntax_error("expected newline after ':' in 'else' statement",
                   parser->lexer.filename, parser->current);
      return NULL;
    }

    // Parse the 'else' block
    while (advance(parser) != NULL) {
      ASTNode *stmt = parse_statement(parser);
      if (stmt == NULL || stmt->type == END_BLOCK)
        break;
      ASTNode_add_last(&if_node->ctrl_stmt.orelse, stmt);
    }
  }
  return if_node;
}

ASTNode *parse_function_def(Parser *parser, ASTNode *func_node) {
  Token *token = advance(parser);
  if (token == NULL || token->type != IDENTIFIER) {
    syntax_error("expected function name after 'def'", parser->lexer.filename,
                 token);
    return NULL;
  }

  ASTNode *name_node = node_new(parser, token, VARIABLE);
  func_node->def.name = name_node;
  func_node->def.returns = NULL;

  token = advance(parser);
  if (token == NULL || token->type != LPAR) {
    syntax_error("expected '(' after function name", parser->lexer.filename,
                 token);
    return NULL;
  }

  func_node->def.params = ASTNode_new_with_allocator(&parser->ast.allocator, 4);

  // Parse parameters
  token = advance(parser);
  while (token != NULL && token->type != RPAR) {
    if (token->type == IDENTIFIER) {
      ASTNode *param_node = node_new(parser, token, VARIABLE);

      if (parser->next && parser->next->type == COLON) {
        advance(parser); // Consume identifier
        advance(parser); // Consume COLON
        param_node->child = parse_expression(parser, 0);
      }
      param_node->parent = func_node;
      ASTNode_add_last(&func_node->def.params, param_node);
    } else if (token->type != COMMA) {
      syntax_error("expected parameter name or ','", parser->lexer.filename,
                   token);
      return NULL;
    }
    token = advance(parser);
  }

  token = advance(parser);
  if (token && token->type == RARROW) {
    advance(parser);
    func_node->def.returns = parse_expression(parser, 0);
    func_node->def.returns->parent = func_node;
    token = advance(parser);
  }

  if (!token || token->type != COLON) {
    syntax_error("expected ':' after function parameters",
                 parser->lexer.filename, token);
    return NULL;
  }

  token = advance(parser);
  func_node->def.body = ASTNode_new_with_allocator(&parser->ast.allocator, 4);

  // Parse function body
  while ((token = advance(parser)) != NULL) {
    ASTNode *stmt = parse_statement(parser);
    if (stmt == NULL || stmt->type == END_BLOCK)
      break;

    stmt->parent = func_node;
    ASTNode_add_last(&func_node->def.body, stmt);
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
      advance(parser);
      advance(parser);

      // Parse the type (e.g., "int", "List", etc.)
      var->child = parse_expression(parser, 0);

      if (parser->next && strcmp(parser->next->lexeme, "=") == 0) {
        return parse_assign(parser, var);
      }

      return var;
    }

    if (parser->next && is_augassign_op(parser->next->lexeme)) {
      ASTNode_LinkedList targets = parse_identifier_list(parser, token, STORE);
      ASTNode *node = node_new(parser, parser->current, AUG_ASSIGNMENT);
      advance(parser);
      ASTNode *expr = parse_expression(parser, 0);
      ASTNode *target = ASTNode_pop(&targets);
      node->aug_assign =
          (AugAssign){.target = target, .op = parser->current, .value = expr};
      return node;
    }

    if (parser->next && (strcmp(parser->next->lexeme, "=") == 0 ||
                         strcmp(parser->next->lexeme, ",") == 0)) {
      ASTNode_LinkedList targets = parse_identifier_list(parser, token, STORE);
      ASTNode *node = node_new(parser, parser->current, ASSIGNMENT);
      advance(parser);
      ASTNode *expr = parse_expression(parser, 0);
      node->assign = (Assign){.targets = targets, .value = expr};
      node->ctx = STORE;
      return node;
    }

    return parse_expression(parser, 0);
  } break;
  case KEYWORD: {
    if (strcmp(token->lexeme, "import") == 0) {
      ASTNode *node = node_new(parser, token, IMPORT);
      token = advance(parser);
      node->collection = parse_identifier_list(parser, token, LOAD);
      return node;
    }

    if (strcmp(token->lexeme, "if") == 0) {
      ASTNode *node = node_new(parser, token, IF);
      token = advance(parser);
      return parse_if_statement(parser, node);
    }

    if (strcmp(token->lexeme, "elif") == 0 ||
        strcmp(token->lexeme, "else") == 0) {
      // Signal end of current block - elif/else should be handled by parent if
      return node_new(parser, token, END_BLOCK);
    }

    if (strcmp(token->lexeme, "while") == 0) {
      ASTNode *node = node_new(parser, token, WHILE);
      token = advance(parser);
      return parse_while_statement(parser, node);
    }

    if (strcmp(token->lexeme, "def") == 0) {
      ASTNode *node = node_new(parser, token, FUNCTION_DEF);
      return parse_function_def(parser, node);
    }

    if (strcmp(token->lexeme, "class") == 0) {
      ASTNode *node = node_new(parser, token, CLASS_DEF);
      node->parent = NULL;
      return parse_class_def(parser, node);
    }

    if (strcmp(token->lexeme, "return") == 0) {
      ASTNode *node = node_new(parser, token, RETURN);
      advance(parser);

      if (parser->next != NULL) {
        node->child = parse_expression(parser, 0);
      } else {
        node->child = NULL;
      }
      return node;
    }

    if (strcmp(token->lexeme, "match") == 0) {
      return parse_match_stmt(parser);
    }
  } break;
  case NEWLINE: {
    while (parser->next && parser->next->type == NEWLINE) {
      advance(parser);
    }

    if (parser->next->ident != token->ident) {
      return node_new(parser, token, END_BLOCK);
    }

    advance(parser);
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
  // Create a synthetic main node to collect unbound logic
  Token *main_tok = create_token_from_str(&parser.lexer, "main", IDENTIFIER);
  Token *def_tok = create_token_from_str(&parser.lexer, "def", KEYWORD);
  ASTNode *name = node_new(&parser, main_tok, VARIABLE);
  ASTNode *synthetic_main = node_new(&parser, def_tok, FUNCTION_DEF);
  synthetic_main->def.name = name;
  synthetic_main->def.returns = NULL;
  synthetic_main->def.body =
      ASTNode_new_with_allocator(&parser.ast.allocator, 4);
  synthetic_main->def.params =
      ASTNode_new_with_allocator(&parser.ast.allocator, 2);
  bool explicit_main_found = false;
  // ---

  while (advance(&parser) != NULL) {
    ASTNode *stmt = parse_statement(&parser);

    if (!stmt)
      break;

    if (stmt->type == FUNCTION_DEF || stmt->type == CLASS_DEF ||
        stmt->type == IMPORT) { // TODO: This should be a function
      ASTNode_add_last(&parser.ast, stmt);
    } else if (stmt->type == ASSIGNMENT && !stmt->parent) {
      ASTNode_add_last(&parser.ast, stmt);
    } else if (is_python_main_check(stmt)) {
      // Found 'if __name__ == "__main__":'
      explicit_main_found = true;
      ASTNode_add_last(&parser.ast, stmt);
    } else {
      // Unbound executable statements (calls, etc) move to synthetic main
      ASTNode_add_last(&synthetic_main->def.body, stmt);
    }
  }

  // Finalization: If we have unbound code and no explicit main, add our
  // synthetic one
  if (!explicit_main_found && synthetic_main->def.body.size > 0) {
    ASTNode_add_last(&parser.ast, synthetic_main);
  }

  return parser;
}

void parser_free(Parser *parser) {
  if (!parser)
    return;

  ASTNode_free(&parser->ast);
  Token_free(&parser->lexer.tokens);
}

ASTNode *parse_class_def(Parser *parser, ASTNode *class_node) {
  // 1. Consume Class Name
  Token *token = advance(parser);
  if (token == NULL || token->type != IDENTIFIER) {
    syntax_error("expected class name after 'class'", parser->lexer.filename,
                 token);
    return NULL;
  }

  class_node->def.name = node_new(parser, token, VARIABLE);
  // Initialize lists
  class_node->def.params =
      ASTNode_new_with_allocator(&parser->ast.allocator, 2);
  class_node->def.body = ASTNode_new_with_allocator(&parser->ast.allocator, 4);
  class_node->def.returns = NULL;

  // 2. Handle Inheritance: class Dog(Animal):
  if (parser->next && parser->next->type == LPAR) {
    advance(parser); // Consume '('

    while (parser->next && parser->next->type != RPAR) {
      advance(parser);
      ASTNode *base = parse_expression(parser, 0);
      base->parent = class_node;
      ASTNode_add_last(&class_node->def.params, base);

      if (parser->next && parser->next->type == COMMA) {
        advance(parser); // Consume ','
      } else {
        break;
      }
    }
    consume(parser, RPAR);
  }

  // 3. Consume Colon
  token = advance(parser);
  if (!token || token->type != COLON) {
    syntax_error("expected ':' after class definition", parser->lexer.filename,
                 token);
    return NULL;
  }

  // 4. Parse Class Body
  // Expect a NEWLINE then increased indentation
  advance(parser);
  while ((token = advance(parser)) != NULL) {
    ASTNode *stmt = parse_statement(parser);
    // If parse_statement hits a NEWLINE with less indentation, it returns
    // END_BLOCK
    if (stmt == NULL || stmt->type == END_BLOCK)
      break;

    if (stmt->type == VARIABLE)
      stmt->ctx = STORE;
    stmt->parent = class_node;
    ASTNode_add_last(&class_node->def.body, stmt);
  }

  return class_node;
}

bool is_python_main_check(ASTNode *node) {
  if (node->type != IF || !node->ctrl_stmt.test)
    return false;
  ASTNode *test = node->ctrl_stmt.test;
  // Look for: VARIABLE(__name__) == LITERAL("__main__")
  if (test->type == COMPARE && test->compare.left->type == VARIABLE) {
    if (strcmp(test->compare.left->token->lexeme, "__name__") == 0) {
      ASTNode *first_comp =
          test->compare.comparators.elements[test->compare.comparators.head]
              .data;
      if (first_comp->type == LITERAL &&
          strcmp(first_comp->token->lexeme, "\"__main__\"") == 0) {
        return true;
      }
    }
  }
  return false;
}

ASTNode *parse_match_stmt(Parser *parser) {
  ASTNode *node = node_new(parser, parser->current, MATCH);
  advance(parser); // Consume 'match' keyword
  node->ctrl_stmt.test = parse_expression(parser, 0);
  node->ctrl_stmt.body = ASTNode_new_with_allocator(&parser->ast.allocator, 4);
  advance(parser);
  advance(parser); // Consume ':'
  advance(parser); // Consume NEWLINE

  while (parser->current && parser->current->lexeme &&
         strcmp(parser->current->lexeme, "case") == 0) {
    ASTNode *case_node = node_new(parser, parser->current, CASE);
    advance(parser); // Consume 'case' keyword
    case_node->ctrl_stmt.test = parse_expression(parser, 0);
    case_node->ctrl_stmt.body =
        ASTNode_new_with_allocator(&parser->ast.allocator, 4);
    advance(parser); // Consume ':'
    advance(parser); // Consume NEWLINE

    while (parser->current && strcmp(parser->current->lexeme, "case") != 0) {
      ASTNode *stmt = parse_statement(parser);

      if (stmt != NULL && stmt->type != END_BLOCK) {
        stmt->parent = case_node;
        ASTNode_add_last(&case_node->ctrl_stmt.body, stmt);
      }

      advance(parser);
    }

    ASTNode_add_last(&node->ctrl_stmt.body, case_node);
  }

  return node;
}