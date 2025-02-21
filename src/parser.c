#include "parser.h"

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

ASTNode *node_new(Parser *parser, Token *token, NodeType type) {
  ASTNode *node = arena_alloc(&parser->ast.allocator, sizeof(ASTNode));
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
  default:
    break;
  }
  return root;
}

ASTNode *bin_op_new(Parser *parser, Token *operation, ASTNode *left,
                    ASTNode *right) {
  ASTNode *node = node_new(parser, operation, BINARY_OPERATION);
  node->bin_op = (BinaryOperation){.left = left, .right = right};
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

Token_ArrayList collect_expression(Token_ArrayList const *tokens, size_t from) {
  Token *token = Token_get(tokens, from);
  Token_ArrayList expression = Token_new(20);
  TokenType blacklist[] = {NEWLINE, ENDMARKER};
  size_t scope = token->ident;

  for (size_t j = from + 1;
       (!blacklist_tokens(token->type, blacklist, ARRAYSIZE(blacklist)) &&
        strcmp(token->lexeme, ":") != 0);
       ++j) {
    Token_push(&expression, token);
    token = Token_get(tokens, j);

    if (token->ident != scope) {
      break;
    }
  }

  return expression;
}

ASTNode_LinkedList parse_identifier_list(Parser *parser, Token *token,
                                         size_t capacity) {
  ASTNode_LinkedList targets = ASTNode_new(capacity);
  ASTNode *var = node_new(parser, token, VARIABLE);
  ASTNode_add_last(&targets, var);
  Token *next = next_token(parser->lexer);

  for (; next != NULL && next->type == COMMA;
       next = next_token(parser->lexer)) {
    if (token == NULL || token->type != IDENTIFIER) {
      trace_log(LOG_FATAL, "Syntax error: Expected identifier after comma");
    }

    token = next_token(parser->lexer);
    var = node_new(parser, token, VARIABLE);
    ASTNode_add_last(&targets, var);
  }

  return targets;
}

Token_ArrayList infix_to_postfix(Token_ArrayList *tokens) {
  Token_ArrayList stack = Token_new(10);
  Token_ArrayList postfix = Token_new(10);

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

  arena_free(&stack.allocator);
  arena_free(&tokens->allocator);
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
        ASTNode *node = bin_op_new(parser, token, left, right);
        ASTNode_add_last(&stack, node);
        continue;
      }
    } break;
    default:
      break;
    }
  }
  arena_free(&tokens->allocator);
  ASTNode *root = ASTNode_pop(&stack);
  ASTNode_free(&stack);
  return root;
}

ASTNode *parse_expression(Parser *parser) {
  Token_ArrayList expression =
      collect_expression(&parser->lexer->tokens, parser->lexer->token_idx - 1);
  parser->lexer->token_idx += expression.size;
  expression = infix_to_postfix(&expression);
  return shunting_yard(parser, &expression);
}

Parser parse(Lexer *lexer) {
  Parser parser = parser_new(lexer);
  Token *token = NULL;

  while ((token = next_token(parser.lexer)) != NULL) {
    switch (token->type) {
    case NUMBER: {
      ASTNode *expr = parse_expression(&parser);
      ASTNode_add_last(&parser.ast, expr);
    } break;
    case IDENTIFIER: {
      ASTNode_LinkedList targets = parse_identifier_list(&parser, token, 1);
      Token *next =
          Token_get(&parser.lexer->tokens, parser.lexer->token_idx - 1);

      if (next != NULL && strcmp(next->lexeme, "=") == 0) {
        parser.lexer->token_idx++;
        ASTNode *node = node_new(&parser, next, ASSIGNMENT);
        ASTNode *expr = parse_expression(&parser);
        node->assign = (Assign){.targets = targets, .value = expr};
        ASTNode_add_last(&parser.ast, node);
        continue;
      }

    } break;
    case KEYWORD: {
      if (strcmp(token->lexeme, "import") == 0) {
        ASTNode *node = node_new(&parser, token, IMPORT);
        token = next_token(parser.lexer);
        node->import = parse_identifier_list(&parser, token, 5);
        ASTNode_add_last(&parser.ast, node);
      }
    } break;
    default:
      break;
    }
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