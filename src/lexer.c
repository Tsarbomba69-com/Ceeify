#include "lexer.h"

#define NUM_KEYWORDS 35
#define LEX_CAP 32

const char* PYTHON_KEYWORD[NUM_KEYWORDS] = {
	"False", "None", "True", "and", "as", "assert", "async", "await", "break",
	"class", "continue", "def", "del", "elif", "else", "except", "finally",
	"for", "from", "global", "if", "import", "in", "is", "lambda", "nonlocal",
	"not", "or", "pass", "raise", "return", "try", "while", "with", "yield" };

const char OPERATORS[] = { '+', '-', '*', '/', '%', '>', '<', '!', '=', '&', '|', '^', '~', '.' };

const char* EXTENDED_OPERATORS[] = {
	"//", "==", "!=", "**", ">=", "<=", "&&", "||", "+=", "-=", "*=", "/=", "%=", "//=", "**=", "<<", ">>" };

const char DELIMITERS[] = { '(', ')', '[', ']', '{', '}', ',', ';', '.', ':', '`' };

Lexer CreateLexer(char* source)
{
	size_t len = strlen(source);
	return (Lexer) { source, 0, len };
}

ArrayList Tokenize(Lexer* lexer)
{
	size_t len = strlen(lexer->source);
	ArrayList tokens = CreateArrayList(100);
	while (lexer->position < lexer->sourceLength)
	{
		char character = lexer->source[lexer->position];
		if (character == ' ' || character == '\t')
		{
			lexer->position++;
			continue;
		}

		if (character == '#')
			while ('\n' != lexer->source[lexer->position++]) continue;

		Token* token;
		const char* matchedOperator = NULL;
		for (size_t i = 0; i < ARRAYSIZE(OPERATORS); i++)
		{
			if (character == OPERATORS[i])
			{
				matchedOperator = &character;
				break;
			}
		}

		const char* matchedDelimiter = NULL;
		for (size_t i = 0; i < ARRAYSIZE(DELIMITERS); i++)
		{
			if (character == DELIMITERS[i])
			{
				matchedDelimiter = &character;
			}
		}

		if (matchedOperator != NULL)
		{
			// Build the operator lexeme
			token = CreateOperatorToken(lexer, matchedOperator);
			ArrayListPush(&tokens, token);
			continue;
		}

		if (matchedDelimiter != NULL)
		{
			token = CreateDelimiterToken(lexer, (const char*)character);
			ArrayListPush(&tokens, token);
			continue;
		}

		if (isdigit(character))
		{
			token = CreateNumberToken(lexer, character);
			ArrayListPush(&tokens, token);
		}

		if (character == '\'' || character == '\"')
		{
			token = CreateStringToken(lexer, character);
			ArrayListPush(&tokens, token);
		}

		if (isalpha(character) || character == '_')
		{
			token = CreateKeywordToken(lexer, character);
			ArrayListPush(&tokens, token);
			continue;
		}

		if (character == '\n')
		{
			token = CreateNewLineToken(lexer);
			ArrayListPush(&tokens, token);
		}

		lexer->position++;
	}
	Token* eof = CreateEOFToken(lexer);
	ArrayListPush(&tokens, eof);
	return tokens;
}

void PrintToken(Token* token)
{
	const char* type = TokenTypeToString(token->type);
	printf("    { \033[0;36mlexeme\033[0m: \033[0;33m\"%s\"\033[0m, \033[0;36m\033[0;36mtype\033[0m\033[0m: \033[0;36m\033[0;92m%s\033[0m\033[0m },\n", token->lexeme, type);
}

Token* CreateStringToken(Lexer* lexer, char character)
{
	size_t start = lexer->position + 1;
	lexer->position++;
	while (lexer->source[lexer->position] != character) lexer->position++;
	char* lexeme = Slice(lexer->source, start, lexer->position);
	Token* token = (Token*)malloc(sizeof(Token));
	if (token == NULL)
	{
		fprintf(stderr, "ERROR: Failed to allocate memory for token\n");
		return NULL;
	}

	token->type = STRING;
	token->lexeme = lexeme;
	return token;
}

Token* CreateDelimiterToken(Lexer* lexer, const char* character)
{
	Token* token = (Token*)malloc(sizeof(Token));
	if (token == NULL)
	{
		fprintf(stderr, "ERROR: Could not allocate memory for token\n");
		return NULL;
	}

	char* lexeme = (char*)malloc(sizeof(char) * 2);
	if (lexeme == NULL)
	{
		fprintf(stderr, "ERROR: Failed to allocate memory for lexeme\n");
		return NULL;
	}

	memcpy(lexeme, &character, 2);
	lexeme[1] = '\0';
	token->lexeme = lexeme;
	token->type = DELIMITER;
	lexer->position++;
	return token;
}

Token* CreateNewLineToken(Lexer* lexer)
{
	Token* token = (Token*)malloc(sizeof(Token));
	if (token == NULL)
	{
		fprintf(stderr, "ERROR: Failed to allocate memory for NEWLINE token\n");
		return NULL;
	}

	token->type = NEWLINE;
	token->lexeme = _strdup("\\n");
	return token;
}

Token* CreateEOFToken(Lexer* lexer)
{
	Token* token = (Token*)malloc(sizeof(Token));
	if (token == NULL)
	{
		fprintf(stderr, "ERROR: Failed to allocate memory for EOF token\n");
		return NULL;
	}

	token->type = ENDMARKER;
	token->lexeme = _strdup("EOF");
	return token;
}

Token* CreateOperatorToken(Lexer* lexer, const char* matchedOperator)
{
	size_t maxLexemeLength = 1;
	for (size_t i = 0; i < ARRAYSIZE(EXTENDED_OPERATORS); i++)
	{
		const char* operatorStr = EXTENDED_OPERATORS[i];
		size_t operatorLength = strlen(operatorStr);

		if (strncmp(&lexer->source[lexer->position], operatorStr, operatorLength) == 0 &&
			operatorLength > maxLexemeLength)
		{
			maxLexemeLength = operatorLength;
			matchedOperator = operatorStr;
		}
	}

	char* lexeme = (char*)malloc(maxLexemeLength * sizeof(char) + 1);
	if (lexeme == NULL)
	{
		fprintf(stderr, "ERROR: Failed to allocate memory for lexeme\n");
		return NULL;
	}

	memcpy(lexeme, matchedOperator, maxLexemeLength);
	lexeme[maxLexemeLength] = '\0';
	lexer->position += maxLexemeLength;

	Token* token = (Token*)malloc(sizeof(Token));
	if (token == NULL)
	{
		fprintf(stderr, "ERROR: Failed to allocate memory for token\n");
		return NULL;
	}

	token->type = OPERATOR;
	token->lexeme = lexeme;
	return token;
}

Token* CreateKeywordToken(Lexer* lexer, char character)
{
	char* lexeme = (char*)malloc(LEX_CAP * sizeof(char));
	if (lexeme == NULL)
	{
		fprintf(stderr, "ERROR: Failed to allocate memory for lexeme\n");
		return NULL;
	}
	size_t lexeme_length = 0;
	lexeme[lexeme_length++] = character;
	lexer->position++;

	// Build the lexeme until a non-alphanumeric character is encountered
	while (lexer->position < lexer->sourceLength)
	{
		character = lexer->source[lexer->position];
		if (isalnum(character) || character == '_')
		{
			lexeme[lexeme_length++] = character;
			lexer->position++;
		}
		else break;
	}
	lexeme[lexeme_length] = '\0';

	Token* token = (Token*)malloc(sizeof(Token));
	if (token == NULL)
	{
		fprintf(stderr, "ERROR: Failed to allocate memory for token\n");
		return NULL;
	}
	token->lexeme = lexeme;
	for (size_t i = 0; i < NUM_KEYWORDS; i++)
	{
		const char* substring = PYTHON_KEYWORD[i];
		if (strcmp(lexeme, substring) == 0)
		{
			token->type = KEYWORD;
			return token;
		}
	}

	token->type = IDENTIFIER;
	return token;
}

Token* CreateNumberToken(Lexer* lexer, char character)
{
	char* lexeme = (char*)malloc(LEX_CAP * sizeof(char));
	if (lexeme == NULL)
	{
		fprintf(stderr, "ERROR: Failed to allocate memory for lexeme\n");
		return NULL;
	}
	size_t lexeme_length = 0;
	bool hasDecimal = false;
	lexeme[lexeme_length++] = character;
	lexer->position++;

	while (lexer->position < lexer->sourceLength)
	{
		character = lexer->source[lexer->position];
		if (isdigit(character) || character == '_')
		{
			lexeme[lexeme_length++] = character;
			lexer->position++;
		}
		else if (character == '.')
		{
			lexeme[lexeme_length++] = character;
			lexer->position++;
			hasDecimal = true;
		}
		else
		{
			break;
		}
	}
	lexeme[lexeme_length] = '\0';
	Token* token = (Token*)malloc(sizeof(Token));
	if (token == NULL)
	{
		fprintf(stderr, "ERROR: Failed to allocate memory for token\n");
		return NULL;
	}

	token->lexeme = lexeme;

	if (hasDecimal)
	{
		token->type = FLOAT;
		return token;
	}
	token->type = INTEGER;
	return token;
}

const char* TokenTypeToString(TokenType type)
{
	switch (type)
	{
	case KEYWORD: return "KEYWORD";
	case OPERATOR: return "OPERATOR";
	case STRING: return "STRING";
	case IDENTIFIER: return "IDENTIFIER";
	case DELIMITER: return "DELIMITER";
	case INTEGER: return "INTEGER";
	case FLOAT: return "FLOAT";
	case NEWLINE: return "NEWLINE";
	case ENDMARKER: return "ENDMARKER";
	default: return "UNKNOWN";
	}
}

void DestroyToken(Token* token)
{
	free(token->lexeme);
	free(token);
}
