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

Token_ArrayList infix_to_postfix(Token_ArrayList *tokens) {
  Token_ArrayList stack = Token_new(10);
  Token_ArrayList postfix = Token_new(10);

  for (size_t i = 0; i < tokens->size; i++) {
    Token *token = Token_get(tokens, i);

    if (token->type == IDENTIFIER || token->type == NUMERIC) {
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
    case TEXT:
    case NUMERIC: {
      ASTNode *literal = node_new(parser, token, LITERAL);
      ASTNode_add_last(&stack, literal);
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
    case NUMERIC: {
      ASTNode *expr = parse_expression(&parser);
      ASTNode_add_last(&parser.ast, expr);
    } break;
    case IDENTIFIER: {
      Token *next = next_token(parser.lexer);

      if (strcmp(next->lexeme, "=") == 0) {
        parser.lexer->token_idx++;
        ASTNode *node = node_new(&parser, next, ASSIGNMENT);
        ASTNode *expr = parse_expression(&parser);
        node->assign = (Assign){.targets = ASTNode_new(1), .value = expr};
        ASTNode *var = node_new(&parser, token, VARIABLE);
        ASTNode_add_last(&node->assign.targets, var);
        ASTNode_add_last(&parser.ast, node);
        continue;
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