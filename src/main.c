#include "parser.h"
#ifndef FLAG_IMPLEMENTATION
#define FLAG_IMPLEMENTATION
#include "flag.h"
#endif

#ifndef ARENA_IMPLEMENTATION
#define ARENA_IMPLEMENTATION
#include "arena.h"
#endif

#define ONE_MB 1024 * 1024

int dump_ast(int argc, char **argv) {
  if (argc < 1) {
      trace_log(LOG_ERROR, "ARGS: No source file provided");
      return EXIT_FAILURE;
  }

  Arena allocator = {0};
  arena_alloc(&allocator, ONE_MB);
  char* source = load_file_text(&allocator, argv[0]);
  Lexer lexer = tokenize(source);
  Parser parser = parse(&lexer);
  cJSON *root = serialize_program(&parser.ast);
  char *result = cJSON_Print(root);
  trace_log(LOG_INFO, "%s", result);
  cJSON_Delete(root);
  parser_free(&parser);
  arena_free(&allocator);
  free(result);
  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  bool *dump_ast_t = flag_bool("dump-ast", false,
                             "Dump the parse tree after parsing and stop");

  if (!flag_parse(argc, argv)) {
    flag_print_error(stderr);
    return EXIT_FAILURE;
  }

  argc = flag_rest_argc();
  argv = flag_rest_argv();

  if (*dump_ast_t) {
    return dump_ast(argc, argv);
  }
  return EXIT_SUCCESS;
}