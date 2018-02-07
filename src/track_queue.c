#include <malloc.h>
#include <assert.h>
#include <string.h>

#include "track_queue.h"

struct Queue *InitializeQueue()
{
    struct Queue *res = malloc(sizeof(struct Queue));
    res->head = NULL;
    res->tail = NULL;
    return res;
}

void DestroyQueue(struct Queue *queue)
{
    struct QueueNode *first = queue->head;
    while (first != NULL)
    {
        struct QueueNode *next = first->next;
        free(first);
        first = next;
    }
}

int EmptyQueue(struct Queue *queue)
{
    // queue->tail == NULL  =>  queue->head == NULL
    assert(queue->tail != NULL || queue->head == NULL);

    return (queue->tail == NULL);
}

char *Peek(struct Queue *queue)
{
    // queue->tail == NULL  =>  queue->head == NULL
    assert(queue->tail != NULL || queue->head == NULL);

    if (queue->head == NULL)
        return NULL;
    else
    {
        assert(queue->tail->next == NULL);
        return queue->head->value;
    }
}

void Enqueue (struct Queue *queue, char *value)
{
    struct QueueNode *last = malloc(sizeof(struct QueueNode));

    int len = strlen(value);
    last->value = malloc((len + 1) * sizeof(char));
    memcpy(last->value, value, len);
    last->value[len] = '\0';

    last->next = NULL;
    struct QueueNode *tail = queue->tail;

    if (tail == NULL)
    {
        queue->head = last;
        queue->tail = last;
    }
    else
    {
        queue->tail->next = last;
        queue->tail = last;
    }
}

void Dequeue (struct Queue *queue)
{
    struct QueueNode *first = queue->head;
    if (first != NULL)
    {
        // There was only one element in the queue...
        if (queue->tail == first)
        {
            queue->head = NULL;
            queue->tail = NULL;
        }
        else
            queue->head = first->next;
        free(first);
    }
}