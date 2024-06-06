//#include "lexer.h"
//#include "parser.h"
//
//int main(int argc, char *argv[]) {
//    if (argc < 2) {
//        printf("Usage: %s <file-path>\n", argv[0]);
//        return EXIT_FAILURE;
//    }
//    char *source = LoadFileText(argv[1]);
//    if (source == NULL) return EXIT_FAILURE;
//    Lexer lexer = CreateLexer(source);
//    Tokens tokens = Tokenize(&lexer);
//    printf("Total Tokens = \033[0;31m%zu\033[0m\n", tokens.size);
//    puts("");
//    printf("Token list = [\n");
//    Token_ForEach(&tokens, PrintToken);
//    printf("]\n");
//    PrintArena();
//    Node_LinkedList program = ParseStatements(&tokens);
//    Node_ForEach(&program, PrintNode);
//    PrintArena();
//    FreeContext();
//    PrintArena();
//    return EXIT_SUCCESS;
//}
