#pragma once
#ifndef LEXER_H
#define LEXER_H

#include "utils.h"
#include "Token_arraylist.h"
#include <ctype.h>

typedef struct Token_ArrayList Token_ArrayList;

typedef enum TokenType {
    IDENTIFIER,
    INTEGER,
    FLOAT,
    STRING,
    OPERATOR,
    KEYWORD,
    DELIMITER,
    NEWLINE,
    LSQB,
    RSQB,
    ENDMARKER
} TokenType;

typedef struct Token {
    TokenType type;
    char *lexeme;
    size_t line;
    size_t col;
    size_t ident;
} Token;

typedef struct Lexer {
    char *source;
    size_t position;
    size_t token_idx;
    size_t sourceLength;
    Token_ArrayList tokens;
} Lexer;

Lexer CreateLexer(char *source);

Lexer Tokenize(char *source);

void PrintToken(Token *token);

const char *TokenTypeToString(TokenType type);

Token *CreateToken(TokenType type);

Token *CreateTokenFromChar(Lexer *lexer, char character, TokenType type);

Token *CreateStringToken(Lexer *lexer, char character);

Token *CreateOperatorToken(Lexer *lexer, const char *matchedOperator);

Token *CreateKeywordToken(Lexer *lexer, char character);

Token *CreateNumberToken(Lexer *lexer, char character);

Token *CreateNewLineToken();

Token *CreateEOFToken();

#endif // !LEXER_H


