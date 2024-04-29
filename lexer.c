#include "lexer.h"

#define NUM_KEYWORDS 35

const char* PYTHON_KEYWORD[NUM_KEYWORDS] = {
	"False", "None", "True", "and", "as", "assert", "async", "await", "break",
	"class", "continue", "def", "del", "elif", "else", "except", "finally",
	"for", "from", "global", "if", "import", "in", "is", "lambda", "nonlocal",
	"not", "or", "pass", "raise", "return", "try", "while", "with", "yield" };

const char OPERATORS[] = { '+', '-', '*', '/', '%', '>', '<', '!', '=', '&', '|', '^', '~' };

const char* EXTENDED_OPERATORS[] = {
	"//", "==", "!=", "**", ">=", "<=", "&&", "||", "+=", "-=", "*=", "/=", "%=", "//=", "**=", "<<", ">>" };
// TODO: Should return a collection of Tokens
void Tokenize(Lexer* lexer)
{
	size_t len = strlen(lexer->source);

	while (lexer->position < len)
	{
		char character = lexer->source[lexer->position];
		if (character == ' ' || character == '\t')
		{
			lexer->position++;
			continue;
		}
		// TODO: Abstract token creation
		// ------------- BEGIN OPERATORS  -------------------------
		unsigned int maxLexemeLength = 0;
		const char* matchedOperator = NULL;
		for (size_t i = 0; i < ARRAYSIZE(OPERATORS); i++)
		{
			if (character == OPERATORS[i])
			{
				maxLexemeLength = 1;
				matchedOperator = &character;
				break;
			}
		}

		if (matchedOperator != NULL)
		{
			// Build the operator lexeme
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

			char lexeme[3];
			memcpy(lexeme, matchedOperator, maxLexemeLength);
			lexeme[maxLexemeLength] = '\0';
			Token token = { OPERATOR, lexeme };
			lexer->position += maxLexemeLength;
			PrintToken(&token);
		}
		// ------------- END OPERATORS  -------------------------

		if (character == '\'' || character == '\"')
		{
			size_t start = lexer->position + 1;
			lexer->position++;
			while (lexer->source[lexer->position] != character) lexer->position++;
			char* lexeme = Slice(lexer->source, start, lexer->position);
			Token token = { STRING, lexeme };
			PrintToken(&token);
		}

		// ------------- BEGIN KEYWORDS  -------------------------
		if (isalpha(character))
		{
			char lexeme[32];
			size_t lexeme_length = 0;
			lexeme[lexeme_length++] = character;
			lexer->position++;

			// Build the lexeme until a non-alphanumeric character is encountered
			while (lexer->position < len)
			{
				character = lexer->source[lexer->position];
				if (isalnum(character))
				{
					lexeme[lexeme_length++] = character;
					lexer->position++;
				}
				else
					break;
			}
			lexeme[lexeme_length] = '\0';

			for (size_t i = 0; i < NUM_KEYWORDS; i++)
			{
				const char* substring = PYTHON_KEYWORD[i];
				if (strcmp(lexeme, substring) == 0)
				{
					Token token = { KEYWORD, lexeme };
					PrintToken(&token);
				}
			}
		}
		// -------------- END KEYWORDS --------------------------
		lexer->position++;
	}
}

void PrintToken(Token* token)
{
	const char* type = TokenTypeToString(token->type);
	printf("{ lexeme: \"%s\", type: %s }\n", token->lexeme, type);
}

const char* TokenTypeToString(TokenType type)
{
	switch (type)
	{
	case KEYWORD: return "KEYWORD";
	case OPERATOR: return "OPERATOR";
	case STRING: return "STRING";
	default: return "UNKNOWN";
	}
}
