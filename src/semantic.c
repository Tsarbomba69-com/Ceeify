#include "semantic.h"
// TODO: Create main scope (it is different from global scope)

const char *ARITHMETIC_OPS[] = {
    "+", "-", "*", "/", "%", // arithmetic
};

const char *COMPARISON_OPS[] = {
    "==", "!=", "<", ">", "<=", ">=", // comparison
};

Symbol *sa_create_symbol(SemanticAnalyzer *sa, ASTNode *node, DataType type,
                         SymbolType kind);
DataType sa_infer_type(SemanticAnalyzer *sa, ASTNode *node);
cJSON *serialize_symbol(Symbol *sym);
cJSON *serialize_symbol_table(SymbolTable *st);
cJSON *serialize_symbol(Symbol *sym);
bool analyze_match_stmt(SemanticAnalyzer *sa, ASTNode *node);

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
  case NONE:
    return "None";
  case OBJECT:
    return "OBJECT";
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

    sa_set_error(sa, SEM_TYPE_MISMATCH, node->token,
                 "unsupported operand type(s) for %s: '%s' and '%s'",
                 node->token->lexeme, datatype_to_string(lt),
                 datatype_to_string(rt));
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

bool analyze_class_def(SemanticAnalyzer *sa, ASTNode *node) {
  node->def.name->parent = node;
  Symbol *class_sym = sa_create_symbol(sa, node->def.name, OBJECT, CLASS);
  sa_define_symbol(sa, class_sym);
  sa_enter_scope(sa);
  class_sym->scope = sa->current_scope;
  Symbol *previous_class = sa->current_class;
  sa->current_class = class_sym;

  for (size_t cur = node->def.params.head; cur != SIZE_MAX;
       cur = node->def.params.elements[cur].next) {
    ASTNode *param = node->def.params.elements[cur].data;
    Symbol *base = sa_lookup(sa, param->token->lexeme);

    if (base && base->kind == CLASS) {
      class_sym->base_class = base;
    } else {
      sa_set_error(sa, SEM_TYPE_MISMATCH, node->child->token,
                   "base class '%s' is undefined or not a class",
                   node->child->token->lexeme);
      return false;
    }
  }

  // TODO: fix code duplication for body
  for (size_t cur = node->def.body.head; cur != SIZE_MAX;
       cur = node->def.body.elements[cur].next) {
    ASTNode *body_node = node->def.body.elements[cur].data;
    if (!analyze_node(sa, body_node)) {
      return false;
    }
  }

  sa_exit_scope(sa);
  sa->current_class = previous_class;
  return true;
}

/**
 * @brief Checks if the current analysis context is inside an __init__ method.
 */
bool is_inside_constructor(SemanticAnalyzer *sa) {
  SymbolTable *st = sa->current_scope;
  while (st) {
    for (SymbolTableEntry *e = st->entries; e; e = e->next) {
      if (e->symbol->kind == FUNCTION) {
        return strcmp(e->symbol->name, "__init__") == 0;
      }
    }
    st = st->parent;
  }
  return false;
}

/**
 * @brief Determines if an ASTNode refers to the 'self' parameter of the current
 * method.
 */
bool is_self_reference(SemanticAnalyzer *sa, ASTNode *node) {
  if (!node || node->type != VARIABLE)
    return false;

  SymbolTable *st = sa->current_scope;

  while (st) {
    for (SymbolTableEntry *e = st->entries; e; e = e->next) {
      Symbol *sym = e->symbol;

      // Case 1: class → traverse its inner scope
      if (sym->kind == CLASS && sym->scope) {
        SymbolTable *class_scope = sym->scope;

        for (SymbolTableEntry *ce = class_scope->entries; ce; ce = ce->next) {
          Symbol *cs = ce->symbol;

          // Case 2: function inside class → check first param
          if (cs->kind == FUNCTION) {
            ASTNode_LinkedList *params = &cs->decl_node->parent->def.params;

            if (params->size > 0) {
              ASTNode *first_param = params->elements[params->head].data;

              if (strcmp(node->token->lexeme, first_param->token->lexeme) ==
                  0) {
                return true;
              }
            }
          }
        }
      }

      // Case 3: function in current scope (already inside class scope)
      if (sym->kind == FUNCTION && sym->base_class) {
        ASTNode_LinkedList *params = &sym->decl_node->parent->def.params;

        if (params->size > 0) {
          ASTNode *first_param = params->elements[params->head].data;

          return strcmp(node->token->lexeme, first_param->token->lexeme) == 0;
        }

        return false;
      }
    }

    // Otherwise go up one scope
    st = st->parent;
  }

  return false;
}

/**
 * @brief Registers a new attribute symbol into a class's member scope.
 */
void sa_define_member(SemanticAnalyzer *sa, Symbol *class_sym,
                      Symbol *member_sym) {
  if (!class_sym || !class_sym->scope || !member_sym)
    return;

  SymbolTableEntry *entry =
      allocator_alloc(&sa->parser.ast.allocator, sizeof(SymbolTableEntry));
  entry->symbol = member_sym;
  entry->next = class_sym->scope->entries;
  class_sym->scope->entries = entry;
}

bool analyze_func_def(SemanticAnalyzer *sa, ASTNode *node) {
  ASSERT(sa, "Semantic Analyzer context not provided");
  ASSERT(node, "Node not provided");
  node->def.name->parent = node;
  Symbol *sym = sa_create_symbol(sa, node->def.name, UNKNOWN, FUNCTION);
  sa_define_symbol(sa, sym);
  sa_enter_scope(sa);
  sym->scope = sa->current_scope;

  for (size_t cur = node->def.params.head; cur != SIZE_MAX;
       cur = node->def.params.elements[cur].next) {
    // TODO: it will need to check for static also to not set it to object
    // automatically
    ASTNode *param = node->def.params.elements[cur].data;
    Symbol *param_sym =
        allocator_alloc(&sa->parser.ast.allocator, sizeof(Symbol));
    param_sym->name =
        arena_strdup(&sa->parser.ast.allocator.base, param->token->lexeme);
    param_sym->kind = VAR;
    param_sym->dtype = UNKNOWN;
    param_sym->decl_node = param;
    param_sym->scope_level = sa->current_scope->depth;

    if (sa->current_class && cur == 0) {
      param_sym->dtype = OBJECT;
      param_sym->base_class = sa->current_class;
    } else {
      param_sym->dtype = sa_infer_type(sa, param);
    }

    sa_define_symbol(sa, param_sym);
  }

  for (size_t cur = node->def.body.head; cur != SIZE_MAX;
       cur = node->def.body.elements[cur].next) {
    ASTNode *body_node = node->def.body.elements[cur].data;
    if (!analyze_node(sa, body_node)) {
      return false;
    }
  }

  sym->dtype = sa_infer_type(sa, node);
  sa_exit_scope(sa);
  return true;
}

DataType sa_infer_type(SemanticAnalyzer *sa, ASTNode *node) {
  ASSERT(sa != NULL, "SemanticAnalyzer cannot be NULL in sa_infer_type");
  if (!node)
    return NONE;

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
    if (node->child) {
      return string_to_datatype(node->child->token->lexeme);
    }

    DataType dtype = string_to_datatype(node->token->lexeme);

    if (is_self_reference(sa, node)) {
      return OBJECT;
    }

    if (dtype != UNKNOWN) {
      return dtype;
    }

    Symbol *sym = resolve_symbol(sa, node);
    if (sym) {
      return sym->dtype;
    } else {
      sa_set_error(sa, SEM_UNDEFINED_VARIABLE, node->token,
                   "name '%s' is not defined", node->token->lexeme);
      return UNKNOWN;
    }
  } break;
  case FUNCTION_DEF: {
    DataType ret_type = NONE;

    for (size_t cur = node->def.body.head; cur != SIZE_MAX;
         cur = node->def.body.elements[cur].next) {
      ASTNode *body_node = node->def.body.elements[cur].data;
      ret_type = sa_infer_type(sa, body_node->child);
    }

    if (node->def.returns) {
      DataType annotation_type = sa_infer_type(sa, node->def.returns);
      if (ret_type != annotation_type) {
        sa_set_error(sa, SEM_TYPE_MISMATCH, node->def.returns->token,
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
  case CALL: {
    Symbol *sym = sa_lookup(sa, node->call.func->token->lexeme);
    if (sym) {
      return sym->dtype;
    } else {
      sa_set_error(sa, SEM_UNDEFINED_VARIABLE, node->call.func->token,
                   "name '%s' is not defined", node->call.func->token->lexeme);
      return UNKNOWN;
    }
  } break;

  case ATTRIBUTE: {
    Symbol *obj_sym = sa_lookup(sa, node->attribute.value->token->lexeme);
    Symbol *class_sym = (obj_sym) ? obj_sym->base_class : NULL;
    Symbol *member = sa_lookup_member(class_sym, node->attribute.attr);
    return member ? member->dtype : UNKNOWN;
  } break;
  default:
    return NONE;
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

Symbol *sa_lookup_local(SemanticAnalyzer *sa, const char *name) {
  SymbolTable *scope = sa->current_scope;

  for (SymbolTableEntry *e = scope->entries; e; e = e->next) {
    if (strcmp(e->symbol->name, name) == 0)
      return e->symbol;
  }

  return NULL;
}

bool analyze_node(SemanticAnalyzer *sa, ASTNode *node) {
  if (!sa || !node)
    return true;

  switch (node->type) {
  case VARIABLE: {
    Symbol *sym;

    if (node->ctx == STORE && node->parent && node->parent->type == CLASS_DEF) {
      sym = sa_create_symbol(sa, node, sa_infer_type(sa, node), VAR);
      sa_define_symbol(sa, sym);
      return true;
    }

    sym = sa_lookup(sa, node->token->lexeme);

    if (!sym) {
      sa_set_error(sa, SEM_UNDEFINED_VARIABLE, node->token,
                   "name '%s' is not defined", node->token->lexeme);
      return false;
    }

    sym->dtype = sa_infer_type(sa, node);

    if (sym->dtype == UNKNOWN && !is_self_reference(sa, node)) {
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
      Symbol *local_sym = sa_lookup_local(sa, target->token->lexeme);
      DataType rhs_type = sa_infer_type(sa, node->assign.value);

      if (rhs_type == UNKNOWN) {
        return false;
      }

      if (target->type == ATTRIBUTE) {
        if (!analyze_node(sa, target))
          return false;

        continue;
      }

      if (target->child) {
        if (local_sym) {
          sa_set_error(sa, SEM_REDECLARATION, target->token,
                       "variable '%s' already declared in this scope",
                       target->token->lexeme);
          return false;
        }

      define_sym:
        sym = sa_create_symbol(sa, target, rhs_type, VAR);
        sa_define_symbol(sa, sym);
        sym->dtype =
            target->type == VARIABLE && target->child ? UNKNOWN : rhs_type;

        if (!analyze_node(sa, target)) {
          return false;
        }
      } else if (!sym) {
        goto define_sym;
      } else if (sym && !types_compatible(sym->dtype, rhs_type)) {
        // Type compatibility check
        sa_set_error(
            sa, SEM_TYPE_MISMATCH, sym->decl_node->token,
            "cannot assign value of type '%s' to variable '%s' of type '%s'",
            datatype_to_string(rhs_type), sym->name,
            datatype_to_string(sym->dtype));
        return false;
      }
    }
    // Analyze value
    return analyze_node(sa, node->assign.value);
  }
  case CLASS_DEF: {
    if (!analyze_class_def(sa, node))
      return false;
  } break;
  case FUNCTION_DEF: {
    if (!analyze_func_def(sa, node))
      return false;
  } break;
  case RETURN: {
    if (node->child) {
      return analyze_node(sa, node->child);
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
    size_t num_params = sym->decl_node->parent->def.params.size;

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
          sym->decl_node->parent->def.params
              .elements[sym->decl_node->parent->def.params.head + i]
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

  case WHILE:
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
  } break;
  case ATTRIBUTE: {
    if (!analyze_node(sa, node->attribute.value))
      return false;

    DataType base_dtype = sa_infer_type(sa, node->attribute.value);
    Symbol *obj_sym = sa_lookup(sa, node->attribute.value->token->lexeme);
    Symbol *class_sym =
        (obj_sym && obj_sym->dtype == OBJECT) ? obj_sym->base_class : NULL;
    Symbol *member = sa_lookup_member(class_sym, node->attribute.attr);

    if (node->ctx == LOAD) {
      if (!member) {
        sa_set_error(sa, SEM_UNDEFINED_VARIABLE, node->token,
                     "Object of type '%s' has no attribute '%s'",
                     datatype_to_string(base_dtype), node->attribute.attr);
        return false;
      }
    } else if (node->ctx == STORE) {
      if (!member) {
        if (is_inside_constructor(sa) &&
            is_self_reference(sa, node->attribute.value)) {
          DataType inferred = sa_infer_type(sa, node->parent->assign.value);
          char *inferred_str = (char *)datatype_to_string(inferred);
          Token *tok_var = create_token_from_str(
              &sa->parser.lexer, node->token->lexeme, IDENTIFIER);
          tok_var->ident = node->parent->parent->token->ident;
          tok_var->line = node->token->line;
          tok_var->col = node->token->col;
          ASTNode *var = node_new(&sa->parser, tok_var, VARIABLE);
          var->ctx = STORE;
          Token *t = create_token_from_str(&sa->parser.lexer, inferred_str,
                                           IDENTIFIER);
          t->ident = node->parent->parent->token->ident;
          t->line = node->token->line;
          t->col = node->token->col + 1;
          var->child = node_new(&sa->parser, t, VARIABLE);
          Symbol *new_attr = sa_create_symbol(sa, var, inferred, VAR);
          ASTNode_add_last(&class_sym->decl_node->parent->def.body, var);
          sa_define_member(sa, class_sym, new_attr);
        } else {
          sa_set_error(sa, SEM_INVALID_OPERATION, node->token,
                       "Cannot create new attribute '%s' on type '%s' outside "
                       "constructor",
                       node->attribute.attr, datatype_to_string(base_dtype));
          return false;
        }
      }
    }
  } break;
  case MATCH:
    return analyze_match_stmt(sa, node);
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

char *error_to_string(SemanticErrorType type) {
  switch (type) {
  case SEM_OK:
    return "Ok";
  case SEM_UNDEFINED_VARIABLE:
    return "NameError";
  case SEM_TYPE_MISMATCH:
    return "TypeError";
  case SEM_INVALID_OPERATION:
    return "TypeError";
  case SEM_UNREACHABLE_PATTERN:
    return "SyntaxError";
  default:
    return "SemanticError";
  }
}

char *sa_format_error(SemanticAnalyzer *sa) {
  if (!sa || sa->last_error.type == SEM_OK || !sa->last_error.token) {
    slog_error("No error to format in sa_format_error");
    return NULL;
  }

  Token *tok = sa->last_error.token;
  SemanticErrorType type = sa->last_error.type;
  const char *error_name = error_to_string(type);

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
  size_t col = tok->col;
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

Symbol *sa_create_symbol(SemanticAnalyzer *sa, ASTNode *node, DataType type,
                         SymbolType kind) {
  Symbol *sym = allocator_alloc(&sa->parser.ast.allocator, sizeof(Symbol));
  sym->id = sa->next_symbol_id++;
  sym->name = arena_strdup(&sa->parser.ast.allocator.base, node->token->lexeme);
  sym->kind = kind;
  sym->dtype = type;
  sym->decl_node = node;
  sym->scope_level = sa->current_scope ? sa->current_scope->depth : 0;
  sym->scope = NULL;
  sym->base_class = NULL;
  return sym;
}

/**
 * @brief Recursively searches for an attribute in a class and its base classes.
 */
Symbol *sa_lookup_member(Symbol *class_sym, const char *name) {
  if (!class_sym || class_sym->kind != CLASS)
    return NULL;

  // 1. Check current class scope
  SymbolTable *st = class_sym->scope;
  for (SymbolTableEntry *e = st->entries; e; e = e->next) {
    if (strcmp(e->symbol->name, name) == 0)
      return e->symbol;
  }

  // 2. Recursive check in base class
  if (class_sym->base_class) {
    return sa_lookup_member(class_sym->base_class, name);
  }

  return NULL;
}

const char *symbol_kind_to_string(SymbolType kind) {
  switch (kind) {
  case CLASS:
    return "CLASS";
  case MODULE:
    return "MODULE";
  case FUNCTION:
    return "FUNCTION";
  case BLOCK:
    return "BLOCK";
  case VAR:
    return "VAR";
  default:
    return "UNKNOWN";
  }
}

cJSON *serialize_symbol(Symbol *sym) {
  if (sym == NULL)
    return NULL;

  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "name", sym->name);
  cJSON_AddStringToObject(root, "kind", symbol_kind_to_string(sym->kind));
  cJSON_AddStringToObject(root, "dtype", datatype_to_string(sym->dtype));
  cJSON_AddNumberToObject(root, "scope_level", sym->scope_level);

  // If it's a class with a base class, record the name to avoid circular
  // recursion
  if (sym->base_class) {
    cJSON_AddStringToObject(root, "base_class", sym->base_class->name);
  }

  // Recursively serialize nested scopes (for functions and classes)
  if (sym->scope) {
    cJSON_AddItemToObject(root, "scope", serialize_symbol_table(sym->scope));
  }

  return root;
}

cJSON *serialize_symbol_table(SymbolTable *st) {
  if (st == NULL)
    return NULL;

  cJSON *root = cJSON_CreateObject();
  cJSON_AddNumberToObject(root, "depth", st->depth);

  cJSON *entries = cJSON_CreateArray();
  SymbolTableEntry *curr = st->entries;
  while (curr) {
    cJSON_AddItemToArray(entries, serialize_symbol(curr->symbol));
    curr = curr->next;
  }
  cJSON_AddItemToObject(root, "entries", entries);

  return root;
}

char *dump_symbol_table(SymbolTable *st) {
  cJSON *json = serialize_symbol_table(st);
  char *dump = cJSON_Print(json);
  cJSON_Delete(json);
  return dump;
}

Symbol *resolve_symbol(SemanticAnalyzer *sa, ASTNode *node) {
  if (node->parent && node->parent->type == CLASS_DEF) {
    Symbol *class_sym = sa_lookup(sa, node->parent->def.name->token->lexeme);
    Symbol *sym = sa_lookup_member(class_sym, node->token->lexeme);
    return sym;
  }

  return sa_lookup(sa, node->token->lexeme);
}

Symbol *find_enclosing_class(SemanticAnalyzer *sa) {
  SymbolTable *st = sa->current_scope;

  while (st) {
    for (SymbolTableEntry *e = st->entries; e; e = e->next) {
      if (e->symbol->kind == CLASS)
        return e->symbol;
    }
    st = st->parent;
  }
  return NULL;
}

static bool class_has_field(Symbol *cls, const char *name) {
  if (!cls || !cls->scope)
    return false;

  for (SymbolTableEntry *e = cls->scope->entries; e; e = e->next) {
    if (e->symbol->kind == VAR && strcmp(e->symbol->name, name) == 0) {
      return true;
    }
  }
  return false;
}

static Symbol *resolve_base_class(SemanticAnalyzer *sa, Symbol *cls) {
  if (!cls || !cls->base_class)
    return NULL;

  SymbolTable *st = sa->current_scope;
  while (st) {
    for (SymbolTableEntry *e = st->entries; e; e = e->next) {
      if (e->symbol->kind == CLASS &&
          strcmp(e->symbol->name, cls->base_class->name) == 0) {
        return e->symbol;
      }
    }
    st = st->parent;
  }
  return NULL;
}

AttrOwnership resolve_attribute_owner(SemanticAnalyzer *sa,
                                      ASTNode *attr_node) {
  if (!sa || !attr_node || attr_node->type != ATTRIBUTE)
    return ATTR_OWN_CURRENT;

  const char *attr = attr_node->attribute.attr;

  // Rule 1: assignment ALWAYS creates / writes on current class
  if (attr_node->type == ASSIGNMENT) {
    return ATTR_OWN_CURRENT;
  }

  // Rule 2: resolve by class layout
  Symbol *cls = find_enclosing_class(sa);
  if (!cls)
    return ATTR_OWN_CURRENT;

  // Field defined on current class?
  if (class_has_field(cls, attr))
    return ATTR_OWN_CURRENT;

  // Field inherited from base?
  Symbol *base = resolve_base_class(sa, cls);
  if (class_has_field(base, attr))
    return ATTR_OWN_BASE;

  return ATTR_OWN_CURRENT;
}

bool analyze_match_stmt(SemanticAnalyzer *sa, ASTNode *node) {
  if (!analyze_node(sa, node->ctrl_stmt.test))
    return false;

  for (size_t cur = node->ctrl_stmt.body.head; cur != SIZE_MAX;
       cur = node->ctrl_stmt.body.elements[cur].next) {
    ASTNode *case_node = node->ctrl_stmt.body.elements[cur].data;

    if (!analyze_node(sa, case_node))
      return false;

    ASTNode *pat = case_node->ctrl_stmt.test;
    if ((pat->type == VARIABLE && strcmp(pat->token->lexeme, "_") == 0) ||
        pat->type == VARIABLE) {
      sa_set_error(sa, SEM_UNREACHABLE_PATTERN, pat->token,
                   "wildcard makes remaining patterns unreachable");
      slog_info("%s", sa->last_error.message);
      return false;
    }
  }
  UNREACHABLE("MATCH statement analysis not implemented yet");
  return false;
}