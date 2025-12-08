#include "semantic.h"

static SymbolTable symbol_table_new(SymbolTable *parent, size_t depth) {
  SymbolTable st = {.depth = depth, .parent = parent, .entries = NULL};
  return st;
}

SemanticAnalyzer analyze_program(Parser *parser) {
  SemanticAnalyzer sa = {0};
  SymbolTable global_scope = symbol_table_new(NULL, 0);
  sa.current_scope = &global_scope;
  sa.parser = parser;
  sa.last_error.type = SEM_OK;

  // Iterate over all top-level AST Nodes
  ASTNode_LinkedList *program = &parser->ast;
  ASTNode *node = NULL;
  do {
    node = ASTNode_pop(program);
    if (!analyze_node(&sa, node)) {
      // Stop immediately on first semantic error
      return sa;
    }
  } while (node != NULL);

  return sa;
}

Symbol *sa_lookup(SemanticAnalyzer *sa, const char *name) {
  SymbolTable *scope = sa->current_scope;
  while (scope) {
    for (SymbolTableEntry *e = scope->entries; e; e = e->next) {
      if (strcmp(e->symbol.name, name) == 0)
        return &e->symbol;
    }
    scope = scope->parent;
  }
  return NULL;
}

bool analyze_node(SemanticAnalyzer *sa, ASTNode *node) {
  if (!sa || !node)
    return true;

  switch (node->type) {
  case VARIABLE: {
    Symbol *sym = sa_lookup(sa, node->token->lexeme);
    if (!sym) {
      sa_set_error(sa, SEM_UNDEFINED_VARIABLE, node->token);
      return false;
    }
  } break;
  case BINARY_OPERATION: {
    if (!analyze_node(sa, node->bin_op.left))
      return false;
    if (!analyze_node(sa, node->bin_op.right))
      return false;
    break;
  }
  case ASSIGNMENT: {
    // Define all targets
    for (size_t cur = node->assign.targets.head; cur != SIZE_MAX;
         cur = node->assign.targets.elements[cur].next) {
      ASTNode *target = node->assign.targets.elements[cur].data;
      Symbol sym = {.name = arena_strdup(&sa->parser->ast.allocator.base,
                                         target->token->lexeme),
                    .kind = VAR,
                    .dtype =
                        node->assign.value->type == LITERAL
                            ? INT
                            : VOID, // TODO: Implement proper type inference
                    .decl_node = node,
                    .scope_level = node->depth};
      sa_define_symbol(sa, sym);
    }
    // Analyze value
    return analyze_node(sa, node->assign.value);
  }
  default:
    break;
  }

  return true;
}

void sa_define_symbol(SemanticAnalyzer *sa, Symbol sym) {
  if (!sa || !sa->current_scope) {
    slog_error("SemanticAnalyzer or current scope is NULL in sa_define_symbol");
    return;
  }

  SymbolTableEntry *entry =
      allocator_alloc(&sa->parser->ast.allocator, sizeof(SymbolTableEntry));
  entry->symbol = sym;
  entry->next = sa->current_scope->entries;
  sa->current_scope->entries = entry;
}

bool sa_has_error(SemanticAnalyzer *sa) {
  return sa && sa->last_error.type != SEM_OK;
}

char *sa_format_error(SemanticAnalyzer *sa) {
  if (!sa || sa->last_error.type == SEM_OK || !sa->last_error.token) {
    slog_error("No error to format in sa_format_error");
    return NULL;
  }

  Token *tok = sa->last_error.token;
  SemanticErrorType type = sa->last_error.type;
  const char *error_name = (type == SEM_UNDEFINED_VARIABLE)  ? "NameError"
                           : (type == SEM_REDEFINITION)      ? "SemanticError"
                           : (type == SEM_TYPE_MISMATCH)     ? "TypeError"
                           : (type == SEM_INVALID_OPERATION) ? "TypeError"
                                                             : "SemanticError";

  /* -----------------------------------------------------------
     Determine frame name (Python-style)
     <module> for global code
     function_name for inside functions
     ----------------------------------------------------------- */
  const char *frame = "<module>";
  SymbolTable *st = sa->current_scope;

  while (st) {
    SymbolTableEntry *e = st->entries;
    while (e) {
      if (e->symbol.kind == FUNCTION) {
        frame = e->symbol.name;
        goto frame_done; // break both loops
      }
      e = e->next;
    }
    st = st->parent;
  }
frame_done:;

  /* -----------------------------------------------------------
     Extract the line of source code where the error occurred
     ----------------------------------------------------------- */
  const char *source = sa->parser->lexer->source;
  size_t line_number = tok->line;

  const char *line_start = source;
  size_t curr_line = 1;

  while (*line_start && curr_line < line_number) {
    if (*line_start == '\n')
      curr_line++;
    line_start++;
  }

  const char *line_end = line_start;
  while (*line_end && *line_end != '\n')
    line_end++;

  size_t line_len = line_end - line_start;
  char *line_content = (char *)malloc(line_len + 1);
  memcpy(line_content, line_start, line_len);
  line_content[line_len] = 0;

  /* -----------------------------------------------------------
     Build caret underline: place '^' under the token column
     ----------------------------------------------------------- */
  size_t col = tok->col > 1 ? tok->col + 1 : 1;
  char *highlight = malloc(col + 2);
  memset(highlight, ' ', col);
  highlight[col] = '^';
  highlight[col + 1] = 0;
  const char *filename = sa->parser->lexer->filename;
  size_t buf_size =
      strlen(filename) + strlen(line_content) + strlen(highlight) + 256;
  char *msg = allocator_alloc(&sa->parser->ast.allocator, buf_size);
  snprintf(msg, buf_size,
           "  File \"%s\", line %zu, in %s\n"
           "    %s\n"
           "    %s\n"
           "%s: name '%s' is not defined",
           filename, line_number, frame, line_content, highlight, error_name,
           tok->lexeme);
  free(line_content);
  free(highlight);
  return msg;
}

void sa_set_error(SemanticAnalyzer *sa, SemanticErrorType type, Token *tok) {
  if (!sa) {
    slog_error("SemanticAnalyzer pointer is NULL in sa_set_error");
    return;
  }
  sa->last_error.type = type;
  sa->last_error.token = tok;
  sa->last_error.message = sa_format_error(sa);
}

SemanticError sa_get_error(SemanticAnalyzer *sa) {
  if (!sa) {
    SemanticError err = {0};
    err.type = SEM_UNKNOWN;
    err.token = NULL;
    err.message = "SemanticAnalyzer pointer is NULL";
    return err;
  }

  return sa->last_error;
}