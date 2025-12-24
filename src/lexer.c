#include "lexer.h"
#define NUM_KEYWORDS 35
#define LEX_CAP 32

const char *PYTHON_KEYWORD[NUM_KEYWORDS] = {
    "False",  "None",   "True",    "and",      "as",       "assert", "async",
    "await",  "break",  "class",   "continue", "def",      "del",    "elif",
    "else",   "except", "finally", "for",      "from",     "global", "if",
    "import", "in",     "is",      "lambda",   "nonlocal", "not",    "or",
    "pass",   "raise",  "return",  "try",      "while",    "with",   "yield"};

const char OPERATORS[] = {'+', '-', '*', '/', '%', '>', '<',
                          '!', '=', '&', '|', '^', '~', '.'};

const char *EXTENDED_OPERATORS[] = {
    "//", "==", "!=", "**", ">=",  "<=",  "&&", "||", "+=",
    "-=", "*=", "/=", "%=", "//=", "**=", "<<", ">>"};

const char DELIMITERS[] = {'(', ')', '{', '}', ',', ';', '.', ':', '`'};

Token *create_token_from_char(Lexer *lexer, char character, TokenType type);

Token *create_operator_token(Lexer *lexer, const char *matchedOperator);

Token *create_EOF_token(Lexer *lexer);

Token *create_number_token(Lexer *lexer, char character);

Token *create_string_token(Lexer *lexer, char character);

Token *create_keyword_token(Lexer *lexer, char character);

Token *create_newline_token(Lexer *lexer);

const char *token_type_to_string(TokenType type) {
  switch (type) {
  case KEYWORD:
    return "KEYWORD";
  case OPERATOR:
    return "OPERATOR";
  case STRING:
    return "STRING";
  case NUMBER:
    return "NUMBER";
  case IDENTIFIER:
    return "IDENTIFIER";
  case LPAR:
    return "LPAR";
  case RPAR:
    return "RPAR";
  case NEWLINE:
    return "NEWLINE";
  case ENDMARKER:
    return "ENDMARKER";
  case LSQB:
    return "LSQB";
  case RSQB:
    return "RSQB";
  case COMMA:
    return "COMMA";
  case COLON:
    return "COLON";
  default:
    return "UNKNOWN";
  }
}

cJSON *serialize_token(Token *token) {
  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "type", token_type_to_string(token->type));
  cJSON_AddStringToObject(root, "lexeme", token->lexeme);
  cJSON_AddNumberToObject(root, "line", token->line);
  cJSON_AddNumberToObject(root, "col", token->col);
  cJSON_AddNumberToObject(root, "ident", token->ident);
  return root;
}

Lexer lexer_new(const char *source, const char *filename) {
  size_t len = strlen(source);
  return (Lexer){.source = source,
                 .filename = filename,
                 .position = 0,
                 .token_idx = 0,
                 .source_length = len,
                 .tokens = Token_new(100)};
}

Lexer tokenize(const char *source, const char *filename) {
  ASSERT(source != NULL, "Source file was not provided");
  Lexer lexer = lexer_new(source, filename);
  size_t line = 1;
  size_t column = 1;
  size_t ident = 0;

  while (lexer.position < lexer.source_length) {
    char character = lexer.source[lexer.position];
    char prev = lexer.position > 0 ? lexer.source[lexer.position - 1] : ' ';
    size_t spaces = 0;

    while (character == ' ' || character == '\t') {
      if (character == ' ')
        spaces++;
      else
        spaces += 4;
      column++;
      spaces += column;
      character = lexer.source[++lexer.position];
    }

    size_t new_ident = spaces / 4;
    ident = prev == '\n' && new_ident != ident ? new_ident : ident;

    if (character == '#') {
      while ('\n' != lexer.source[++lexer.position])
        continue;
      line++;
      column = 1;
      continue;
    }

    Token *token = NULL;
    const char *matched_operator = NULL;
    for (size_t i = 0; i < ARRAYSIZE(OPERATORS); i++) {
      if (character == OPERATORS[i]) {
        matched_operator = &character;
        break;
      }
    }

    switch (character) {
    case '(':
      token = create_token_from_char(&lexer, character, LPAR);
      break;
    case ')':
      token = create_token_from_char(&lexer, character, RPAR);
      break;
    case ',':
      token = create_token_from_char(&lexer, character, COMMA);
      break;
    case ':':
      token = create_token_from_char(&lexer, character, COLON);
      break;
    case '[':
      token = create_token_from_char(&lexer, character, LSQB);
      break;
    case ']':
      token = create_token_from_char(&lexer, character, RSQB);
      break;
    default:
      break;
    }

    if (token != NULL) {
      token->col = column;
      token->line = line;
      token->ident = ident;
      Token_push(&lexer.tokens, token);
      continue;
    }

    if (matched_operator != NULL) {
      // Build the operator lexeme
      token = create_operator_token(&lexer, matched_operator);
      token->col = column;
      token->line = line;
      token->ident = ident;
      Token_push(&lexer.tokens, token);
      continue;
    }

    if (isdigit(character)) {
      token = create_number_token(&lexer, character);
      token->col = column;
      token->line = line;
      token->ident = ident;
      Token_push(&lexer.tokens, token);
      continue;
    }

    if (character == '\'' || character == '\"') {
      token = create_string_token(&lexer, character);
      token->col = column;
      token->line = line;
      token->ident = ident;
      Token_push(&lexer.tokens, token);
      continue;
    }

    if (isalpha(character) || character == '_') {
      token = create_keyword_token(&lexer, character);
      token->col = column;
      token->line = line;
      token->ident = ident;
      Token_push(&lexer.tokens, token);
      continue;
    }

    if (character == '\n') {
      token = create_newline_token(&lexer);
      token->col = column;
      token->line = line;
      token->ident = ident;
      Token_push(&lexer.tokens, token);
      line++;
      column = 0;
    }

    lexer.position++;
  }

  Token *eof = create_EOF_token(&lexer);
  eof->col = column;
  eof->line = line;
  eof->ident = 0;
  Token_push(&lexer.tokens, eof);
  return lexer;
}

Token *create_token_from_char(Lexer *lexer, char character, TokenType type) {
  Token *token = allocator_alloc(&lexer->tokens.allocator, sizeof(Token));
  if (token == NULL) {
    slog_error("Could not allocate memory for token");
    return NULL;
  }

  char *lexeme = allocator_alloc(&lexer->tokens.allocator, sizeof(char) * 2);
  if (lexeme == NULL) {
    slog_error("Failed to allocate memory for lexeme");
    return NULL;
  }

  lexeme[0] = character;
  lexeme[1] = '\0';
  token->lexeme = lexeme;
  token->type = type;
  lexer->position++;
  return token;
}

Token *create_operator_token(Lexer *lexer, const char *matched_operator) {
  size_t max_lexeme_length = 1;
  for (size_t i = 0; i < ARRAYSIZE(EXTENDED_OPERATORS); i++) {
    const char *operatorStr = EXTENDED_OPERATORS[i];
    size_t operatorLength = strlen(operatorStr);

    if (strncmp(&lexer->source[lexer->position], operatorStr, operatorLength) ==
            0 &&
        operatorLength > max_lexeme_length) {
      max_lexeme_length = operatorLength;
      matched_operator = operatorStr;
    }
  }

  size_t lexeme_size = max_lexeme_length * sizeof(char) + 1;
  char *lexeme = allocator_alloc(&lexer->tokens.allocator, lexeme_size);
  if (lexeme == NULL) {
    slog_error("Failed to allocate memory for lexeme");
    return NULL;
  }

  safe_memcpy(lexeme, max_lexeme_length * sizeof(char) + 1, matched_operator,
              max_lexeme_length);
  lexeme[max_lexeme_length] = '\0';
  lexer->position += max_lexeme_length;

  Token *token = allocator_alloc(&lexer->tokens.allocator, sizeof(Token));
  if (token == NULL) {
    slog_error("Failed to allocate memory for token");
    return NULL;
  }

  token->type = OPERATOR;
  token->lexeme = lexeme;
  return token;
}

Token *create_EOF_token(Lexer *lexer) {
  Token *token = allocator_alloc(&lexer->tokens.allocator, sizeof(Token));
  if (token == NULL) {
    slog_error("Failed to allocate memory for EOF token");
    return NULL;
  }

  token->type = ENDMARKER;
  token->lexeme = arena_strdup(&lexer->tokens.allocator.base, "EOF");
  return token;
}

Token *create_number_token(Lexer *lexer, char character) {
  char *lexeme =
      allocator_alloc(&lexer->tokens.allocator, LEX_CAP * sizeof(char));
  if (lexeme == NULL) {
    slog_error("Failed to allocate memory for lexeme");
    return NULL;
  }
  size_t lexeme_length = 0;
  lexeme[lexeme_length++] = character;
  lexer->position++;

#ifdef __GNUC__
#pragma GCC unroll 100
#endif
  while (lexer->position < lexer->source_length) {
    character = lexer->source[lexer->position];
    if (isdigit(character) || character == '_') {
      lexeme[lexeme_length++] = character;
      lexer->position++;
    } else if (character == '.') {
      lexeme[lexeme_length++] = character;
      lexer->position++;
    } else {
      break;
    }
  }
  lexeme[lexeme_length] = '\0';
  Token *token = allocator_alloc(&lexer->tokens.allocator, sizeof(Token));
  if (token == NULL) {
    slog_error("Failed to allocate memory for token");
    return NULL;
  }

  token->type = NUMBER;
  token->lexeme = lexeme;
  return token;
}

Token *create_string_token(Lexer *lexer, char character) {
  size_t start = lexer->position + 1;
  lexer->position++;

#ifdef __GNUC__
#pragma GCC unroll 100
#endif
  while (lexer->source[lexer->position] != character) {
    lexer->position++;
  }

  char *lexeme =
      slice(&lexer->tokens.allocator, lexer->source, start, lexer->position);
  Token *token = allocator_alloc(&lexer->tokens.allocator, sizeof(Token));

  if (token == NULL) {
    slog_error("Failed to allocate memory for token");
    return NULL;
  }

  lexer->position++;
  token->type = STRING;
  token->lexeme = lexeme;
  return token;
}

Token *create_keyword_token(Lexer *lexer, char character) {
  char *lexeme =
      allocator_alloc(&lexer->tokens.allocator, LEX_CAP * sizeof(char));

  if (lexeme == NULL) {
    slog_error("Failed to allocate memory for lexeme");
    return NULL;
  }

  size_t lexeme_length = 0;
  lexeme[lexeme_length++] = character;
  lexer->position++;

// Build the lexeme until a non-alphanumeric character is encountered
#ifdef __GNUC__
#pragma GCC unroll 100
#endif
  while (lexer->position < lexer->source_length) {
    character = lexer->source[lexer->position];
    if (isalnum(character) || character == '_') {
      lexeme[lexeme_length++] = character;
      lexer->position++;
      continue;
    }

    break;
  }

  lexeme[lexeme_length] = '\0';
  Token *token = allocator_alloc(&lexer->tokens.allocator, sizeof(Token));

  if (token == NULL) {
    slog_error("Failed to allocate memory for token");
    return NULL;
  }

  token->lexeme = lexeme;

  for (size_t i = 0; i < NUM_KEYWORDS; i++) {
    const char *substring = PYTHON_KEYWORD[i];
    if (strcmp(lexeme, substring) == 0) {
      token->type = KEYWORD;
      return token;
    }
  }

  token->type = IDENTIFIER;
  return token;
}

Token *create_newline_token(Lexer *lexer) {
  Token *token = allocator_alloc(&lexer->tokens.allocator, sizeof(Token));
  if (token == NULL) {
    slog_error("Failed to allocate memory for NEWLINE token");
    return NULL;
  }

  token->type = NEWLINE;
  token->lexeme = arena_strdup(&lexer->tokens.allocator.base, "\\n");
  return token;
}

char *dump_tokens(Token_ArrayList *tokens) {
  cJSON *json = serialize_tokens(tokens);
  char *dump = cJSON_Print(json);
  cJSON_Delete(json);
  return dump;
}

Token *peek_token(Lexer *lexer) {
  if (!lexer || lexer->token_idx >= lexer->tokens.size) {
    return NULL;
  }
  return lexer->tokens.elements[lexer->token_idx];
}

cJSON *serialize_tokens(Token_ArrayList *tokens) {
  cJSON *root = cJSON_CreateArray();

  for (size_t i = 0; i < tokens->size; i++) {
    cJSON_AddItemToArray(root, serialize_token(tokens->elements[i]));
  }
  return root;
}

cJSON *serialize_lexer(Lexer *lexer) {
  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "filename", lexer->filename);
  cJSON_AddNumberToObject(root, "position", lexer->position);
  cJSON_AddNumberToObject(root, "token_idx", lexer->token_idx);
  cJSON_AddNumberToObject(root, "source_length", lexer->source_length);
  cJSON *tokens_json = serialize_tokens(&lexer->tokens);
  cJSON_AddItemToObject(root, "tokens", tokens_json);
  return root;
}
