#include "codegen.h"

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
  case FUNCTION_DEF: {
    // Function signature
    sb_appendf(
        &cg->output, "%s %s(",
        datatype_to_string(node->def.returns
                               ? sa_infer_type(&cg->sa, node->def.returns)
                               : NONE),
        node->def.name->token->lexeme);

    if (node->def.params.size == 0) {
      sb_appendf(&cg->output, "void");
    }

    // Parameters
    for (size_t cur = node->def.params.head; cur != SIZE_MAX;
         cur = node->def.params.elements[cur].next) {
      ASTNode *param = node->def.params.elements[cur].data;
      DataType param_type = sa_infer_type(&cg->sa, param);
      sb_appendf(&cg->output, "%s %s", datatype_to_string(param_type),
                 param->token->lexeme);
      if (cur != node->def.params.tail) {
        sb_appendf(&cg->output, ", ");
      }
    }
    sb_appendf(&cg->output, ") {\n");
    // Function body
    for (size_t cur = node->def.body.head; cur != SIZE_MAX;
         cur = node->def.body.elements[cur].next) {
      ASTNode *body_node = node->def.body.elements[cur].data;
      cg->is_standalone = true;
      gen_code(cg, body_node);
    }

    sb_appendf(&cg->output, "}\n");
  } break;
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
  case VARIABLE:
  case LITERAL:
    sb_appendf(&cg->output, "%s", node->token->lexeme);
    break;
  case BINARY_OPERATION: {
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
  } break;
  default:
    slog_fatal("Code generation for \"%s\" type is not implemented yet", node_type_to_string(node->type));
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
  ASTNode_LinkedList main =
      ASTNode_new_with_allocator(&cg->sa.parser.ast.allocator, 4);

  for (size_t current = program->head; current != SIZE_MAX;
       current = program->elements[current].next) {
    ASTNode *node = program->elements[current].data;
    if (node->type == FUNCTION_DEF) {
      cg->is_standalone = true;
      if (!gen_code(cg, node))
        return false;
      continue;
    }

    ASTNode_add_first(&main, node);
  }

  if (main.size == 0)
    return true; // No standalone statements to generate

  sb_appendf(&cg->output, "\nint main(void) {\n");

  for (size_t current = main.head; current != SIZE_MAX;
       current = main.elements[current].next) {
    ASTNode *node = main.elements[current].data;
    cg->is_standalone = true;
    sb_appendf(&cg->output, "    ");

    if (!gen_code(cg, node))
      return false;
  }

  sb_appendf(&cg->output, "    return 0;\n}\n");
  return true;
}

bool codegen_has_error(Codegen *cg) { return cg->last_error.type != CG_OK; }

CodegenError codegen_get_error(Codegen *cg) { return cg->last_error; }