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

Lexer lexer_new(const char *source) {
  size_t len = strlen(source);
  return (Lexer){.source = source,
                 .position = 0,
                 .token_idx = 0,
                 .source_length = len,
                 .tokens = Token_arraylist_new(100)};
}

Lexer tokenize(const char *source) {
  Lexer lexer = lexer_new(source);
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

    for (size_t i = 0; i < ARRAYSIZE(DELIMITERS); i++) {
      if (character == DELIMITERS[i]) {
        token = create_token_from_char(&lexer, character, DELIMITER);
        token->col = column;
        token->line = line;
        token->ident = ident;
        Token_push(&lexer.tokens, token);
        break;
      }
    }

    if (token != NULL) {
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

    if (character == '[') {
      token = create_token_from_char(&lexer, character, LSQB);
      token->col = column;
      token->line = line;
      token->ident = ident;
      Token_push(&lexer.tokens, token);
      continue;
    }

    if (character == ']') {
      token = create_token_from_char(&lexer, character, RSQB);
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

Token *next_token(Lexer *lexer) {
  lexer->token_idx++;
  return Token_get(&lexer->tokens, lexer->token_idx - 1);
}

Token *create_token_from_char(Lexer *lexer, char character, TokenType type) {
  Token *token = arena_alloc(&lexer->tokens.arena, sizeof(Token));
  if (token == NULL) {
    fprintf(stderr, "ERROR: Could not allocate memory for token\n");
    return NULL;
  }

  char *lexeme = arena_alloc(&lexer->tokens.arena, sizeof(char) * 2);
  if (lexeme == NULL) {
    fprintf(stderr, "ERROR: Failed to allocate memory for lexeme\n");
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

  char *lexeme =
      arena_alloc(&lexer->tokens.arena, max_lexeme_length * sizeof(char) + 1);
  if (lexeme == NULL) {
    trace_log(LOG_ERROR, "Failed to allocate memory for lexeme");
    return NULL;
  }

  memcpy(lexeme, matched_operator, max_lexeme_length);
  lexeme[max_lexeme_length] = '\0';
  lexer->position += max_lexeme_length;

  Token *token = arena_alloc(&lexer->tokens.arena, sizeof(Token));
  if (token == NULL) {
    trace_log(LOG_ERROR, "Failed to allocate memory for token");
    return NULL;
  }

  token->type = OPERATOR;
  token->lexeme = lexeme;
  return token;
}

Token *create_EOF_token(Lexer *lexer) {
  Token *token = arena_alloc(&lexer->tokens.arena, sizeof(Token));
  if (token == NULL) {
    fprintf(stderr, "ERROR: Failed to allocate memory for EOF token\n");
    return NULL;
  }

  token->type = ENDMARKER;
  token->lexeme = arena_strdup(&lexer->tokens.arena, "EOF");
  return token;
}

Token *create_number_token(Lexer *lexer, char character) {
  char *lexeme = arena_alloc(&lexer->tokens.arena, LEX_CAP * sizeof(char));
  if (lexeme == NULL) {
    fprintf(stderr, "ERROR: Failed to allocate memory for lexeme\n");
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
  Token *token = arena_alloc(&lexer->tokens.arena, sizeof(Token));
  if (token == NULL) {
    fprintf(stderr, "ERROR: Failed to allocate memory for token\n");
    return NULL;
  }

  token->type = NUMERIC;
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
      slice(&lexer->tokens.arena, lexer->source, start, lexer->position);
  Token *token = arena_alloc(&lexer->tokens.arena, sizeof(Token));

  if (token == NULL) {
    fprintf(stderr, "ERROR: Failed to allocate memory for token\n");
    return NULL;
  }

  lexer->position++;
  token->type = TEXT;
  token->lexeme = lexeme;
  return token;
}

Token *create_keyword_token(Lexer *lexer, char character) {
  char *lexeme = arena_alloc(&lexer->tokens.arena, LEX_CAP * sizeof(char));

  if (lexeme == NULL) {
    fprintf(stderr, "ERROR: Failed to allocate memory for lexeme\n");
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

  Token *token = arena_alloc(&lexer->tokens.arena, sizeof(Token));
  if (token == NULL) {
    fprintf(stderr, "ERROR: Failed to allocate memory for token\n");
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
  Token *token = arena_alloc(&lexer->tokens.arena, sizeof(Token));
  if (token == NULL) {
    fprintf(stderr, "ERROR: Failed to allocate memory for NEWLINE token\n");
    return NULL;
  }

  token->type = NEWLINE;
  token->lexeme = arena_strdup(&lexer->tokens.arena, "\\n");
  return token;
}
