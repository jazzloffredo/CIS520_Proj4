#ifndef __QUEUE_H
#define __QUEUE_H

// A linked list (LL) node to store a queue entry
struct QNode
{
    void *data;
    struct QNode *next;
};

// The queue, front stores the front node of LL and rear stores the
// last node of LL
struct Queue
{
    struct QNode *front, *rear;
    int count;
};

struct QNode *new_node (void *);
struct Queue *create_queue ();

void enqueue (struct Queue *, void *);
void *dequeue (struct Queue *);

#endif