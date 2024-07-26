#include "code_generator.h"

static size_t currentRegister = 0;
static size_t lastRegister = 0;

const char *Transpile(Parser *parser) {
    return TranspileBlock(parser, "int main(void) {\n", "\treturn 0;\n}");
}

const char *CompileIR(Parser *parser) {
    size_t cap = 1024;
    char *outputStr = AllocateContext(cap);
    memset(outputStr, 0, cap);
    size_t outLen = strlen(outputStr);
    Node_Node *current = parser->ast.head;
    while (current != NULL) {
        const char *nodeStr = GenerateIR(current->data, parser->context);
        size_t nodeStrLen = strlen(nodeStr) + 3;
        // Reallocate if necessary
        while (outLen + nodeStrLen >= cap) {
            cap *= 2;
            outputStr = ReallocateContext(outputStr, outLen, cap);
            if (!outputStr) {
                perror("ERROR: Unable to allocate memory for code");
                exit(EXIT_FAILURE);
            }
        }

        outLen += snprintf(outputStr + outLen, cap - outLen, "%s\n", nodeStr);
        current = current->next;
    }

    return outputStr;
}

const char *TranspileBlock(Parser *parser, const char *initStr, const char *finalStr) {
    size_t cap = 1024;
    char *outputStr = AllocateContext(cap);
    strcpy(outputStr, initStr);
    size_t outLen = strlen(outputStr);
    Node_Node *current = parser->ast.head;
    while (current != NULL) {
        const char *nodeStr = TranspileNode(current->data, parser->context);
        size_t nodeStrLen = strlen(nodeStr) + 3;
        // Reallocate if necessary
        while (outLen + nodeStrLen >= cap) {
            cap *= 2;
            outputStr = ReallocateContext(outputStr, outLen, cap);
            if (!outputStr) {
                perror("ERROR: Unable to allocate memory for code");
                exit(EXIT_FAILURE);
            }
        }

        outLen += snprintf(outputStr + outLen, cap - outLen, "\t%s\n", nodeStr);
        current = current->next;
    }

    size_t finalStrLen = strlen(finalStr) + 1; // +1 for \0
    if (outLen + finalStrLen >= cap) {
        cap = outLen + finalStrLen;
        outputStr = ReallocateContext(outputStr, outLen, cap);
        if (!outputStr) {
            perror("ERROR: Unable to reallocate memory for code generation");
            exit(EXIT_FAILURE);
        }
    }

    strcat(outputStr, finalStr);
    return outputStr;
}

const char *TranspileNode(Node *node, Symbol *namespace) {
    switch (node->type) {
        case ASSIGNMENT:
            Symbol *sym = StackSymbolsLookup(namespace, node->assignStmt->target->id);
            const char *nodeCode = TranspileNode(node->assignStmt->value, namespace);
            return sym->col == node->assignStmt->target->col && sym->line == node->assignStmt->target->line ?
                   TextFormat("%s %s = %s;",
                              PyToCType(node->assignStmt->target->type),
                              node->assignStmt->target->id,
                              nodeCode) : TextFormat("%s = %s;", node->assignStmt->target->id, nodeCode);
        case LITERAL:
            return TextFormat("%s", node->literal->value);
        case BINARY_OPERATION:
            const char *left = TranspileNode(node->binOp->left, namespace);
            const char *right = TranspileNode(node->binOp->right, namespace);
            size_t currentPrecedence = Precedence(node->binOp->operator);
            size_t leftPrecedence = Precedence(node->binOp->left->binOp->operator);
            size_t rightPrecedence = Precedence(node->binOp->right->binOp->operator);
            const char *leftStr = (leftPrecedence < currentPrecedence) ? TextFormat("(%s)", left) : left;
            const char *rightStr = (rightPrecedence < currentPrecedence) ? TextFormat("(%s)", right) : right;
            return TextFormat("%s %s %s", leftStr, node->binOp->operator, rightStr);
        case UNARY_OPERATION:
            return TextFormat("%s%s", node->unOp->operator, TranspileNode(node->unOp->operand, namespace));
        case VARIABLE:
            return node->variable->id;
        default:
            return "";
    }
}

const char *GenerateIR(Node *node, Symbol *namespace) {
    switch (node->type) {
        case ASSIGNMENT: {
            const char *target = node->assignStmt->target->id;
            const char *value = GenerateIR(node->assignStmt->value, namespace);
            const char *targetType = PyToIRType(node->assignStmt->target->type);
            return node->assignStmt->value->type == BINARY_OPERATION ?
                   TextFormat("%s\nstore %s %%%d, %s* %s", value,
                              targetType,
                              currentRegister, targetType, target) :
                   TextFormat("%%%d = add %s 0, %s\n"
                              "store %s %%%d, %s* %s",
                              currentRegister++, targetType, value,
                              targetType, currentRegister,
                              targetType, target);
        }
        case LITERAL:
            return TextFormat("%s", node->literal->value);
        case VARIABLE:
            return node->variable->id;
        case BINARY_OPERATION: {
            const char *left = GenerateIR(node->binOp->left, namespace);
            const char *right = GenerateIR(node->binOp->right, namespace);
            const char *op;
            if (strcmp(node->binOp->operator, "+") == 0) {
                op = "add";
            } else if (strcmp(node->binOp->operator, "-") == 0) {
                op = "sub";
            } else if (strcmp(node->binOp->operator, "*") == 0) {
                op = "mul";
            } else if (strcmp(node->binOp->operator, "*") == 0) {
                op = "sdiv";  // Assuming signed division
            } else return "";
            if (node->binOp->left->type == BINARY_OPERATION) {
                return TextFormat("%s\n%%%d = %s %s %%%d, %s",
                                  left,
                                  ++currentRegister,
                                  op,
                                  PyToIRType(node->binOp->type),
                                  currentRegister,
                                  right);
            } else if (node->binOp->right->type == BINARY_OPERATION) {
                return TextFormat("%s\n%%%d = %s %s %s, %%%d",
                                  right,
                                  ++currentRegister,
                                  op,
                                  PyToIRType(node->binOp->type),
                                  left,
                                  currentRegister);
            }
            return TextFormat("\n%%%d = %s %s %s, %s",
                              currentRegister,
                              op,
                              PyToIRType(node->binOp->type),
                              left,
                              right);
        }
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





