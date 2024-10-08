#include "Node_linkedlist.h"

Node_LinkedList Node_CreateLinkedList() {
    Node_LinkedList list = {0};
    list.size = 0;
    list.head = NULL;
    return list;
}

Node_LinkedList *Node_AllocateLinkedList() {
    Node_LinkedList *list = malloc(sizeof(Node_LinkedList));
    if (list == NULL) {
        fprintf(stderr, "ERROR: Could not allocate memory for \"Node*\" linked list");
        return NULL;
    }
    list->size = 0;
    list->head = NULL;
    return list;
}

Node_Node *Node_CreateNode(Node *data) {
    Node_Node *newNode = malloc(sizeof(Node_Node));
    if (newNode == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate memory for \"Node*\" node\n");
        return NULL;
    }
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

void Node_AddFirst(Node_LinkedList *list, Node *data) {
    Node_Node *newNode = Node_CreateNode(data);
    newNode->next = list->head;
    list->head = newNode;
    list->size++;
}

void Node_AddLast(Node_LinkedList *list, Node *data) {
    Node_Node *newNode = Node_CreateNode(data);
    list->size++;

    if (list->head == NULL) {
        list->head = newNode;
        return;
    }

    Node_Node *temp = list->head;
    while (temp->next != NULL) {
        temp = temp->next;
    }

    temp->next = newNode;
}

void Node_ForEach(Node_LinkedList *list, Node_Action callback) {
    Node_Node *current = list->head;
    while (current != NULL) {
        callback(current->data);
        current = current->next;
    }
}

Node *Node_GetFirst(Node_LinkedList *list) {
    if (list->head == NULL) {
        perror("ERROR: List is empty\n");
        return NULL;
    }

    if (list->head->next == NULL) {
        Node *data = list->head->data;
        free(list->head);
        list->head = NULL;
        list->size--;
        return data;
    }

    Node_Node *current = list->head;
    while (current->next->next != NULL) {
        current = current->next;
    }

    Node *data = current->next->data;
    free(current->next);
    current->next = NULL;
    list->size--;
    return data;
}

Node *Node_Pop(Node_LinkedList *list) {
    if (list->head == NULL) {
        return NULL;
    }

    Node *value = list->head->data;
    Node_Node *temp = list->head;
    list->head = list->head->next;
    list->size--;
    free(temp);
    return value;
}
