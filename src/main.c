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

int dump_ast(const char *source_path, const char *out_file) {
  Arena allocator = {0};
  arena_alloc(&allocator, ONE_MB);
  char* source = load_file_text(&allocator, source_path);
  Lexer lexer = tokenize(source);
  Parser parser = parse(&lexer);
  cJSON *root = serialize_program(&parser.ast);
  char *result = cJSON_Print(root);

  if (out_file != NULL && strlen(out_file) > 0) {
    if (!save_file_text(out_file, result)) return EXIT_FAILURE;
  } else {
    trace_log(LOG_INFO, "%s", result);
  }

  cJSON_Delete(root);
  parser_free(&parser);
  arena_free(&allocator);
  free(result);
  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  char **dump_source = flag_str("dump-ast", NULL,
                             "Dump the parse tree after parsing and stop");
  char **out_file = flag_str("o", NULL, "Output file (default: stdout)");

  if (!flag_parse(argc, argv)) {
    flag_print_error(stderr);
    return EXIT_FAILURE;
  }

  argc = flag_rest_argc();
  argv = flag_rest_argv();

  if (*dump_source != NULL) {
    return dump_ast(*dump_source, *out_file);
  }

  return EXIT_SUCCESS;
}