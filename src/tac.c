#include "tac.h"

static TACValue gen_expr(Tac *tac, ASTNode *node);

static void gen_stmt(Tac *tac, ASTNode *node);

static void gen_assign(Tac *tac, ASTNode *node);

static TACValue new_tac_value(size_t id, DataType type) {
  TACValue val;
  val.id = id;
  val.type = type;
  return val;
}

static TACInstruction create_instruction(TACOp op, TACValue lhs, TACValue rhs,
                                         TACValue result, const char *label,
                                         const char *label_false) {
  TACInstruction instr;
  instr.op = op;
  instr.lhs = lhs;
  instr.rhs = rhs;
  instr.result = result;
  instr.label = label;
  instr.label_false = label_false;
  return instr;
}

static void append_instruction(Tac *tac, TACInstruction instr) {
  ASSERT(tac != NULL, "Tac cannot be NULL in append_instruction");

  if (tac->program.count >= tac->program.capacity) {
    size_t new_capacity =
        tac->program.capacity == 0 ? 16 : tac->program.capacity * 2;
    allocator_realloc(&tac->sa->parser->ast.allocator,
                      tac->program.instructions, tac->sa->parser->ast.size,
                      new_capacity * sizeof(TACInstruction));
    tac->program.capacity = new_capacity;
  }

  tac->program.instructions[tac->program.count++] = instr;
}

static const char *type_to_str(DataType type) {
  switch (type) {
  case INT:
    return "INT";
  case FLOAT:
    return "FLOAT";
  case STR:
    return "STR";
  case BOOL:
    return "BOOL";
  case LIST:
    return "LIST";
  case OBJECT:
    return "OBJECT";
  case NONE:
    return "NONE";
  case VOID:
    return "VOID";
  case UNKNOWN:
    return "UNKNOWN";
  default:
    return "UNKNOWN";
  }
}

static const char *op_to_str(TACOp op) {
  switch (op) {
  case TAC_CONST:
    return "CONST";
  case TAC_LOAD:
    return "LOAD";
  case TAC_STORE:
    return "STORE";
  case TAC_ADD:
    return "ADD";
  case TAC_SUB:
    return "SUB";
  case TAC_MUL:
    return "MUL";
  case TAC_DIV:
    return "DIV";
  case TAC_CMP:
    return "CMP";
  case TAC_CALL:
    return "CALL";
  case TAC_RETURN:
    return "RETURN";
  case TAC_JUMP:
    return "JUMP";
  case TAC_CJUMP:
    return "CJUMP";
  case TAC_LABEL:
    return "LABEL";
  default:
    return "UNKNOWN";
  }
}

static TACValue new_reg(Tac *tac, DataType type) {
  TACValue reg;
  reg.id = tac->reg_counter++;
  reg.type = type;
  return reg;
}

TACProgram tac_generate(SemanticAnalyzer *sa) {
  ASSERT(sa != NULL, "SemanticAnalyzer cannot be NULL");
  // Initialize TAC generator state
  Tac tac;
  tac.sa = sa;
  tac.reg_counter = 0;
  tac.program.instructions =
      allocator_alloc(&sa->parser->ast.allocator,
                      sizeof(TACInstruction) * sa->parser->ast.capacity);
  tac.program.count = 0;
  tac.program.capacity = sa->parser->ast.capacity;
  // Generate TAC for all AST nodes in the program
  ASTNode_LinkedList *program = &sa->parser->ast;
  for (size_t current = program->head; current != SIZE_MAX;
       current = program->elements[current].next) {
    ASTNode *node = program->elements[current].data;
    gen_stmt(&tac, node);
  }

  return tac.program;
}

static void gen_stmt(Tac *tac, ASTNode *node) {
  if (!node) {
    slog_warn("TAC generation received NULL AST node");
    return;
  }

  switch (node->type) {
  case ASSIGNMENT:
    gen_assign(tac, node);
    break;
  default:
    slog_warn("TAC generation for node type %d not implemented", node->type);
    break;
  }
}

static void gen_assign(Tac *tac, ASTNode *node) {
  if (node->type != ASSIGNMENT) {
    slog_warn("gen_assign called with non-assignment node");
    return;
  }

  ASTNode *value_node = node->assign.value;
  TACValue value = gen_expr(tac, value_node);

  for (size_t current = node->assign.targets.head; current != SIZE_MAX;
       current = node->assign.targets.elements[current].next) {
    ASTNode *target = node->assign.targets.elements[current].data;
    if (target->type == VARIABLE) {
      Symbol *sym = sa_lookup(tac->sa, target->token->lexeme);
      if (sym) {
        TACValue var_addr = new_tac_value(0, sym->dtype);
        TACInstruction instr = create_instruction(
            TAC_STORE, value, new_tac_value(0, VOID), var_addr, NULL, NULL);
        append_instruction(tac, instr);
      }
    }
  }
}

static TACValue gen_expr(Tac *tac, ASTNode *node) {
  if (!node)
    return new_tac_value(0, UNKNOWN);

  switch (node->type) {
  case LITERAL: {
    TACValue reg = new_reg(tac, sa_infer_type(tac->sa, node));
    TACValue const_val = new_tac_value(0, reg.type);

    // For simplicity, we store the constant value in lhs.id
    // In a real implementation, you'd have a proper constant table
    TACInstruction instr = create_instruction(
        TAC_CONST, const_val, new_tac_value(0, VOID), reg, NULL, NULL);
    // Store the constant value (simplified)
    instr.lhs.id = (size_t)node->token->lexeme; // Just a placeholder

    append_instruction(tac, instr);
    return reg;
  }

  case VARIABLE: {
    Symbol *sym = sa_lookup(tac->sa, node->token->lexeme);
    if (!sym)
      return new_tac_value(0, UNKNOWN);

    TACValue reg = new_reg(tac, sym->dtype);
    TACValue var_addr = new_tac_value(0, sym->dtype);
    // In a real implementation, var_addr.id would hold the variable's
    // address/ID

    TACInstruction instr = create_instruction(
        TAC_LOAD, var_addr, new_tac_value(0, VOID), reg, NULL, NULL);
    append_instruction(tac, instr);
    return reg;
  }

  case BINARY_OPERATION:
    UNREACHABLE("Binary operations not yet implemented");
    // return gen_binary_op(tac, sa, node);

  case COMPARE:
    UNREACHABLE("Binary operations not yet implemented");
    // return gen_compare(tac, sa, node);

  case UNARY_OPERATION:
    UNREACHABLE("Binary operations not yet implemented");
    // return gen_unary_op(tac, sa, node);

  case CALL:
    UNREACHABLE("Binary operations not yet implemented");
    // return gen_call(tac, sa, node);

  default:
    return new_tac_value(0, UNKNOWN);
  }
}

// |-----------------|
// | Printing region |
// |-----------------|

// Format a TACValue as a string
static void format_value(StringBuilder *sb, TACValue val, const char *prefix) {
  if (val.type == VOID) {
    sb_appendf(sb, "_");
  } else {
    sb_appendf(sb, "%s%zu:%s", prefix, val.id, type_to_str(val.type));
  }
}

// Generate TAC code from a TACProgram and return it as a StringBuilder
StringBuilder tac_generate_code(TACProgram *program, Allocator *allocator) {
  StringBuilder sb = {0};
  sb.allocator = allocator;
  sb.items = allocator_alloc(allocator, ARENA_DA_INIT_CAP * sizeof(char));
    sb.count = 0;
  sb.capacity = ARENA_DA_INIT_CAP;
  if (!program) {
    return sb;
  }

  // Append header
  sb_appendf(&sb, "=== TAC Program ===\n");
  sb_appendf(&sb, "Instructions: %zu\n\n", program->count);

  // Process each instruction
  for (size_t i = 0; i < program->count; i++) {
    TACInstruction *instr = &program->instructions[i];

    // Append instruction number
    sb_appendf(&sb, "%04zu: ", i);

    // Format based on operation type
    switch (instr->op) {
    case TAC_LABEL:
      sb_appendf(&sb, "%s:\n", instr->label ? instr->label : "unnamed");
      break;

    case TAC_CONST: {
      StringBuilder result_sb = {.allocator = allocator};
      format_value(&result_sb, instr->result, "t");
      sb_appendf(&sb, "    %.*s = CONST %lu\n", (int)result_sb.count,
                 result_sb.items, instr->lhs.id);
      break;
    }

    case TAC_LOAD: {
      StringBuilder lhs_sb = {.allocator = allocator}, res_sb = {.allocator = allocator};
      format_value(&lhs_sb, instr->lhs, "v");
      format_value(&res_sb, instr->result, "t");
      sb_appendf(&sb, "    %.*s = LOAD %.*s\n", (int)res_sb.count, res_sb.items,
                 (int)lhs_sb.count, lhs_sb.items);
      break;
    }

    case TAC_STORE: {
      StringBuilder lhs_sb = {.allocator = allocator}, res_sb = {.allocator = allocator};
      format_value(&lhs_sb, instr->lhs, "t");
      format_value(&res_sb, instr->result, "v");
      sb_appendf(&sb, "    %.*s = STORE %.*s\n", (int)res_sb.count,
                 res_sb.items, (int)lhs_sb.count, lhs_sb.items);
      break;
    }

    case TAC_ADD:
    case TAC_SUB:
    case TAC_MUL:
    case TAC_DIV:
    case TAC_CMP: {
      StringBuilder lhs_sb = {0}, rhs_sb = {0}, res_sb = {0};
      format_value(&lhs_sb, instr->lhs, "t");
      format_value(&rhs_sb, instr->rhs, "t");
      format_value(&res_sb, instr->result, "t");
      sb_appendf(&sb, "    %.*s = %s %.*s, %.*s\n", (int)res_sb.count,
                 res_sb.items, op_to_str(instr->op), (int)lhs_sb.count,
                 lhs_sb.items, (int)rhs_sb.count, rhs_sb.items);
      free(lhs_sb.items);
      free(rhs_sb.items);
      free(res_sb.items);
      break;
    }

    case TAC_CALL: {
      StringBuilder res_sb = {0};
      format_value(&res_sb, instr->result, "t");
      sb_appendf(&sb, "    %.*s = CALL %s(%lu args)\n", (int)res_sb.count,
                 res_sb.items, instr->label ? instr->label : "unknown",
                 instr->lhs.id);
      free(res_sb.items);
      break;
    }

    case TAC_RETURN:
      if (instr->lhs.type != VOID) {
        StringBuilder lhs_sb = {0};
        format_value(&lhs_sb, instr->lhs, "t");
        sb_appendf(&sb, "    RETURN %.*s\n", (int)lhs_sb.count, lhs_sb.items);
        free(lhs_sb.items);
      } else {
        sb_appendf(&sb, "    RETURN\n");
      }
      break;

    case TAC_JUMP:
      sb_appendf(&sb, "    JUMP %s\n", instr->label ? instr->label : "unknown");
      break;

    case TAC_CJUMP: {
      StringBuilder lhs_sb = {0};
      format_value(&lhs_sb, instr->lhs, "t");
      sb_appendf(&sb, "    CJUMP %.*s -> %s\n", (int)lhs_sb.count, lhs_sb.items,
                 instr->label ? instr->label : "unknown");
      free(lhs_sb.items);
      break;
    }

    default:
      sb_appendf(&sb, "    UNKNOWN\n");
      break;
    }
  }

  sb_appendf(&sb, "\n=== End TAC Program ===\n");
  return sb;
}

// Alternative version that returns pretty formatted code with comments
StringBuilder tac_generate_pretty_code(TACProgram *program) {
  StringBuilder sb = {0};

  if (!program) {
    return sb;
  }

  // Append header with comments
  sb_appendf(&sb, "; === Three-Address Code ===\n");
  sb_appendf(&sb, "; Generated from source program\n");
  sb_appendf(&sb, "; Total instructions: %zu\n\n", program->count);

  // Process each instruction
  for (size_t i = 0; i < program->count; i++) {
    TACInstruction *instr = &program->instructions[i];

    // Handle labels specially
    if (instr->op == TAC_LABEL) {
      sb_appendf(&sb, "\n%s:\n", instr->label ? instr->label : "unnamed");
      continue;
    }

    // Indent regular instructions
    sb_appendf(&sb, "    ");

    // Format based on operation type
    switch (instr->op) {
    case TAC_CONST: {
      StringBuilder res_sb = {0};
      format_value(&res_sb, instr->result, "t");
      sb_appendf(&sb, "%.*s = CONST %lu", (int)res_sb.count, res_sb.items,
                 instr->lhs.id);
      free(res_sb.items);
      break;
    }

    case TAC_LOAD: {
      StringBuilder lhs_sb = {0}, res_sb = {0};
      format_value(&lhs_sb, instr->lhs, "v");
      format_value(&res_sb, instr->result, "t");
      sb_appendf(&sb, "%.*s = LOAD %.*s", (int)res_sb.count, res_sb.items,
                 (int)lhs_sb.count, lhs_sb.items);
      free(lhs_sb.items);
      free(res_sb.items);
      break;
    }

    case TAC_STORE: {
      StringBuilder lhs_sb = {0}, res_sb = {0};
      format_value(&lhs_sb, instr->lhs, "t");
      format_value(&res_sb, instr->result, "v");
      sb_appendf(&sb, "%.*s = STORE %.*s", (int)res_sb.count, res_sb.items,
                 (int)lhs_sb.count, lhs_sb.items);
      free(lhs_sb.items);
      free(res_sb.items);
      break;
    }

    case TAC_ADD:
    case TAC_SUB:
    case TAC_MUL:
    case TAC_DIV:
    case TAC_CMP: {
      StringBuilder lhs_sb = {0}, rhs_sb = {0}, res_sb = {0};
      format_value(&lhs_sb, instr->lhs, "t");
      format_value(&rhs_sb, instr->rhs, "t");
      format_value(&res_sb, instr->result, "t");
      sb_appendf(&sb, "%.*s = %s %.*s, %.*s", (int)res_sb.count, res_sb.items,
                 op_to_str(instr->op), (int)lhs_sb.count, lhs_sb.items,
                 (int)rhs_sb.count, rhs_sb.items);
      free(lhs_sb.items);
      free(rhs_sb.items);
      free(res_sb.items);
      break;
    }

    case TAC_CALL: {
      StringBuilder res_sb = {0};
      format_value(&res_sb, instr->result, "t");
      sb_appendf(&sb, "%.*s = CALL %s(%lu)", (int)res_sb.count, res_sb.items,
                 instr->label ? instr->label : "unknown", instr->lhs.id);
      free(res_sb.items);
      break;
    }

    case TAC_RETURN:
      if (instr->lhs.type != VOID) {
        StringBuilder lhs_sb = {0};
        format_value(&lhs_sb, instr->lhs, "t");
        sb_appendf(&sb, "RETURN %.*s", (int)lhs_sb.count, lhs_sb.items);
        free(lhs_sb.items);
      } else {
        sb_appendf(&sb, "RETURN");
      }
      break;

    case TAC_JUMP:
      sb_appendf(&sb, "JUMP %s", instr->label ? instr->label : "unknown");
      break;

    case TAC_CJUMP: {
      StringBuilder lhs_sb = {0};
      format_value(&lhs_sb, instr->lhs, "t");
      sb_appendf(&sb, "CJUMP %.*s -> %s", (int)lhs_sb.count, lhs_sb.items,
                 instr->label ? instr->label : "unknown");
      free(lhs_sb.items);
      break;
    }

    default:
      sb_appendf(&sb, "UNKNOWN");
      break;
    }

    // Add instruction index as comment
    sb_appendf(&sb, "  ; #%zu\n", i);
  }

  sb_appendf(&sb, "\n; === End of TAC ===\n");
  return sb;
}

// Helper function to append TAC instruction to StringBuilder
void tac_append_instruction(StringBuilder *sb, TACInstruction *instr,
                            size_t index) {
  if (!sb || !instr)
    return;

  // Basic formatting with index
  sb_appendf(sb, "%04zu: ", index);

  // Simplified formatting for debug output
  switch (instr->op) {
  case TAC_LABEL:
    sb_appendf(sb, "%s:\n", instr->label ? instr->label : "L");
    break;

  case TAC_CONST:
    sb_appendf(sb, "    t%zu = %lu\n", instr->result.id, instr->lhs.id);
    break;

  case TAC_ADD:
  case TAC_SUB:
  case TAC_MUL:
  case TAC_DIV:
    sb_appendf(sb, "    t%zu = t%zu %s t%zu\n", instr->result.id, instr->lhs.id,
               op_to_str(instr->op), instr->rhs.id);
    break;

  case TAC_CALL:
    sb_appendf(sb, "    t%zu = %s()\n", instr->result.id, instr->label);
    break;

  case TAC_RETURN:
    if (instr->lhs.type != VOID) {
      sb_appendf(sb, "    RETURN t%zu\n", instr->lhs.id);
    } else {
      sb_appendf(sb, "    RETURN\n");
    }
    break;

  case TAC_JUMP:
    sb_appendf(sb, "    JUMP %s\n", instr->label);
    break;

  case TAC_CJUMP:
    sb_appendf(sb, "    CJUMP t%zu -> %s\n", instr->lhs.id, instr->label);
    break;

  default:
    sb_appendf(sb, "    %s\n", op_to_str(instr->op));
    break;
  }
}