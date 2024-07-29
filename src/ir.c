#include "ir.h"

static size_t currentRegister = 0;

IRNode_ArrayList ParseIR(Parser *parser) {
    IRNode_ArrayList ir = IRNode_CreateArrayList(parser->ast.size * 2);
    for (Node_Node *curr = parser->ast.head; curr != NULL; curr = curr->next) {
        IRNode *node = ParseIRNode(curr->data);
        if (node != NULL) IRNode_Push(&ir, node);
    }
}

IRNode *ParseIRNode(Node *ast) {
    IRNode *node = AllocateContext(sizeof(IRNode));
    switch (ast->type) {
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
