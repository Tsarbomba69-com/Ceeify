#include "code_generator.h"

static size_t currentRegister = 0;

static int AllocateRegister() {
    return currentRegister++;
}

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
            size_t targetReg = AllocateRegister();
            const char *dt = PyToIRType(node->assignStmt->target->type);
            const char *value = GenerateIR(node->assignStmt->value, namespace);
            const char *result = TextFormat("store %%%d, %s* %s", targetReg, dt,
                                            node->assignStmt->target->id);
            return TextFormat("%s\n%s", value, result);
        }
        case LITERAL:
            size_t reg = AllocateRegister();
            return TextFormat("%%%d = add %s 0, %s",
                              reg, PyToIRType(node->binOp->type), node->literal->value);
        case VARIABLE:
            size_t varReg = AllocateRegister();
            const char *dt = PyToIRType(node->variable->type);
            return TextFormat("%%%d = load %s, %s* %s", varReg, dt, dt, node->variable->id);
        case BINARY_OPERATION: {
            const char *left = GenerateIR(node->binOp->left, namespace);
            const char *right = GenerateIR(node->binOp->right, namespace);
            size_t resultReg = AllocateRegister();
            if (strcmp(node->binOp->operator, "+") == 0) {
                const char *result = TextFormat("%%%d = add %s %%%d, %%%d",
                                                resultReg, PyToIRType(node->binOp->type), currentRegister - 1,
                                                currentRegister - 2);
                return TextFormat("%s\n%s\n%s", left, right, result);
            } else if (strcmp(node->binOp->operator, "-") == 0) {
                const char *result = TextFormat("%%%d = sub %s %%%d, %%%d",
                                                resultReg, PyToIRType(node->binOp->type), currentRegister - 1,
                                                currentRegister - 2);
                return TextFormat("%s\n%s\n%s", left, right, result);
            } else if (strcmp(node->binOp->operator, "*") == 0) {
                const char *result = TextFormat("%%%d = mul %s %%%d, %%%d",
                                                resultReg, PyToIRType(node->binOp->type), currentRegister - 1,
                                                currentRegister - 2);
                return TextFormat("%s\n%s\n%s", left, right, result);
            }
            return "";
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





