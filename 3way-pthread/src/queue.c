/* Implemenation taken from: https://www.geeksforgeeks.org/queue-linked-list-implementation/ */

#include <stdio.h>
#include <stdlib.h>

#include "../include/queue.h"

// A utility function to create a new linked list node.
struct QNode *new_node (void *d)
{
    struct QNode *temp = (struct QNode *) malloc (sizeof (struct QNode));
    temp->data = d;
    temp->next = NULL;
    return temp;
}

// A utility function to create an empty queue
struct Queue *create_queue ()
{
    struct Queue *q = (struct Queue *) malloc (sizeof (struct Queue));
    q->front = q->rear = NULL;
    q->count = 0;
    return q;
}

// The function to add a key k to q
void enqueue (struct Queue *q, void *data)
{
    // Create a new LL node
    struct QNode *temp = new_node (data);

    // If queue is empty, then new node is front and rear both
    if (q->rear == NULL)
    {
        q->front = q->rear = temp;
    }
    else
    {
        // Add the new node at the end of queue and change rear
        q->rear->next = temp;
        q->rear = temp;
    }
    
    q->count += 1;
}

// Function to remove a key from given queue q
void *dequeue (struct Queue *q)
{
    // If queue is empty, return NULL.
    if (q->front == NULL)
        return NULL;

    // Store previous front and move front one node ahead
    struct QNode *temp = q->front;

    q->front = q->front->next;

    // If front becomes NULL, then change rear also as NULL
    if (q->front == NULL)
        q->rear = NULL;

    q->count -= 1;

    return temp->data;
}