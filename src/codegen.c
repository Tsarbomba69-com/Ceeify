#include "codegen.h"

/* -----------------------------
 *  INTERNAL API
 * ----------------------------- */

void gen_function_def(Codegen *cg, ASTNode *node, const char *prefix,
                      const char *self_type);

void gen_ctrl_flow(Codegen *cg, ASTNode *node);

void gen_match_stmt(Codegen *cg, ASTNode *node);

/* -----------------------------
 *  CODEGEN IMPLEMENTATION
 * ----------------------------- */

// NULL means no substitution
typedef struct {
  const char *from; // original variable name (e.g. "x")
  const char *to;   // replacement (e.g. "_tmp0")
} VarSubst;

static void gen_expr(Codegen *cg, ASTNode *node, VarSubst *subst);

int8_t get_node_precedence(ASTNode *node) {
  if (node == NULL)
    return 0;

  if (node->type == BINARY_OPERATION) {
    return get_infix_precedence(node->token->lexeme);
  }

  if (node->type == UNARY_OPERATION) {
    return get_prefix_precedence(node->token->lexeme);
  }

  return 127;
}

const char *ctype_to_string(Codegen *cg, ASTNode *node) {
  DataType t = sa_infer_type(&cg->sa, node);
  switch (t) {
  case INT:
    return "int";
  case STR:
    return "char*";
  case BOOL:
    return "bool";
  case FLOAT:
    return "float";
  case LIST:
    return "list";
  case NONE:
    return "void";
  case OBJECT: {
    Symbol *obj_sym = sa_lookup(&cg->sa, node->token->lexeme);
    Symbol *class_sym =
        (obj_sym && obj_sym->dtype == OBJECT) ? obj_sym->base_class : NULL;
    return class_sym ? class_sym->base_class->name : "void*";
  } break;
  default:
    return "<unknown>";
  }
}

static const char *py_op_to_c_op(const char *op) {
  if (strcmp(op, "and") == 0)
    return "&&";
  if (strcmp(op, "or") == 0)
    return "||";
  if (strcmp(op, "not") == 0)
    return "!";
  if (strcmp(op, "//") == 0)
    return "/"; // integer division (assumes int operands)
  if (strcmp(op, "**") == 0)
    return NULL; // no C equivalent, handle separately
  return op;     // +, -, *, /, %, ==, !=, <, >, <=, >= pass through unchanged
}

Codegen codegen_init(SemanticAnalyzer *sa) {
  Codegen cg;
  cg.sa = *sa;
  cg.output = sb_init(&sa->parser.ast.allocator, DEFAULT_CAP);
  cg.last_error.type = CG_OK;
  cg.last_error.message = NULL;
  cg.last_error.node = NULL;
  return cg;
}

bool gen_code(Codegen *cg, ASTNode *node) {
  switch (node->type) {
  case FUNCTION_DEF:
    gen_function_def(cg, node, NULL, NULL);
    break;
  case CALL: {
    bool saved_standalone = cg->is_standalone;
    cg->is_standalone = false;
    sb_appendf(&cg->output, "%s(", node->call.func->token->lexeme);
    for (size_t cur = node->call.args.head; cur != SIZE_MAX;
         cur = node->call.args.elements[cur].next) {
      ASTNode *arg = node->call.args.elements[cur].data;
      gen_code(cg, arg);
      if (cur != node->call.args.tail) {
        sb_appendf(&cg->output, ", ");
      }
    }
    sb_appendf(&cg->output, ")");

    cg->is_standalone = saved_standalone;
    if (cg->is_standalone) {
      sb_appendf(&cg->output, ";\n");
    }
  } break;
  case RETURN: {
    sb_appendf(&cg->output, "    return ");
    ASTNode *ret_expr = node->child;
    cg->is_standalone = false;
    gen_code(cg, ret_expr);
    sb_appendf(&cg->output, ";\n");
  } break;
  case VARIABLE: {
    gen_expr(cg, node, NULL);
    if (cg->is_standalone) {
      sb_appendf(&cg->output, ";\n");
    }
    cg->is_standalone = false;
  } break;
  case LITERAL: {
    gen_expr(cg, node, NULL);
    if (cg->is_standalone) {
      sb_appendf(&cg->output, ";\n");
    }
  } break;
  case BINARY_OPERATION: {
    bool saved = cg->is_standalone;
    cg->is_standalone = false;
    gen_expr(cg, node, NULL);
    cg->is_standalone = saved;
    if (cg->is_standalone) {
      sb_appendf(&cg->output, ";\n");
    }
  } break;
  case CLASS_DEF: {
    const char *class_name = node->def.name->token->lexeme;
    sb_appendf(&cg->output, "typedef struct {\n");

    // Handle Inheritance (Composition)
    if (node->def.params.size == 1) {
      ASTNode *base = ASTNode_pop(&node->def.params);
      sb_appendf(&cg->output, "  %s* base;\n", base->token->lexeme);
    } else {
      for (size_t cur = node->def.params.head; cur != SIZE_MAX;
           cur = node->def.params.elements[cur].next) {
        ASTNode *base = node->call.args.elements[cur].data;
        sb_appendf(&cg->output, "  %s* base%zu;\n", base->token->lexeme, cur);
      }
    }

    ASTNode_LinkedList methods =
        ASTNode_new_with_allocator(&cg->sa.parser.ast.allocator, DEFAULT_CAP);

    for (size_t cur = node->def.body.head; cur != SIZE_MAX;
         cur = node->def.body.elements[cur].next) {
      ASTNode *member = node->def.body.elements[cur].data;
      if (member->type == FUNCTION_DEF) {
        ASTNode_add_first(&methods, member);
        cg->is_standalone = true;
        continue;
      } else {
        sb_append_padding(&cg->output, ' ', member->token->ident);
        gen_code(cg, member);
      }
    }

    sb_appendf(&cg->output, "} %s;\n\n", class_name);

    for (size_t cur = methods.head; cur != SIZE_MAX;
         cur = methods.elements[cur].next) {
      ASTNode *method = methods.elements[cur].data;
      gen_function_def(cg, method, class_name, class_name);
    }
  } break;
  case ASSIGNMENT: {
    sb_append_padding(&cg->output, ' ', node->token->ident);
    for (size_t cur = node->assign.targets.head; cur != SIZE_MAX;
         cur = node->assign.targets.elements[cur].next) {
      ASTNode *target = node->assign.targets.elements[cur].data;
      cg->is_standalone = false;
      gen_code(cg, target);
      sb_appendf(&cg->output, " = ");
      cg->is_standalone = true;
      gen_code(cg, node->assign.value);
    }
  } break;
  case ATTRIBUTE: {
    gen_code(cg, node->attribute.value);
    AttrOwnership owner = resolve_attribute_owner(&cg->sa, node);

    if (owner == ATTR_OWN_BASE) {
      sb_appendf(&cg->output, "->base->%s", node->attribute.attr);
    } else {
      sb_appendf(&cg->output, "->%s", node->attribute.attr);
    }
  } break;
  case IF: {
    sb_append_padding(&cg->output, ' ', node->token->ident);
    sb_appendf(&cg->output, "if (");
    cg->is_standalone = false;
    gen_code(cg, node->ctrl_stmt.test);
    cg->is_standalone = true;
    sb_appendf(&cg->output, ") {\n");

    for (size_t cur = node->ctrl_stmt.body.head; cur != SIZE_MAX;
         cur = node->ctrl_stmt.body.elements[cur].next) {
      ASTNode *stmt = node->ctrl_stmt.body.elements[cur].data;
      gen_code(cg, stmt);
    }

    sb_append_padding(&cg->output, ' ', node->token->ident);

    if (node->ctrl_stmt.orelse.size > 0) {
      sb_appendf(&cg->output, "}");
      ASTNode *last =
          node->ctrl_stmt.orelse.elements[node->ctrl_stmt.orelse.head].data;
      sb_append_padding(&cg->output, ' ',
                        last->type == IF ? 0 : node->token->ident);
      sb_appendf(&cg->output, last->type == IF ? "else " : "else {\n");
      for (size_t cur = node->ctrl_stmt.orelse.head; cur != SIZE_MAX;
           cur = node->ctrl_stmt.orelse.elements[cur].next) {
        ASTNode *stmt = node->ctrl_stmt.orelse.elements[cur].data;
        gen_code(cg, stmt);
      }

      sb_append_padding(&cg->output, ' ', node->token->ident);
      sb_appendf(&cg->output, "}\n");
    } else {
      sb_appendf(&cg->output, "}\n");
    }
  } break;
  case WHILE:
    gen_ctrl_flow(cg, node);
    break;
  case COMPARE: {
    bool saved = cg->is_standalone;
    cg->is_standalone = false;

    // Python comparison chaining: a < b < c  => (a < b && b < c)
    // We'll use parentheses to ensure C precedence doesn't break logic
    bool chained = node->compare.comparators.size > 1;
    if (chained)
      sb_appendf(&cg->output, "(");

    ASTNode *left_side = node->compare.left;

    size_t op_idx = 0;
    for (size_t cur = node->compare.comparators.head; cur != SIZE_MAX;
         cur = node->compare.comparators.elements[cur].next) {

      if (op_idx > 0) {
        sb_appendf(&cg->output, " && ");
      }

      // Generate the sub-expression: left_side OP right_side
      gen_code(cg, left_side);

      Token *op = node->compare.ops.elements[op_idx];
      // Map Python '==' to C '==', 'is' to '==', etc.
      const char *c_op = op->lexeme;
      if (strcmp(c_op, "is") == 0)
        c_op = "==";

      sb_appendf(&cg->output, " %s ", c_op);

      ASTNode *right_side = node->compare.comparators.elements[cur].data;
      gen_code(cg, right_side);

      // In chained comparison, the right of the current becomes the left of the
      // next
      left_side = right_side;
      op_idx++;
    }

    if (chained)
      sb_appendf(&cg->output, ")");

    cg->is_standalone = saved;
    if (cg->is_standalone) {
      sb_appendf(&cg->output, ";\n");
    }
  } break;
  case MATCH:
    gen_match_stmt(cg, node);
    break;
  default:
    slog_fatal("Code generation for \"%s\" type is not implemented yet",
               node_type_to_string(node->type));
    break;
  }
  return true;
}

void codegen_free(Codegen *cg) {
  cg->output.items = NULL;
  cg->output.count = 0;
  cg->last_error.message = NULL;
  parser_free(&cg->sa.parser);
}

bool codegen_program(Codegen *cg) {
  ASSERT(cg != NULL, "Codegen context cannot be NULL");
  ASTNode_LinkedList *program = &cg->sa.parser.ast;

  for (size_t current = program->head; current != SIZE_MAX;
       current = program->elements[current].next) {
    ASTNode *node = program->elements[current].data;
    cg->is_standalone = true;

    if (!gen_code(cg, node))
      return false;
  }

  return true;
}

bool codegen_has_error(Codegen *cg) { return cg->last_error.type != CG_OK; }

CodegenError codegen_get_error(Codegen *cg) { return cg->last_error; }

void gen_function_def(Codegen *cg, ASTNode *node, const char *prefix,
                      const char *self_type) {
  // 1. Return Type
  sb_appendf(&cg->output, "%s ", ctype_to_string(cg, node->def.returns));

  // 2. Name (with optional prefix for methods)
  if (prefix) {
    sb_appendf(&cg->output, "%s_%s(", prefix, node->def.name->token->lexeme);
  } else {
    sb_appendf(&cg->output, "%s(", node->def.name->token->lexeme);
  }

  // 3. Parameters
  if (node->def.params.size == 0 && !self_type) {
    sb_appendf(&cg->output, "void");
  }

  for (size_t cur = node->def.params.head; cur != SIZE_MAX;
       cur = node->def.params.elements[cur].next) {
    ASTNode *param = node->def.params.elements[cur].data;

    // If this is the first param and we have a self_type override
    if (cur == node->def.params.head && self_type) {
      sb_appendf(&cg->output, "%s* %s", self_type, param->token->lexeme);
    } else {
      const char *p_type = ctype_to_string(cg, param);
      sb_appendf(&cg->output, "%s %s", p_type, param->token->lexeme);
    }

    if (cur != node->def.params.tail) {
      sb_appendf(&cg->output, ", ");
    }
  }
  sb_appendf(&cg->output, ") {\n");
  Symbol *fun_sym = sa_lookup(&cg->sa, node->def.name->token->lexeme);
  cg->sa.current_scope = fun_sym ? fun_sym->scope : cg->sa.current_scope;
  // 4. Body
  for (size_t cur = node->def.body.head; cur != SIZE_MAX;
       cur = node->def.body.elements[cur].next) {
    ASTNode *body_node = node->def.body.elements[cur].data;
    cg->is_standalone = true;
    gen_code(cg, body_node);
  }

  sb_appendf(&cg->output, "}\n");
  sa_exit_scope(&cg->sa);
}

void gen_ctrl_flow(Codegen *cg, ASTNode *node) {
  ASSERT(cg, "Codegen context cannot be NULL");
  ASSERT(node, "Node cannot be null");

  sb_append_padding(&cg->output, ' ', node->token->ident);

  sb_appendf(&cg->output, "while (");
  cg->is_standalone = false;
  gen_code(cg, node->ctrl_stmt.test);
  cg->is_standalone = true;
  sb_appendf(&cg->output, ") {\n");

  for (size_t cur = node->ctrl_stmt.body.head; cur != SIZE_MAX;
       cur = node->ctrl_stmt.body.elements[cur].next) {
    ASTNode *stmt = node->ctrl_stmt.body.elements[cur].data;
    gen_code(cg, stmt);
  }

  sb_append_padding(&cg->output, ' ', node->token->ident);
  sb_appendf(&cg->output, "}\n");
}

void gen_match_stmt(Codegen *cg, ASTNode *node) {
  ASSERT(cg, "Codegen context cannot be NULL");
  ASSERT(node, "Node cannot be null");
  ASTNode *scrutinee = node->ctrl_stmt.test;
  bool first = true;
  static uint8_t match_depth = 0;
  int current_tmp_id = match_depth++;

  // 1. Generate the temporary variable for the scrutinee
  sb_append_padding(&cg->output, ' ', node->token->ident);
  sb_appendf(&cg->output, "%s _tmp%d = ", ctype_to_string(cg, scrutinee),
             current_tmp_id);

  bool saved_standalone = cg->is_standalone;
  cg->is_standalone = false;
  gen_code(cg, scrutinee);
  sb_appendf(&cg->output, ";\n");

  // 2. Iterate through cases
  for (size_t cur = node->ctrl_stmt.body.head; cur != SIZE_MAX;
       cur = node->ctrl_stmt.body.elements[cur].next) {

    ASTNode *case_node = node->ctrl_stmt.body.elements[cur].data;
    size_t pattern_idx = case_node->ctrl_stmt.orelse.head;
    ASTNode *pattern = case_node->ctrl_stmt.orelse.elements[pattern_idx].data;

    // Check for a guard condition on this case node
    ASTNode *guard = case_node->ctrl_stmt.test;

    sb_append_padding(&cg->output, ' ', node->token->ident);

    bool is_wildcard =
        (pattern->type == VARIABLE && strcmp(pattern->token->lexeme, "_") == 0);
    bool is_capture =
        (pattern->type == VARIABLE && strcmp(pattern->token->lexeme, "_") != 0);

    // Header: if / else if / else
    char tmp_name[16];
    snprintf(tmp_name, sizeof(tmp_name), "_tmp%d", current_tmp_id);

    if (guard != NULL) {
      // A guard overrides the normal pattern match — emit if/else if with guard
      const char *branch = first ? "if" : "else if";
      sb_appendf(&cg->output, "%s (", branch);
      cg->is_standalone = false;
      VarSubst subst = {scrutinee->token->lexeme, tmp_name};
      gen_expr(cg, guard, &subst);
      sb_appendf(&cg->output, ") {\n");
      first = false;
    } else if (is_wildcard || is_capture) {
      sb_appendf(&cg->output, "else {\n");
    } else {
      const char *branch = first ? "if" : "else if";
      sb_appendf(&cg->output, "%s (_tmp%d == ", branch, current_tmp_id);
      cg->is_standalone = false;
      gen_code(cg, pattern);
      sb_appendf(&cg->output, ") {\n");
      first = false;
    }

    // 3. Body: Handle variable capture assignment
    if (is_capture) {
      sb_append_padding(&cg->output, ' ', node->token->ident + 4);
      sb_appendf(&cg->output, "%s %s = _tmp%d;\n",
                 ctype_to_string(cg, scrutinee), pattern->token->lexeme,
                 current_tmp_id);
    }

    // 4. Body: Generate statements
    for (size_t bcur = case_node->ctrl_stmt.body.head; bcur != SIZE_MAX;
         bcur = case_node->ctrl_stmt.body.elements[bcur].next) {
      ASTNode *stmt = case_node->ctrl_stmt.body.elements[bcur].data;
      sb_append_padding(&cg->output, ' ', node->token->ident + 4);
      cg->is_standalone = true;
      gen_code(cg, stmt);
    }

    sb_append_padding(&cg->output, ' ', node->token->ident);
    sb_appendf(&cg->output, "}\n");
  }

  cg->is_standalone = saved_standalone;
  match_depth--;
}

static void gen_expr(Codegen *cg, ASTNode *node, VarSubst *subst) {
  if (node == NULL)
    return;

  switch (node->type) {
  case VARIABLE: {
    const char *name = node->token->lexeme;
    // Apply substitution if provided and name matches
    if (subst && strcmp(name, subst->from) == 0) {
      sb_appendf(&cg->output, "%s", subst->to);
      break;
    }
    // Normal variable emit — check for definition vs usage
    Symbol *var_sym = sa_lookup(&cg->sa, name);
    Symbol *class_sym = var_sym ? NULL : find_enclosing_class(&cg->sa);
    var_sym =
        !var_sym && class_sym ? sa_lookup_member(class_sym, name) : var_sym;
    if (var_sym && ((node->ctx == STORE && var_sym->decl_node == node) ||
                    var_sym->base_class)) {
      sb_appendf(&cg->output, "%s %s", ctype_to_string(cg, node), name);
    } else {
      sb_appendf(&cg->output, "%s", name);
    }
  } break;

  case LITERAL:
    if (node->token->type == STRING) {
      sb_appendf(&cg->output, "\"%s\"", node->token->lexeme);
    } else {
      sb_appendf(&cg->output, "%s", node->token->lexeme);
    }
    break;

  case BINARY_OPERATION: {
    int8_t current_prec = get_infix_precedence(node->token->lexeme);
    int8_t left_prec = get_node_precedence(node->bin_op.left);
    int8_t right_prec = get_node_precedence(node->bin_op.right);

    if (left_prec < current_prec) {
      sb_appendf(&cg->output, "(");
      gen_expr(cg, node->bin_op.left, subst);
      sb_appendf(&cg->output, ")");
    } else {
      gen_expr(cg, node->bin_op.left, subst);
    }

    const char *c_op = py_op_to_c_op(node->token->lexeme);
    if (c_op == NULL) {
      ASSERT(false, "Operator '**' not supported in codegen");
    }
    sb_appendf(&cg->output, " %s ", c_op);

    if (right_prec <= current_prec) {
      sb_appendf(&cg->output, "(");
      gen_expr(cg, node->bin_op.right, subst);
      sb_appendf(&cg->output, ")");
    } else {
      gen_expr(cg, node->bin_op.right, subst);
    }
  } break;

  case COMPARE: {
    bool chained = node->compare.comparators.size > 1;
    if (chained)
      sb_appendf(&cg->output, "(");

    ASTNode *left_side = node->compare.left;
    size_t op_idx = 0;
    for (size_t cur = node->compare.comparators.head; cur != SIZE_MAX;
         cur = node->compare.comparators.elements[cur].next) {
      if (op_idx > 0)
        sb_appendf(&cg->output, " && ");

      gen_expr(cg, left_side, subst);

      Token *op = node->compare.ops.elements[op_idx];
      const char *c_op = op->lexeme;
      if (strcmp(c_op, "is") == 0)
        c_op = "==";
      sb_appendf(&cg->output, " %s ", c_op);

      ASTNode *right_side = node->compare.comparators.elements[cur].data;
      gen_expr(cg, right_side, subst);

      left_side = right_side;
      op_idx++;
    }

    if (chained)
      sb_appendf(&cg->output, ")");
  } break;

  default:
    // Fallback for any expression type not handled above
    gen_code(cg, node);
    break;
  }
}