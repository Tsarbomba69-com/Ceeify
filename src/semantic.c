#include "semantic.h"

const char *ARITHMETIC_OPS[] = {
    "+", "-", "*", "/", "%", // arithmetic
};

const char *COMPARISON_OPS[] = {
    "==", "!=", "<", ">", "<=", ">=", // comparison
};

DataType sa_infer_type(SemanticAnalyzer *sa, ASTNode *node);

static SymbolTable *symbol_table_new(Allocator *allocator, SymbolTable *parent,
                                     size_t depth) {
  ASSERT(allocator != NULL, "Allocator cannot be NULL in symbol_table_new");
  SymbolTable *st = allocator_alloc(allocator, sizeof(SymbolTable));
  st->entries = NULL;
  st->parent = parent;
  st->depth = depth;
  return st;
}

DataType string_to_datatype(const char *name) {
  if (strcmp(name, "int") == 0)
    return INT;
  if (strcmp(name, "float") == 0)
    return FLOAT;
  if (strcmp(name, "str") == 0)
    return STR;
  if (strcmp(name, "bool") == 0)
    return BOOL;
  if (strcmp(name, "list") == 0)
    return LIST;
  if (strcmp(name, "None") == 0)
    return NONE;
  return UNKNOWN;
}

bool types_compatible(DataType lhs, DataType rhs) {
  // Unknown types are never safe
  if (lhs == UNKNOWN || rhs == UNKNOWN) {
    return false;
  }

  // Exact match
  if (lhs == rhs) {
    return true;
  }

  // Widening numeric conversions
  if (lhs == FLOAT && rhs == INT) {
    return true;
  }

  // Everything else is illegal
  return false;
}

const char *datatype_to_string(DataType t) {
  switch (t) {
  case INT:
    return "int";
  case STR:
    return "str";
  case BOOL:
    return "bool";
  case FLOAT:
    return "float";
  case LIST:
    return "list";
  default:
    return "<unknown>";
  }
}

bool is_arithmetic_op(char *op) {
  for (size_t i = 0; i < ARRAYSIZE(ARITHMETIC_OPS); i++) {
    if (strcmp(ARITHMETIC_OPS[i], op) == 0)
      return true;
  }

  return false;
}

bool is_comparison_op(char *op) {
  for (size_t i = 0; i < ARRAYSIZE(COMPARISON_OPS); i++) {
    if (strcmp(COMPARISON_OPS[i], op) == 0)
      return true;
  }

  return false;
}

static DataType infer_binary_op(SemanticAnalyzer *sa, ASTNode *node) {
  if (!node || node->type != BINARY_OPERATION)
    return UNKNOWN;

  ASTNode *L = node->bin_op.left;
  ASTNode *R = node->bin_op.right;
  DataType lt = sa_infer_type(sa, L);
  DataType rt = sa_infer_type(sa, R);

  if (lt == UNKNOWN || rt == UNKNOWN)
    return UNKNOWN;

  /* Arithmetic operators: allow INT/FLOAT mixing */
  if (is_arithmetic_op(node->token->lexeme)) {
    /* string concatenation special case for + */
    if (strcmp(node->token->lexeme, "+") == 0 && lt == STR && rt == STR)
      return STR;

    if ((lt == INT || lt == FLOAT) && (rt == INT || rt == FLOAT)) {
      return (lt == FLOAT || rt == FLOAT) ? FLOAT : INT;
    }

    sa_set_error(
        sa, SEM_TYPE_MISMATCH, node->token,
        "unsupported operand types for operator (arithmetic): '%s' and '%s'",
        datatype_to_string(lt), datatype_to_string(rt));
    return UNKNOWN;
  }

  /* Comparison operators -> bool (we allow comparing same-typed values) */
  if (is_comparison_op(node->token->lexeme)) {
    /* allow comparing same basic types (int/float interchangeable) */
    if ((lt == INT || lt == FLOAT) && (rt == INT || rt == FLOAT))
      return BOOL;
    if (lt == rt)
      return BOOL;

    sa_set_error(sa, SEM_TYPE_MISMATCH, node->token,
                 "unsupported operand types for comparison: '%s' and '%s'",
                 datatype_to_string(lt), datatype_to_string(rt));
    return UNKNOWN;
  }

  // /* Logical operators (and/or) -> bool */
  if (is_boolean_operator(node->token)) {
    if (lt == BOOL && rt == BOOL)
      return BOOL;
    sa_set_error(sa, SEM_TYPE_MISMATCH, node->token,
                 "logical operators require bool operands, got '%s' and '%s'",
                 datatype_to_string(lt), datatype_to_string(rt));
    return UNKNOWN;
  }

  /* Fallback: unknown operator */
  sa_set_error(sa, SEM_UNKNOWN, node->token, "unknown binary operator");
  return UNKNOWN;
}
// TODO: Should report operations between numeric and other types as errors
DataType sa_infer_type(SemanticAnalyzer *sa, ASTNode *node) {
  ASSERT(sa != NULL, "SemanticAnalyzer cannot be NULL in sa_infer_type");
  ASSERT(node != NULL, "Cannot infer type of NULL node");

  switch (node->type) {
  case LITERAL: {
    Token *tok = node->token;
    const char *lex = tok->lexeme;

    if (!lex) {
      sa_set_error(sa, SEM_UNKNOWN, tok, "Invalid literal with no lexeme");
      return UNKNOWN;
    }

    if (strcmp(lex, "true") == 0 || strcmp(lex, "false") == 0) {
      return BOOL;
    }

    if (strcmp(lex, "None") == 0) {
      return NONE;
    }

    if (tok->type == STRING) {
      return STR;
    }

    if (tok->type == NUMBER) {
      bool is_int = true;
      bool is_float = false;

      for (size_t i = 0; lex[i]; i++) {
        if (lex[i] == '.') {
          is_float = true;
          is_int = false;
        } else if (!isdigit((unsigned char)lex[i]) &&
                   !(i == 0 && (lex[i] == '-' || lex[i] == '+'))) {
          is_int = false;
          is_float = false;
          break;
        }
      }

      if (is_int)
        return INT;
      if (is_float)
        return FLOAT;
    }

    size_t n = strlen(lex);
    if (lex[0] == '[' && lex[n - 1] == ']') {
      return LIST;
    }

    sa_set_error(sa, SEM_UNKNOWN, tok, "cannot infer type of literal '%s'",
                 lex);
    return UNKNOWN;
  } break;
  case ASSIGNMENT: {
    return sa_infer_type(sa, node->assign.value);
  } break;
  case BINARY_OPERATION:
    return infer_binary_op(sa, node);
  case UNARY_OPERATION:
    return sa_infer_type(sa, node->bin_op.right);
  case VARIABLE: {
    if (node->annotation) {
      return string_to_datatype(node->annotation->token->lexeme);
    }

    DataType dtype = string_to_datatype(node->token->lexeme);

    if (dtype != UNKNOWN) {
      return dtype;
    }

    Symbol *sym = sa_lookup(sa, node->token->lexeme);
    if (sym) {
      return sym->dtype;
    } else {
      sa_set_error(sa, SEM_UNDEFINED_VARIABLE, node->token,
                   "name '%s' is not defined", node->token->lexeme);
      return UNKNOWN;
    }
  } break;
  case FUNCTION_DEF: {
    DataType ret_type = VOID;

    for (size_t cur = node->funcdef.body.head; cur != SIZE_MAX;
         cur = node->funcdef.body.elements[cur].next) {
      ASTNode *body_node = node->funcdef.body.elements[cur].data;
      if (body_node->type == RETURN) {
        ret_type = sa_infer_type(sa, body_node->ret);
      }
    }

    if (node->funcdef.returns) {
      DataType annotation_type = sa_infer_type(sa, node->funcdef.returns);
      if (ret_type != annotation_type) {
        sa_set_error(sa, SEM_TYPE_MISMATCH, node->funcdef.returns->token,
                     "function return type annotation '%s' does not match "
                     "inferred return type '%s'",
                     datatype_to_string(annotation_type),
                     datatype_to_string(ret_type));
        return UNKNOWN;
      }
      return annotation_type;
    }
    return ret_type;
  } break;
  default:
    return VOID;
  }
}

SemanticAnalyzer analyze_program(Parser *parser) {
  SemanticAnalyzer sa = {0};
  SymbolTable *global_scope = symbol_table_new(&parser->ast.allocator, NULL, 0);
  sa.current_scope = global_scope;
  sa.parser = *parser;
  sa.last_error.type = SEM_OK;

  if (parser != NULL && parser->ast.size == 0) {
    slog_warn("No AST to analyze in analyze_program");
    return sa;
  }

  // Iterate over all top-level AST Nodes
  ASTNode_LinkedList *program = &parser->ast;
  for (size_t current = program->head; current != SIZE_MAX;
       current = program->elements[current].next) {
    ASTNode *node = program->elements[current].data;

    if (!analyze_node(&sa, node)) {
      // Stop immediately on first semantic error
      return sa;
    }
  }

  return sa;
}

Symbol *sa_lookup(SemanticAnalyzer *sa, const char *name) {
  SymbolTable *scope = sa->current_scope;
  while (scope) {
    for (SymbolTableEntry *e = scope->entries; e; e = e->next) {
      if (strcmp(e->symbol->name, name) == 0)
        return e->symbol;
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
      sa_set_error(sa, SEM_UNDEFINED_VARIABLE, node->token,
                   "name '%s' is not defined", node->token->lexeme);
      return false;
    }
    sym->dtype = sa_infer_type(sa, node);
    if (sym->dtype == UNKNOWN) {
      sa_set_error(sa, SEM_TYPE_MISMATCH, node->token,
                   "cannot infer type of variable '%s'; add a type annotation "
                   "or initialize it",
                   node->token->lexeme);
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
      Symbol *sym = sa_lookup(sa, target->token->lexeme);
      DataType rhs_type = sa_infer_type(sa, node->assign.value);

      if (!sym) {
        sym = allocator_alloc(&sa->parser.ast.allocator, sizeof(Symbol));
        sym->id = sa->next_symbol_id++;
        sym->name =
            arena_strdup(&sa->parser.ast.allocator.base, target->token->lexeme);
        sym->kind = VAR;
        sym->dtype = rhs_type;
        sym->decl_node = node;
        sym->scope_level = node->depth;
        sym->scope = sa->current_scope;
        sa_define_symbol(sa, sym);
      } else {
        // assignment to existing variable
        if (!types_compatible(sym->dtype, rhs_type)) {
          sa_set_error(
              sa, SEM_TYPE_MISMATCH, sym->decl_node->token,
              "cannot assign value of type '%s' to variable '%s' of type '%s'",
              datatype_to_string(rhs_type), sym->name,
              datatype_to_string(sym->dtype));
          return false;
        }
      }
    }
    // Analyze value
    return analyze_node(sa, node->assign.value);
  }
  case FUNCTION_DEF: {
    Symbol *sym = allocator_alloc(&sa->parser.ast.allocator, sizeof(Symbol));
    sym->name = arena_strdup(&sa->parser.ast.allocator.base,
                             node->funcdef.name->token->lexeme);
    sym->kind = FUNCTION;
    sym->decl_node = node;
    sym->scope_level = node->depth;
    sa_define_symbol(sa, sym);
    sa_enter_scope(sa);
    sym->scope = sa->current_scope;
    for (size_t cur = node->funcdef.params.head; cur != SIZE_MAX;
         cur = node->funcdef.params.elements[cur].next) {
      ASTNode *param = node->funcdef.params.elements[cur].data;
      Symbol *param_sym =
          allocator_alloc(&sa->parser.ast.allocator, sizeof(Symbol));
      param_sym->name =
          arena_strdup(&sa->parser.ast.allocator.base, param->token->lexeme);
      param_sym->kind = VAR;
      param_sym->dtype = UNKNOWN;
      param_sym->decl_node = param;
      param_sym->scope_level = sa->current_scope->depth;
      sa_define_symbol(sa, param_sym);
      param_sym->dtype = sa_infer_type(sa, param);
    }

    for (size_t cur = node->funcdef.body.head; cur != SIZE_MAX;
         cur = node->funcdef.body.elements[cur].next) {
      ASTNode *body_node = node->funcdef.body.elements[cur].data;
      if (!analyze_node(sa, body_node)) {
        return false;
      }
    }

    sym->dtype = sa_infer_type(sa, node);
    sa_exit_scope(sa);
  } break;
  case RETURN: {
    if (node->ret) {
      return analyze_node(sa, node->ret);
    }
  } break;
  case CALL: {
    Symbol *sym = sa_lookup(sa, node->call.func->token->lexeme);
    if (!sym) {
      sa_set_error(sa, SEM_UNDEFINED_VARIABLE, node->call.func->token,
                   "name '%s' is not defined", node->call.func->token->lexeme);
      return false;
    }
    if (sym->kind != FUNCTION) {
      sa_set_error(sa, SEM_INVALID_OPERATION, node->call.func->token,
                   "'%s' is not a function", sym->name);
      return false;
    }

    size_t num_args = node->call.args.size;
    size_t num_params = sym->decl_node->funcdef.params.size;

    if (num_args != num_params) {
      sa_set_error(sa, SEM_ARITY_MISMATCH, node->call.func->token,
                   "function '%s' expects %zu arguments but got %zu", sym->name,
                   num_params, num_args);
      return false;
    }

    // Validate argument types match parameter types
    for (size_t i = 0; i < num_args; i++) {
      ASTNode *arg_node =
          node->call.args.elements[node->call.args.head + i].data;
      ASTNode *param_node =
          sym->decl_node->funcdef.params
              .elements[sym->decl_node->funcdef.params.head + i]
              .data;
      DataType arg_type = sa_infer_type(sa, arg_node);
      DataType param_type = sa_infer_type(sa, param_node);

      if (!types_compatible(param_type, arg_type)) {
        sa_set_error(
            sa, SEM_TYPE_MISMATCH, node->call.func->token,
            "argument %zu to '%s' has type '%s' but parameter expects '%s'",
            i + 1, sym->name, datatype_to_string(arg_type),
            datatype_to_string(param_type));
        return false;
      }
    }

    for (size_t cur = node->call.args.head; cur != SIZE_MAX;
         cur = node->call.args.elements[cur].next) {
      ASTNode *arg_node = node->call.args.elements[cur].data;
      if (!analyze_node(sa, arg_node)) {
        return false;
      }
    }
  } break;

  case IF: {
    if (!analyze_node(sa, node->ctrl_stmt.test))
      return false;

    for (size_t cur = node->ctrl_stmt.body.head; cur != SIZE_MAX;
         cur = node->ctrl_stmt.body.elements[cur].next) {
      ASTNode *body_node = node->ctrl_stmt.body.elements[cur].data;
      if (!analyze_node(sa, body_node))
        return false;
    }

    for (size_t cur = node->ctrl_stmt.orelse.head; cur != SIZE_MAX;
         cur = node->ctrl_stmt.orelse.elements[cur].next) {
      ASTNode *else_node = node->ctrl_stmt.orelse.elements[cur].data;
      if (!analyze_node(sa, else_node))
        return false;
    }
  }
  default:
    break;
  }

  return true;
}

void sa_define_symbol(SemanticAnalyzer *sa, Symbol *sym) {
  if (!sa || !sa->current_scope) {
    slog_error("SemanticAnalyzer or current scope is NULL in sa_define_symbol");
    return;
  }

  SymbolTableEntry *entry =
      allocator_alloc(&sa->parser.ast.allocator, sizeof(SymbolTableEntry));
  entry->symbol = sym;
  entry->next = sa->current_scope->entries;
  sa->current_scope->entries = entry;
}

void sa_exit_scope(SemanticAnalyzer *sa) {
  if (!sa->current_scope)
    return;

  SymbolTable *old_scope = sa->current_scope;
  sa->current_scope = old_scope->parent;
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
      if (e->symbol->kind == FUNCTION) {
        frame = e->symbol->name;
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
  const char *source = sa->parser.lexer.source;
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
  safe_memcpy(line_content, line_len + 1, line_start, line_len);
  line_content[line_len] = 0;

  /* -----------------------------------------------------------
     Build caret underline: place '^' under the token column
     ----------------------------------------------------------- */
  size_t col = tok->col + 1;
  char *highlight = malloc(col + 1);
  memset(highlight, ' ', col - 1);
  highlight[col - 1] = '^';
  highlight[col] = 0;
  const char *filename = sa->parser.lexer.filename;
  size_t buf_size =
      strlen(filename) + strlen(line_content) + strlen(highlight) + 256;
  char *msg = allocator_alloc(&sa->parser.ast.allocator, buf_size);
  snprintf(msg, buf_size,
           "  File \"%s\", line %zu, in %s\n"
           "    %s\n"
           "    %s\n"
           "%s: %s",
           filename, line_number, frame, line_content, highlight, error_name,
           sa->last_error.detail);
  free(line_content);
  free(highlight);
  return msg;
}

void sa_enter_scope(SemanticAnalyzer *sa) {
  ASSERT(sa != NULL, "SemanticAnalyzer pointer is NULL in sa_enter_scope");
  SymbolTable *new_scope =
      symbol_table_new(&sa->parser.ast.allocator, sa->current_scope,
                       sa->current_scope ? sa->current_scope->depth + 1 : 0);
  ASSERT(new_scope != NULL, "Failed to allocate memory for new scope");
  new_scope->entries = NULL;
  new_scope->parent = sa->current_scope;
  sa->current_scope = new_scope;
}

PRINTF_FORMAT(4, 5)
void sa_set_error(SemanticAnalyzer *sa, SemanticErrorType type, Token *tok,
                  const char *fmt, ...) {
  if (!sa) {
    slog_error("SemanticAnalyzer pointer is NULL in sa_set_error");
    return;
  }

  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  sa->last_error.type = type;
  sa->last_error.token = tok;
  sa->last_error.detail = arena_strdup(&sa->parser.ast.allocator.base, buf);
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
