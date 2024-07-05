#include "code_generator.h"

void *Transpile(Node_LinkedList *ast) {
    FILE *outFile = fopen(OUTPUT_PATH"/assign_statement.c", "wt");
    fprintf(outFile, "void main(void) {\n");
    Node_Node *current = ast->head;
    while (current != NULL) {
        fprintf(outFile, "\t%s\n", VisitNode(current->data));
        current = current->next;
    }
    fprintf(outFile, "\treturn 0;\n");
    fprintf(outFile, "}");
    fclose(outFile);
}

const char *VisitNode(Node *node) {
    switch (node->type) {
        case ASSIGNMENT:
            return TextFormat("%s %s = %s;",
                              PyToCType(node->assignStmt->target->type),
                              node->assignStmt->target->id,
                              VisitNode(node->assignStmt->value));
        case LITERAL:
            return TextFormat("%s", node->literal->value);
        case BINARY_OPERATION:
            return TextFormat("%s %s %s",
                              VisitNode(node->binOp->left),
                              node->binOp->operator,
                              VisitNode(node->binOp->right));
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





