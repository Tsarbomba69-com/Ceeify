#include "lexer.h"

#define NUM_KEYWORDS 35

const char* PYTHON_KEYWORD[NUM_KEYWORDS] = {
	"False", "None", "True", "and", "as", "assert", "async", "await", "break",
	"class", "continue", "def", "del", "elif", "else", "except", "finally",
	"for", "from", "global", "if", "import", "in", "is", "lambda", "nonlocal",
	"not", "or", "pass", "raise", "return", "try", "while", "with", "yield"
};

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
				else break;
			}
			lexeme[lexeme_length] = '\0';

			for (size_t i = 0; i < NUM_KEYWORDS; i++)
			{
				const char* substring = PYTHON_KEYWORD[i];
				if (strcmp(lexeme, substring) == 0)
				{
					Token token = { 0 };
					token.lexeme = lexeme;
					token.type = KEYWORD;
					PrintToken(&token);
				}
			}
		}

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
	case KEYWORD:
		return "KEYWORD";
	default:
		return "Unknown";
	}
}

