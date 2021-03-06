#include <malloc.h>
#include <assert.h>
#include <string.h>

#include "track_queue.h"
#include "music_data.h"
#include "player.h"

struct Queue *track_queue;

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

int Peek(struct Queue *queue, char ***music_data)
{
    // queue->tail == NULL  =>  queue->head == NULL
    assert(queue->tail != NULL || queue->head == NULL);

    if (queue->head == NULL)
        return -1;
    else
    {
        assert(queue->tail->next == NULL);
        ( *music_data) = queue->head->music_data;
        return 0;
    }
}

void Enqueue (struct Queue *queue, char *value)
{
    struct QueueNode *last = malloc(sizeof(struct QueueNode));

    last->music_data = CreateMusicData();
    GetMusicDataFromMP3File(value, last->music_data);

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

    if (player_is_paused)
        UnpauseMusic();
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

        DeleteMusicData(first->music_data);
        free(first);
    }
}

void PrintQueue (struct Queue *queue)
{
    struct QueueNode *current = queue->head;
    // If the queue is empty...
    if (current == NULL)
    {
        printf("The queue is empty.\n");
        return;
    }

    printf("Here goes the queue:\n");
    while (1)
    {
        printf("TITLE: %s\n", current->music_data[DATA_TITLE]);

        if (current->next == NULL)
            break;
        else 
            current = current->next;
    }
}