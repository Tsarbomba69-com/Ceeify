#include "lexer.h"
#include "parser.h"

int main2(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file-path>\n", argv[0]);
        return EXIT_FAILURE;
    }
    char *source = LoadFileText(argv[1]);
    if (source == NULL) return EXIT_FAILURE;
    Lexer lexer = Tokenize(source);
    printf("Total Tokens = \033[0;31m%zu\033[0m\n", lexer.tokens.size);
    puts("");
    printf("Token list = [\n");
    Token_ForEach(&lexer.tokens, PrintToken);
    printf("]\n");
    PrintArena();
    Node_LinkedList program = ParseStatements(&lexer);
    Node_ForEach(&program, PrintNode);
    PrintArena();
    FreeContext();
    PrintArena();
    return EXIT_SUCCESS;
}
