#include "code_generator.h"

const char *Transpile(Node_LinkedList *ast) {
    size_t cap = 1024;
    char *outputStr = AllocateContext(cap);
    strcpy(outputStr, "int main(void) {\n");
    size_t outLen = strlen(outputStr);
    Node_Node *current = ast->head;
    while (current != NULL) {
        const char *nodeStr = TranspileNode(current->data);
        size_t nodeStrLen = strlen(nodeStr) + 3;
        // Reallocate if necessary
        while (outLen + nodeStrLen >= cap) {
            cap *= 2;
            outputStr = ReallocateContext(outputStr, outLen, cap);
            if (!outputStr) {
                fprintf(stderr, "Unable to allocate memory for code");
                exit(EXIT_FAILURE);
            }
        }

        outLen += snprintf(outputStr + outLen, cap - outLen, "\t%s\n", nodeStr);
        current = current->next;
    }

    const char *finalStr = "\treturn 0;\n}";
    size_t finalStrLen = strlen(finalStr) + 1; // +1 for \0
    if (outLen + finalStrLen >= cap) {
        cap = outLen + finalStrLen;
        outputStr = ReallocateContext(outputStr, outLen, cap);
        if (!outputStr) {
            perror("Unable to allocate memory");
            exit(EXIT_FAILURE);
        }
    }

    strcat(outputStr, finalStr);
    return outputStr;
}

const char *TranspileNode(Node *node) {
    switch (node->type) {
        case ASSIGNMENT:
            return TextFormat("%s %s = %s;",
                              PyToCType(node->assignStmt->target->type),
                              node->assignStmt->target->id,
                              TranspileNode(node->assignStmt->value));
        case LITERAL:
            return TextFormat("%s", node->literal->value);
        case BINARY_OPERATION:
            return TextFormat("(%s %s %s)",
                              TranspileNode(node->binOp->left),
                              node->binOp->operator,
                              TranspileNode(node->binOp->right));
        case UNARY_OPERATION:
            return TextFormat("%s%s", node->unOp->operator, TranspileNode(node->unOp->operand));
        case VARIABLE:
            return node->variable->id;
        default:
            return "";
    }
}

const char *PyToCType(DataType type) {
    switch (type) {
        case FLOAT:
            return "float";
        case INT:
            return "int";
        case STR:
            return "char*";
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





