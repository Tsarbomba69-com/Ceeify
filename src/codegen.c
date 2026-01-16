#include "codegen.h"

/* -----------------------------
 *  INTERNAL API
 * ----------------------------- */

void gen_function_def(Codegen *cg, ASTNode *node, const char *prefix,
                      const char *self_type);

void gen_ctrl_flow(Codegen *cg, ASTNode *node);

/* -----------------------------
 *  CODEGEN IMPLEMENTATION
 * ----------------------------- */

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
    Symbol *var_sym = sa_lookup(&cg->sa, node->token->lexeme);
    Symbol *class_sym = var_sym ? NULL : find_enclosing_class(&cg->sa);
    var_sym = !var_sym && class_sym
                  ? sa_lookup_member(class_sym, node->token->lexeme)
                  : var_sym;

    if (var_sym && ((node->ctx == STORE && var_sym->decl_node == node) ||
                    var_sym->base_class)) {
      // DEFINITION: Output "type name" (e.g., "int x")
      sb_appendf(&cg->output, "%s %s", ctype_to_string(cg, node),
                 node->token->lexeme);
    } else {
      // USAGE: Output just the name (e.g., "x")
      sb_appendf(&cg->output, "%s", node->token->lexeme);
    }

    if (cg->is_standalone) {
      sb_appendf(&cg->output, ";\n");
    }
    cg->is_standalone = false;
  } break;
  case LITERAL:
    sb_appendf(&cg->output, "%s", node->token->lexeme);
    if (cg->is_standalone) {
      sb_appendf(&cg->output, ";\n");
    }
    break;
  case BINARY_OPERATION: {
    bool saved = cg->is_standalone;
    cg->is_standalone = false;
    int8_t current_prec = get_infix_precedence(node->token->lexeme);
    int8_t left_prec = get_node_precedence(node->bin_op.left);
    int8_t right_prec = get_node_precedence(node->bin_op.right);

    if (left_prec < current_prec) {
      sb_appendf(&cg->output, "(");
      gen_code(cg, node->bin_op.left);
      sb_appendf(&cg->output, ")");
    } else {
      gen_code(cg, node->bin_op.left);
    }
    sb_appendf(&cg->output, " %s ", node->token->lexeme);

    if (right_prec <= current_prec) {
      sb_appendf(&cg->output, "(");
      gen_code(cg, node->bin_op.right);
      sb_appendf(&cg->output, ")");
    } else {
      gen_code(cg, node->bin_op.right);
    }

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