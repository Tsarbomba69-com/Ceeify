#include "semantic.h"

static SymbolTable symbol_table_new(SymbolTable *parent, size_t depth) {
  SymbolTable st = {.depth = depth, .parent = parent, .entries = NULL};
  return st;
}

SemanticAnalyzer analyze_program(ASTNode_LinkedList *program) {
  SemanticAnalyzer sa = {0};
  SymbolTable global_scope = symbol_table_new(NULL, 0);
  sa.current_scope = &global_scope;
  sa.allocator = &program->allocator;
  sa.last_error.type = SEM_OK;

  // Iterate over all top-level AST Nodes
  ASTNode_LinkedList *curr = program;
  ASTNode *node = NULL;
  do {
    node = ASTNode_pop(curr);
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
      sa_set_error(sa, SEM_UNDEFINED_VARIABLE, node->token,
                   "Undefined variable");
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
      Symbol sym = {
          .name = arena_strdup(&sa->allocator->base, target->token->lexeme),
          .kind = VAR,
          .dtype = node->assign.value->type == LITERAL
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
      allocator_alloc(sa->allocator, sizeof(SymbolTableEntry));
  entry->symbol = sym;
  entry->next = sa->current_scope->entries;
  sa->current_scope->entries = entry;
}

bool sa_has_error(SemanticAnalyzer *sa) {
  return sa && sa->last_error.type != SEM_OK;
}

void sa_set_error(SemanticAnalyzer *sa, SemanticErrorType type, Token *tok,
                  const char *msg) {
  if (!sa) {
    slog_error("SemanticAnalyzer pointer is NULL in sa_set_error");
    return;
  }
  sa->last_error.type = type;
  sa->last_error.token = tok;
  sa->last_error.message = msg;
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