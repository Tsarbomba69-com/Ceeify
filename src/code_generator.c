#include "code_generator.h"

const char *Transpile(Parser *parser) {
    return TranspileBlock(parser, "int main(void) {\n", "\treturn 0;\n}");
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
            const Symbol *sym = StackSymbolsLookup(namespace, node->assignStmt->target->id);
            const char *nodeCode = TranspileNode(node->assignStmt->value, namespace);
            return sym->col == node->assignStmt->target->col && sym->line == node->assignStmt->target->line ?
                   TextFormat("%s %s = %s;",
                              PyToCType(node->assignStmt->target->type),
                              node->assignStmt->target->id,
                              nodeCode) : TextFormat("%s = %s;", node->assignStmt->target->id, nodeCode);
        case LITERAL:
            return TextFormat("%s", node->literal->value);
        case BINARY_OPERATION: {
            const char *left = TranspileNode(node->binOp->left, namespace);
            const char *right = TranspileNode(node->binOp->right, namespace);
            size_t currentPrecedence = Precedence(node->binOp->operator);
            size_t leftPrecedence = Precedence(node->binOp->left->binOp->operator);
            size_t rightPrecedence = Precedence(node->binOp->right->binOp->operator);
            const char *leftStr = (leftPrecedence < currentPrecedence) ? TextFormat("(%s)", left) : left;
            const char *rightStr = (rightPrecedence < currentPrecedence) ? TextFormat("(%s)", right) : right;
            return TextFormat("%s %s %s", leftStr, node->binOp->operator, rightStr);
        }
        case IF: {
            StringBuilder sb = sb_create(5);
            sb_append(&sb, TextFormat("if (%s) {\n", TranspileNode(node->ifStmt->test, namespace)));
            for (Node_Node *curr = node->ifStmt->body.head; curr != NULL; curr = curr->next) {
                sb_append(&sb, TranspileNode(curr->data, namespace));
            }

            if (node->ifStmt->orelse.size > 0) {
                if (node->ifStmt->orelse.head->data->type == IF) sb_append(&sb, "} else ");
                else sb_append(&sb, "} else {\n");
            }
            bool close_bracket = true;
            for (Node_Node *curr = node->ifStmt->orelse.head; curr != NULL; curr = curr->next) {
                close_bracket = !close_bracket;
                sb_append(&sb, TranspileNode(curr->data, namespace));
            }
            if (close_bracket) sb_append(&sb, "\n}");
            return sb.buffer;
        }
        case COMPARE: {
            StringBuilder sb = sb_create(5);
            sb_append(&sb, TranspileNode(node->compare->left, namespace));
            for (size_t i = 0; i < node->compare->ops.size; ++i) {
                Token *op = Token_Get(&node->compare->ops, i);
                const char *right = TranspileNode(Node_GetFirst(&node->compare->comparators), namespace);
                sb_append(&sb, TextFormat(" %s %s", op->lexeme, right));
                if (i < node->compare->ops.size - 1) {
                    sb_append(&sb, TextFormat(" && %s", right));
                }
            }
            return sb.buffer;
        }
        case UNARY_OPERATION:
            return TextFormat("%s%s", node->unOp->operator, TranspileNode(node->unOp->operand, namespace));
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





