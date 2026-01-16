#include "tac.h"

static TACValue gen_expr(Tac *tac, ASTNode *node);

static void gen_stmt(Tac *tac, ASTNode *node);

static void gen_assign(Tac *tac, ASTNode *node);

static void gen_if(Tac *tac, ASTNode *node);

static TACValue gen_binary_op(Tac *tac, ASTNode *node);

static TACValue gen_unary_op(Tac *tac, ASTNode *node);

static void gen_function_def(Tac *tac, ASTNode *node);

TACValue gen_const_value(Tac *tac, ASTNode *node);

size_t tac_add_constant(TACProgram *program, ConstantValue value,
                        DataType type);

ConstantEntry *tac_get_constant(TACProgram *program, size_t id);

static TACValue new_tac_value(size_t id, DataType type) {
  TACValue val;
  val.id = id;
  val.type = type;
  return val;
}

static TACInstruction create_instruction(TACOp op, TACValue lhs, TACValue rhs,
                                         TACValue result, const char *label) {
  TACInstruction instr;
  instr.op = op;
  instr.lhs = lhs;
  instr.rhs = rhs;
  instr.result = result;
  instr.label = label;
  return instr;
}

static void append_instruction(Tac *tac, TACInstruction instr) {
  ASSERT(tac != NULL, "Tac cannot be NULL in append_instruction");

  if (tac->program.count >= tac->program.capacity) {
    size_t new_capacity =
        tac->program.capacity == 0 ? 16 : tac->program.capacity * 2;
    tac->program.instructions =
        allocator_realloc(tac->program.allocator, tac->program.instructions,
                          tac->program.capacity * sizeof(TACInstruction),
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
  case UNKNOWN:
    return "UNKNOWN";
  default:
    return "UNKNOWN";
  }
}

const char *op_to_str(TACOp op) {
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
  case TAC_JMP:
    return "JMP";
  case TAC_CJMP:
    return "CJMP";
  case TAC_JZ:
    return "JZ";
  case TAC_LABEL:
    return "LABEL";
  case TAC_ARG:
    return "ARG";
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
      allocator_alloc(&sa->parser.ast.allocator,
                      sizeof(TACInstruction) * sa->parser.ast.capacity);
  tac.program.count = 0;
  tac.program.capacity = sa->parser.ast.capacity;
  tac.program.allocator = &sa->parser.ast.allocator;
  // Generate TAC for all AST nodes in the program
  ASTNode_LinkedList *program = &sa->parser.ast;
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
  case IF:
    gen_if(tac, node);
    break;
  case FUNCTION_DEF:
    gen_function_def(tac, node);
    break;
  default:
    slog_warn("TAC generation for node type \"%s\" not implemented",
              node_type_to_string(node->type));
    break;
  }
}

static void gen_assign(Tac *tac, ASTNode *node) {
  ASSERT(tac != NULL, "Tac cannot be NULL in gen_assign");
  ASSERT(node != NULL, "ASTNode cannot be NULL in gen_assign");
  ASSERT(node->type == ASSIGNMENT, "Node must be of type ASSIGNMENT");

  ASTNode *value_node = node->assign.value;
  TACValue value = gen_expr(tac, value_node);

  for (size_t current = node->assign.targets.head; current != SIZE_MAX;
       current = node->assign.targets.elements[current].next) {
    ASTNode *target = node->assign.targets.elements[current].data;
    if (target->type == VARIABLE) {
      Symbol *sym = sa_lookup(tac->sa, target->token->lexeme);
      if (sym) {
        TACValue var_addr = new_tac_value(sym->id, sym->dtype);
        TACInstruction instr = create_instruction(
            TAC_STORE, value, new_tac_value(sym->id, NONE), var_addr, NULL);
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
    return gen_const_value(tac, node);
  }
  case VARIABLE: {
    Symbol *sym = sa_lookup(tac->sa, node->token->lexeme);
    if (!sym)
      return new_tac_value(0, UNKNOWN);

    TACValue reg = new_reg(tac, sym->dtype);
    TACValue var_addr = new_tac_value(sym->id, sym->dtype);
    TACInstruction instr = create_instruction(
        TAC_LOAD, var_addr, new_tac_value(0, NONE), reg, NULL);
    append_instruction(tac, instr);
    return reg;
  }

  case BINARY_OPERATION:
    return gen_binary_op(tac, node);

  case COMPARE:
    UNREACHABLE("Binary operations not yet implemented");
    // return gen_compare(tac, sa, node);

  case UNARY_OPERATION:
    return gen_unary_op(tac, node);

  case CALL:
    UNREACHABLE("Binary operations not yet implemented");
    // return gen_call(tac, sa, node);

  default:
    return new_tac_value(0, UNKNOWN);
  }
}

TACValue gen_const_value(Tac *tac, ASTNode *node) {
  ASSERT(tac != NULL, "Tac cannot be NULL in gen_const_value");
  ASSERT(node != NULL, "ASTNode cannot be NULL in gen_const_value");
  ASSERT(node->type == LITERAL, "Node must be of type LITERAL");

  ConstantValue const_val;
  DataType dtype = sa_infer_type(tac->sa, node);

  switch (dtype) {
  case INT:
    const_val.int_val = strtoll(node->token->lexeme, NULL, 10);
    break;
  case FLOAT:
    const_val.float_val = strtod(node->token->lexeme, NULL);
    break;
  case STR:
    const_val.str_val =
        arena_strdup(&tac->sa->parser.ast.allocator.base, node->token->lexeme);
    break;
  default:
    UNREACHABLE("Unsupported literal type in gen_const_value");
  }

  size_t const_id = tac_add_constant(&tac->program, const_val, dtype);
  TACValue result = new_reg(tac, dtype);
  TACValue const_value = new_tac_value(const_id, dtype);

  TACInstruction instr = create_instruction(
      TAC_CONST, const_value, new_tac_value(0, NONE), result, NULL);
  append_instruction(tac, instr);
  return result;
}

static TACValue gen_binary_op(Tac *tac, ASTNode *node) {
  ASSERT(tac != NULL, "Tac cannot be NULL in gen_binary_op");
  ASSERT(node != NULL, "ASTNode cannot be NULL in gen_binary_op");
  ASSERT(node->type == BINARY_OPERATION, "Node must be BINARY_OPERATION");
  ASSERT(node->token != NULL, "Binary operation node must have a token");

  // Generate operands
  TACValue lhs = gen_expr(tac, node->bin_op.left);
  TACValue rhs = gen_expr(tac, node->bin_op.right);

  // Infer result type from semantic analysis
  DataType result_type = sa_infer_type(tac->sa, node);
  TACValue result = new_reg(tac, result_type);

  const char *op_lexeme = node->token->lexeme;
  TACOp op;

  if (strcmp(op_lexeme, "+") == 0) {
    op = TAC_ADD;
  } else if (strcmp(op_lexeme, "-") == 0) {
    op = TAC_SUB;
  } else if (strcmp(op_lexeme, "*") == 0) {
    op = TAC_MUL;
  } else if (strcmp(op_lexeme, "/") == 0) {
    op = TAC_DIV;
  } else {
    UNREACHABLE("Invalid binary operator in gen_binary_op");
  }

  TACInstruction instr = create_instruction(op, lhs, rhs, result, NULL);
  append_instruction(tac, instr);
  return result;
}

static TACValue gen_unary_op(Tac *tac, ASTNode *node) {
  ASSERT(tac != NULL, "Tac cannot be NULL in gen_unary_op");
  ASSERT(node != NULL, "ASTNode cannot be NULL in gen_unary_op");
  ASSERT(node->type == UNARY_OPERATION, "Node must be UNARY_OPERATION");
  ASSERT(node->token != NULL, "Unary operation node must have a token");

  // Generate operand
  TACValue operand = gen_expr(tac, node->bin_op.right);

  // Infer result type
  DataType result_type = sa_infer_type(tac->sa, node);

  const char *op = node->token->lexeme;

  /* Unary minus: -x ==> 0 - x */
  if (strcmp(op, "-") == 0) {
    Token zero_token = {
        .type = NUMBER,
        .lexeme = "0",
        .line = 0,
        .col = 0,
        .ident = 0,
    };
    ASTNode *zero_node = node_new(&tac->sa->parser, &zero_token, LITERAL);
    TACValue zero_val = gen_const_value(tac, zero_node);
    TACValue result = new_reg(tac, result_type);
    TACInstruction instr =
        create_instruction(TAC_SUB, zero_val, operand, result, NULL);
    append_instruction(tac, instr);
    return result;
  }

  /* Unary plus: +x ==> x (no-op) */
  if (strcmp(op, "+") == 0) {
    return operand;
  }

  UNREACHABLE("Unknown unary operator in gen_unary_op");
}

// |-----------------|
// | Printing region |
// |-----------------|

// Format a TACValue as a string
// TODO: Make program the first argument
static void format_value(StringBuilder *sb, TACValue val, const char *prefix) {
  if (val.type == NONE) {
    sb_appendf(sb, "_");
    return;
  }

  sb_appendf(sb, "%s%zu:%s", prefix, val.id, type_to_str(val.type));
}

static void format_value_ref(StringBuilder *sb, TACValue val,
                             const char *prefix) {
  if (val.type == NONE) {
    sb_appendf(sb, "_");
    return;
  }

  sb_appendf(sb, "%s%zu", prefix, val.id);
}

// Generate TAC code from a TACProgram and return it as a StringBuilder
StringBuilder tac_generate_code(TACProgram *program) {
  StringBuilder sb = {0};
  sb.allocator = program->allocator;
  sb.items =
      allocator_alloc(program->allocator, ARENA_DA_INIT_CAP * sizeof(char));
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
      StringBuilder res_sb = {.allocator = program->allocator};
      format_value(&res_sb, instr->result, "t");

      ConstantEntry *c = tac_get_constant(program, instr->lhs.id);
      ASSERT(c, "CONST instruction missing constant");

      switch (c->type) {
      case INT:
        sb_appendf(&sb, "    %.*s = CONST %ld\n", (int)res_sb.count,
                   res_sb.items, c->value.int_val);
        break;
      case FLOAT:
        sb_appendf(&sb, "    %.*s = CONST %f\n", (int)res_sb.count,
                   res_sb.items, c->value.float_val);
        break;
      case STR:
        sb_appendf(&sb, "    %.*s = CONST \"%s\"\n", (int)res_sb.count,
                   res_sb.items, c->value.str_val);
        break;
      case BOOL:
        sb_appendf(&sb, "    %.*s = CONST %zu\n", (int)res_sb.count,
                   res_sb.items, c->value.int_val);
        break;
      default:
        sb_appendf(&sb, "    %.*s = CONST <unknown>\n", (int)res_sb.count,
                   res_sb.items);
      }
      break;
    }

    case TAC_LOAD: {
      StringBuilder lhs_sb = {.allocator = program->allocator},
                    res_sb = {.allocator = program->allocator};
      format_value_ref(&lhs_sb, instr->lhs, "v");
      format_value(&res_sb, instr->result, "t");
      sb_appendf(&sb, "    %.*s = %.*s\n", (int)res_sb.count, res_sb.items,
                 (int)lhs_sb.count, lhs_sb.items);
      break;
    }

    case TAC_STORE: {
      StringBuilder src_sb = {.allocator = program->allocator};
      StringBuilder dst_sb = {.allocator = program->allocator};

      format_value_ref(&src_sb, instr->lhs, "t");
      format_value(&dst_sb, instr->result, "v");

      sb_appendf(&sb, "    %.*s = %.*s\n", (int)dst_sb.count, dst_sb.items,
                 (int)src_sb.count, src_sb.items);
      break;
    }

    case TAC_ADD:
    case TAC_SUB:
    case TAC_MUL:
    case TAC_DIV:
    case TAC_CMP: {
      StringBuilder lhs_sb = {.allocator = program->allocator},
                    rhs_sb = {.allocator = program->allocator},
                    res_sb = {.allocator = program->allocator};
      format_value_ref(&lhs_sb, instr->lhs, "t");
      format_value_ref(&rhs_sb, instr->rhs, "t");
      format_value(&res_sb, instr->result, "t");
      sb_appendf(&sb, "    %.*s = %s %.*s, %.*s\n", (int)res_sb.count,
                 res_sb.items, op_to_str(instr->op), (int)lhs_sb.count,
                 lhs_sb.items, (int)rhs_sb.count, rhs_sb.items);
      break;
    }

    case TAC_CALL: {
      StringBuilder res_sb = {0};
      format_value(&res_sb, instr->result, "t");
      sb_appendf(&sb, "    %.*s = CALL %s(%lu args)\n", (int)res_sb.count,
                 res_sb.items, instr->label ? instr->label : "unknown",
                 instr->lhs.id);
      break;
    }

    case TAC_RETURN:
      if (instr->lhs.type != NONE) {
        StringBuilder lhs_sb = {0};
        format_value(&lhs_sb, instr->lhs, "t");
        sb_appendf(&sb, "    RETURN %.*s\n", (int)lhs_sb.count, lhs_sb.items);
        free(lhs_sb.items);
      } else {
        sb_appendf(&sb, "    RETURN\n");
      }
      break;

    case TAC_JMP:
      sb_appendf(&sb, "    JUMP %s\n", instr->label ? instr->label : "unknown");
      break;

    case TAC_JZ: {
      StringBuilder lhs_sb = {.allocator = program->allocator};
      format_value_ref(&lhs_sb, instr->lhs, "t");

      sb_appendf(&sb, "    JZ %.*s -> %s\n", (int)lhs_sb.count, lhs_sb.items,
                 instr->label ? instr->label : "unknown");
      break;
    }

    case TAC_CJMP: {
      StringBuilder lhs_sb = {0};
      format_value(&lhs_sb, instr->lhs, "t");
      sb_appendf(&sb, "    CJUMP %.*s -> %s\n", (int)lhs_sb.count, lhs_sb.items,
                 instr->label ? instr->label : "unknown");
      break;
    }

    case TAC_ARG: {
      StringBuilder res_sb = {.allocator = program->allocator};
      format_value(&res_sb, instr->result, "v");
      sb_appendf(&sb, "    %.*s = ARG %zu\n", (int)res_sb.count, res_sb.items,
                 instr->lhs.id);
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
    if (instr->lhs.type != NONE) {
      sb_appendf(sb, "    RETURN t%zu\n", instr->lhs.id);
    } else {
      sb_appendf(sb, "    RETURN\n");
    }
    break;

  case TAC_JMP:
    sb_appendf(sb, "    JUMP %s\n", instr->label);
    break;

  case TAC_CJMP:
    sb_appendf(sb, "    CJUMP t%zu -> %s\n", instr->lhs.id, instr->label);
    break;

  default:
    sb_appendf(sb, "    %s\n", op_to_str(instr->op));
    break;
  }
}

size_t tac_add_constant(TACProgram *program, ConstantValue value,
                        DataType type) {
  if (!program)
    return 0;

  // Check if constant already exists
  for (size_t i = 0; i < program->constants.count; i++) {
    ConstantEntry *entry = &program->constants.entries[i];
    if (entry->type == type) {
      switch (type) {
      case INT:
        if (entry->value.int_val == value.int_val)
          return entry->id;
        break;
      case FLOAT:
        if (entry->value.float_val == value.float_val)
          return entry->id;
        break;
      case STR:
        if (strcmp(entry->value.str_val, value.str_val) == 0)
          return entry->id;
        break;
      case BOOL:
        if (entry->value.int_val == value.int_val)
          return entry->id;
        break;
      default:
        break;
      }
    }
  }

  // Grow constant table if needed
  if (program->constants.count >= program->constants.capacity) {
    size_t new_capacity =
        program->constants.capacity == 0 ? 16 : program->constants.capacity * 2;
    ConstantEntry *new_entries = allocator_realloc(
        program->allocator, program->constants.entries,
        program->constants.count, new_capacity * sizeof(ConstantEntry));
    if (!new_entries)
      return 0;

    program->constants.entries = new_entries;
    program->constants.capacity = new_capacity;
  }

  // Add new constant
  ConstantEntry *entry = &program->constants.entries[program->constants.count];
  entry->id = program->constants.next_id++;
  entry->type = type;

  // Copy value (handle strings specially)
  if (type == STR) {
    entry->value.str_val = value.str_val;
  } else {
    entry->value = value;
  }

  program->constants.count++;
  return entry->id;
}

ConstantEntry *tac_get_constant(TACProgram *program, size_t id) {
  if (!program)
    return NULL;

  for (size_t i = 0; i < program->constants.count; i++) {
    if (program->constants.entries[i].id == id) {
      return &program->constants.entries[i];
    }
  }
  return NULL;
}

static const char *new_label(Tac *tac) {
  char buf[32];
  snprintf(buf, sizeof(buf), "L%zu", tac->label_counter++);
  return arena_strdup(&tac->sa->parser.ast.allocator.base, buf);
}

static void gen_if(Tac *tac, ASTNode *node) {
  ASSERT(node->type == IF, "Expected IF");
  TACValue cond = gen_expr(tac, node->ctrl_stmt.test);
  bool has_else = node->ctrl_stmt.orelse.head != SIZE_MAX;
  const char *else_label = has_else ? new_label(tac) : NULL;
  const char *end_label = new_label(tac);

  /* if not cond -> else or end */
  append_instruction(tac,
                     create_instruction(TAC_JZ, cond, new_tac_value(0, NONE),
                                        new_tac_value(0, NONE),
                                        has_else ? else_label : end_label));

  /* then-body */
  for (size_t cur = node->ctrl_stmt.body.head; cur != SIZE_MAX;
       cur = node->ctrl_stmt.body.elements[cur].next) {
    gen_stmt(tac, node->ctrl_stmt.body.elements[cur].data);
  }

  /* jump over else */
  if (has_else) {
    append_instruction(tac,
                       create_instruction(TAC_JMP, new_tac_value(0, NONE),
                                          new_tac_value(0, NONE),
                                          new_tac_value(0, NONE), end_label));
    append_instruction(tac,
                       create_instruction(TAC_LABEL, new_tac_value(0, NONE),
                                          new_tac_value(0, NONE),
                                          new_tac_value(0, NONE), else_label));

    for (size_t cur = node->ctrl_stmt.orelse.head; cur != SIZE_MAX;
         cur = node->ctrl_stmt.orelse.elements[cur].next) {
      gen_stmt(tac, node->ctrl_stmt.orelse.elements[cur].data);
    }
  }

  /* end label */
  append_instruction(tac,
                     create_instruction(TAC_LABEL, new_tac_value(0, NONE),
                                        new_tac_value(0, NONE),
                                        new_tac_value(0, NONE), end_label));
}

// TODO: Add name mangling
static void gen_function_def(Tac *tac, ASTNode *node) {
  ASSERT(tac != NULL, "Tac cannot be NULL");
  ASSERT(node != NULL, "ASTNode cannot be NULL");
  ASSERT(node->type == FUNCTION_DEF, "Expected FUNCTION_DEF node");

  // 1. Function Label
  // We use the function name as the label so CALL instructions can find it.
  const char *func_name = node->def.name->token->lexeme;
  append_instruction(tac,
                     create_instruction(TAC_LABEL, new_tac_value(0, NONE),
                                        new_tac_value(0, NONE),
                                        new_tac_value(0, NONE), func_name));

  // 2. Handle Parameters
  // This maps the calling convention's arguments into the function's local
  // virtual registers.
  for (size_t cur = node->def.params.head; cur != SIZE_MAX;
       cur = node->def.params.elements[cur].next) {
    ASTNode *param_node = node->def.params.elements[cur].data;
    Symbol *sym = sa_lookup(tac->sa, param_node->token->lexeme);
    size_t arg_index = 0;

    if (sym) {
      TACValue local_var = new_tac_value(sym->id, sym->dtype);
      TACValue idx = new_tac_value(arg_index++, NONE);
      append_instruction(
          tac, create_instruction(TAC_ARG,
                                  idx, // Which argument index is this?
                                  new_tac_value(0, NONE), // Unused
                                  local_var, // Where to store it locally
                                  NULL));
    }
  }

  // 3. Generate Body
  for (size_t cur = node->def.body.head; cur != SIZE_MAX;
       cur = node->def.body.elements[cur].next) {
    gen_stmt(tac, node->def.body.elements[cur].data);
  }

  // 4. Implicit Return
  append_instruction(tac, create_instruction(TAC_RETURN, new_tac_value(0, NONE),
                                             new_tac_value(0, NONE),
                                             new_tac_value(0, NONE), NULL));
}

static cJSON *serialize_tac_value(TACValue val) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "id", (double)val.id);
    cJSON_AddStringToObject(root, "type", type_to_str(val.type));
    return root;
}

static cJSON *serialize_constant(ConstantEntry *entry) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "id", (double)entry->id);
    cJSON_AddStringToObject(root, "type", type_to_str(entry->type));

    switch (entry->type) {
        case INT:   cJSON_AddNumberToObject(root, "value", (double)entry->value.int_val); break;
        case FLOAT: cJSON_AddNumberToObject(root, "value", entry->value.float_val); break;
        case STR:   cJSON_AddStringToObject(root, "value", entry->value.str_val); break;
        case BOOL:  cJSON_AddBoolToObject(root, "value", entry->value.int_val != 0); break;
        default:    cJSON_AddNullToObject(root, "value"); break;
    }
    return root;
}

cJSON *serialize_tac_program(TACProgram *program) {
    if (!program) return NULL;

    cJSON *root = cJSON_CreateObject();

    // 1. Serialize Constants Table
    cJSON *constants = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "constants", constants);
    for (size_t i = 0; i < program->constants.count; i++) {
        cJSON_AddItemToArray(constants, serialize_constant(&program->constants.entries[i]));
    }

    // 2. Serialize Instructions
    cJSON *instructions = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "instructions", instructions);
    for (size_t i = 0; i < program->count; i++) {
        TACInstruction *instr = &program->instructions[i];
        cJSON *j_instr = cJSON_CreateObject();
        
        cJSON_AddNumberToObject(j_instr, "index", (double)i);
        cJSON_AddStringToObject(j_instr, "op", op_to_str(instr->op));
        
        // Add operands
        cJSON_AddItemToObject(j_instr, "lhs", serialize_tac_value(instr->lhs));
        cJSON_AddItemToObject(j_instr, "rhs", serialize_tac_value(instr->rhs));
        cJSON_AddItemToObject(j_instr, "result", serialize_tac_value(instr->result));
        
        // Add label if it exists
        if (instr->label) {
            cJSON_AddStringToObject(j_instr, "label", instr->label);
        }

        cJSON_AddItemToArray(instructions, j_instr);
    }

    return root;
}

char *tac_dump_program(TACProgram *program) {
  cJSON *json = serialize_tac_program(program);
  char *dump = cJSON_Print(json);
  cJSON_Delete(json);
  return dump;
}