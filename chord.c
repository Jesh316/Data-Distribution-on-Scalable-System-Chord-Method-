#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_FINGERS 10

typedef struct Node {
    int id;
    struct Node* fingers[MAX_FINGERS]; // Finger table
    struct Node* successor;
    struct Node* predecessor;
    pthread_mutex_t lock; // Mutex lock for the node
} Node;

Node* createNode(int id);
void join(Node* n, Node* existingNode);
void stabilize(Node* n);
void notify(Node* n, Node* successor);
Node* findSuccessor(Node* n, int id);
Node* findPredecessor(Node* n, int id);
Node* closestPrecedingFinger(Node* n, int id);
void printNode(Node* n);

Node* createNode(int id) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->id = id;
    newNode->successor = newNode;
    newNode->predecessor = NULL;
    pthread_mutex_init(&newNode->lock, NULL); // Initialize the mutex lock
    for (int i = 0; i < MAX_FINGERS; i++) {
        newNode->fingers[i] = newNode;
    }
    return newNode;
}

void join(Node* n, Node* existingNode) {
    pthread_mutex_lock(&n->lock);
    if (existingNode != NULL) {
        printf("Node %d is joining via existing node %d\n", n->id, existingNode->id);
        n->predecessor = NULL;
        n->successor = findSuccessor(existingNode, n->id);
    } else {
        // Only node in the network
        printf("Node %d is the first node in the network\n", n->id);
        n->predecessor = n;
        n->successor = n;
    }
    pthread_mutex_unlock(&n->lock);
}

void stabilize(Node* n) {
    pthread_mutex_lock(&n->lock);
    printf("Stabilizing node %d\n", n->id);
    Node* x = NULL;
    pthread_mutex_lock(&n->successor->lock);
    x = n->successor->predecessor;
    pthread_mutex_unlock(&n->successor->lock);

    if (x != NULL && x->id > n->id && x->id < n->successor->id) {
        n->successor = x;
    }
    notify(n, n->successor);
    pthread_mutex_unlock(&n->lock);
}

void notify(Node* n, Node* successor) {
    pthread_mutex_lock(&successor->lock);
    printf("Node %d is notifying its successor %d\n", n->id, successor->id);
    if (successor->predecessor == NULL || (n->id > successor->predecessor->id && n->id < successor->id)) {
        successor->predecessor = n;
    }
    pthread_mutex_unlock(&successor->lock);
}

Node* findSuccessor(Node* n, int id) {
    pthread_mutex_lock(&n->lock);
    Node* pred = findPredecessor(n, id);
    Node* succ = pred->successor;
    pthread_mutex_unlock(&n->lock);
    return succ;
}

Node* findPredecessor(Node* n, int id) {
    pthread_mutex_lock(&n->lock);
    Node* current = n;
    while (!(id > current->id && id <= current->successor->id)) {
        printf("findPredecessor: current = %d, successor = %d\n", current->id, current->successor->id);
        Node* next = closestPrecedingFinger(current, id);
        if (next == current) {
            break;
        }
        pthread_mutex_unlock(&current->lock);
        current = next;
        pthread_mutex_lock(&current->lock);
    }
    pthread_mutex_unlock(&current->lock);
    return current;
}

Node* closestPrecedingFinger(Node* n, int id) {
    pthread_mutex_lock(&n->lock);
    for (int i = MAX_FINGERS - 1; i >= 0; i--) {
        if (n->fingers[i]->id > n->id && n->fingers[i]->id < id) {
            printf("closestPrecedingFinger: node %d found closer finger %d for id %d\n", n->id, n->fingers[i]->id, id);
            pthread_mutex_unlock(&n->lock);
            return n->fingers[i];
        }
    }
    pthread_mutex_unlock(&n->lock);
    return n;
}

void printNode(Node* n) {
    pthread_mutex_lock(&n->lock);
    printf("Node %d: successor = %d, predecessor = %d\n",
           n->id, n->successor->id, n->predecessor ? n->predecessor->id : -1);
    pthread_mutex_unlock(&n->lock);
}

int main() {
    // Create initial nodes
    Node* node1 = createNode(1);
    join(node1, NULL);
    Node* node2 = createNode(2);
    join(node2, node1);
    Node* node3 = createNode(3);
    join(node3, node1);

    // Stabilize nodes
    stabilize(node1);
    stabilize(node2);
    stabilize(node3);

    // Print node states
    printNode(node1);
    printNode(node2);
    printNode(node3);

    // Free memory
    pthread_mutex_destroy(&node1->lock);
    pthread_mutex_destroy(&node2->lock);
    pthread_mutex_destroy(&node3->lock);
    free(node1);
    free(node2);
    free(node3);

    return 0;
}

