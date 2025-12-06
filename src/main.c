#include "parser.h"
#include "profiler.h"
#ifndef FLAG_IMPLEMENTATION
#define FLAG_IMPLEMENTATION
#include "flag.h"
#endif

#ifndef ARENA_IMPLEMENTATION
#define ARENA_IMPLEMENTATION
#include "arena.h"
#endif

#define MIN_CAP 1024

// TODO: Add support for multiple files
// TODO: Perhaps move argument parsing to its own module (for testing purposes)

int dump_ast(const char *source_path, const char *out_file) {
  Allocator allocator = {0};
  allocator_init(&allocator, "dump_ast");
  allocator_alloc(&allocator, MIN_CAP);
  char *source = load_file_text(&allocator, source_path);
  TraceBuffer trace = trace_buffer_create(&allocator, 100);
  trace_event_begin(&trace, "lex");
  Lexer lexer = tokenize(source, source_path);
  trace_event_end(&trace, "lex");
  trace_event_begin(&trace, "parse");
  Parser parser = parse(&lexer);
  trace_event_end(&trace, "parse");
  trace_event_begin(&trace, "serialize");
  cJSON *root = serialize_program(&parser.ast);
  trace_event_end(&trace, "serialize");
  trace_event_begin(&trace, "json_print");
  char *result = cJSON_Print(root);
  trace_event_end(&trace, "json_print");

  trace_event_begin(&trace, "json_export");
  if (out_file != NULL && strlen(out_file) > 0) {
    if (!save_file_text(out_file, result))
      return EXIT_FAILURE;
  } else {
    slog_info("%s", result);
  }
  trace_event_end(&trace, "json_export");

  cJSON_Delete(root);
  parser_free(&parser);
  free(result);
  char *json = trace_buffer_to_json(&trace);
  save_file_text("trace.json", json);
  free(json);
  trace_buffer_destroy(&trace);
  return EXIT_SUCCESS;
}

void usage(FILE *stream) {
  slog_info("Usage: ./ceeify [OPTIONS] <input-file>");
  slog_info("OPTIONS:");
  flag_print_options(stream);
}

static void reorder_args(int *argc, char **argv) {
  char **flags_buf = malloc(*argc * sizeof(char *));
  char **pos_buf = malloc(*argc * sizeof(char *));
  if (!flags_buf || !pos_buf) {
    slog_error("allocation failure during argument reordering");
    exit(EXIT_FAILURE);
  }

  int f = 0, p = 0;
  for (int i = 1; i < *argc; ++i) {
    if (strcmp(argv[i], "--") == 0) {
      /* everything after -- is positional */
      for (int j = i + 1; j < *argc; ++j)
        pos_buf[p++] = argv[j];
      break;
    }

    if (argv[i][0] == '-') {
      flags_buf[f++] = argv[i];

      /* group flag with its value if next arg isn't another flag */
      if (i + 1 < *argc && argv[i + 1][0] != '-') {
        flags_buf[f++] = argv[++i];
      }
    } else {
      pos_buf[p++] = argv[i];
    }
  }
  int idx = 0;
  argv[idx++] = argv[0]; /* program name */
  for (int i = 0; i < f; ++i)
    argv[idx++] = flags_buf[i];
  for (int i = 0; i < p; ++i)
    argv[idx++] = pos_buf[i];
  argv[idx] = NULL; /* null terminator */

  free(flags_buf);
  free(pos_buf);
}

int main(int argc, char **argv) {
  slog_init("ceeify", SLOG_FLAGS_ALL, 0);
#if ARENA_DEBUG_MODE
  atexit(allocator_global_report_leaks);
#endif
  bool *help =
      flag_bool("help", false, "Print this help to stdout and exit with 0");
  bool *dump_flag = flag_bool("dump-ast", false,
                              "Dump the parse tree after parsing and stop");
  char **out_file = flag_str("o", NULL, "Output file (default: stdout)");

  /* reorder so flags can appear anywhere */
  reorder_args(&argc, argv);

  if (!flag_parse(argc, argv)) {
    usage(stderr);
    flag_print_error(stderr);
    return EXIT_FAILURE;
  }

  /* remaining args */
  argc = flag_rest_argc();
  argv = flag_rest_argv();

  if (*help) {
    usage(stdout);
    return EXIT_SUCCESS;
  }

  if (argc < 1) {
    slog_error("missing input file");
    usage(stdout);
    return EXIT_FAILURE;
  }

  const char *in_filepath = argv[0];

  if (*dump_flag) {
    int rc = dump_ast(in_filepath, *out_file);
    return rc;
  }

  return EXIT_SUCCESS;
}
