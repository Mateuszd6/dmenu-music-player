#ifndef TRACK_QUEUE_H
#define TRACK_QUEUE_H

struct QueueNode
{
    char *value;
    struct QueueNode *next;
};

struct Queue
{
    struct QueueNode *head, *tail;
};

struct Queue *track_queue;

void Enqueue (struct Queue *queue, char *value);
void Dequeue (struct Queue *queue);

struct Queue *InitializeQueue();
char *Peek(struct Queue *queue);
int EmptyQueue(struct Queue *queue);

#endif