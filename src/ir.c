#include "ir.h"

static size_t currentRegister = 0;

IRNode_ArrayList ParseIR(Parser *parser) {
    IRNode_ArrayList ir = IRNode_CreateArrayList(parser->ast.size * 2);
    for (Node_Node *curr = parser->ast.head; curr != NULL; curr = curr->next) {
        IRNode *node = ParseIRNode(curr->data);
        if (node != NULL) IRNode_Push(&ir, node);
    }
    return ir;
}

IRNode *ParseIRNode(Node *ast) {
    IRNode *node = AllocateContext(sizeof(IRNode));
    switch (ast->type) {
        case ASSIGNMENT:
            node->assign.value = ParseIRNode(ast->assignStmt->value);
            node->assign.target = ast->assignStmt->target->id;
            node->dataType = node->assign.value->dataType;
            node->reg = currentRegister++;
            node->type = IR_ASSIGN;
            return node;
        case BINARY_OPERATION:
            node->dataType = ast->binOp->type;
            node->binary.left = ParseIRNode(ast->binOp->left);
            node->binary.right = ParseIRNode(ast->binOp->right);
            node->reg = currentRegister++;
            node->type = IR_BINARY_OP;
            if (strcmp(ast->binOp->operator, "+") == 0) {
                node->binary.type = IR_ADD;
            } else if (strcmp(ast->binOp->operator, "-") == 0) {
                node->binary.type = IR_SUB;
            } else if (strcmp(ast->binOp->operator, "*") == 0) {
                node->binary.type = IR_MUL;
            }
            return node;
        case LITERAL:
            node->type = IR_CONST;
            node->dataType = ast->literal->type;
            node->id = ast->literal->value;
            node->reg = currentRegister++;
            return node;
        case VARIABLE:
            node->type = IR_VAR;
            node->dataType = ast->variable->type;
            node->reg = currentRegister++;
            node->id = ast->variable->id;
            return node;
        default:
            return NULL;
    }
}

char *GenerateIRText(IRNode *node) {
    if (node == NULL) {
        return strdup(""); // Return empty string for null nodes
    }

    char buffer[1024]; // Adjust size as needed
    char *left;
    char *right;
    char *value;

    switch (node->type) {
        case IR_BINARY_OP:
            left = GenerateIRText(node->binary.left);
            right = GenerateIRText(node->binary.right);
            const char *op;
            switch (node->binary.type) {
                case IR_ADD:
                    op = "add";
                    break;
                case IR_SUB:
                    op = "sub";
                    break;
                case IR_MUL:
                    op = "mul";
                    break;
                case IR_DIV:
                    op = "div";
                    break;
                default:
                    op = "unknown";
            }
            snprintf(buffer, sizeof(buffer), "%s\n", left);
            snprintf(buffer, sizeof(buffer), "%s\n", right);
            snprintf(buffer, sizeof(buffer), "%%%zu = %s %s %%%zu, %%%zu\n",
                     node->reg, op, PyToIRType(node->dataType),
                     node->binary.left->reg, node->binary.right->reg);
            break;

        case IR_CONST:
            snprintf(buffer, sizeof(buffer), "%%%zu = %s %s\n",
                     node->reg, PyToIRType(node->dataType), node->id);
            break;

        case IR_VAR:
            snprintf(buffer, sizeof(buffer), "%%%zu = load %s, %s* %%%s\n",
                     node->reg, PyToIRType(node->dataType),
                     PyToIRType(node->dataType), node->id);
            break;

        case IR_ASSIGN:
            value = GenerateIRText(node->assign.value);
            snprintf(buffer, sizeof(buffer), "%s\n", value);
            snprintf(buffer, sizeof(buffer), "store %s %%%zu, %s* %%%s\n",
                     PyToIRType(node->dataType), node->assign.value->reg,
                     PyToIRType(node->dataType), node->assign.target);
            break;

        default:
            snprintf(buffer, sizeof(buffer), "; Unknown IR node type\n");
    }

    return strdup(buffer);
}

// Helper function to generate IR text for an entire tree
char *GenerateFullIRText(IRNode *root) {
    if (root == NULL) {
        return strdup("");
    }

    char *current = GenerateIRText(root);
    char *left = (root->type == IR_BINARY_OP) ? GenerateFullIRText(root->binary.left) : strdup("");
    char *right = (root->type == IR_BINARY_OP) ? GenerateFullIRText(root->binary.right) : strdup("");
    char *value = (root->type == IR_ASSIGN) ? GenerateFullIRText(root->assign.value) : strdup("");

    size_t total_len = strlen(current) + strlen(left) + strlen(right) + strlen(value) + 1;
    char *result = AllocateContext(total_len);
    snprintf(result, total_len, "%s%s%s%s", left, right, value, current);
    return result;
}

const char *PyToIRType(DataType type) {
    switch (type) {
        case FLOAT:
            return "f32";
        case INT:
            return "i32";
        case STR:
            return "ptr";
        case COMPLEX:
            return "COMPLEX";
        case LIST:
            return "LIST";
        case BOOL:
            return "bool";
        case VOID:
            return "void";
        default:
            return "UNKNOWN";
    }
}
