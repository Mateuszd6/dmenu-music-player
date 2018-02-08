#ifndef TRACK_QUEUE_H
#define TRACK_QUEUE_H

struct QueueNode
{
    char **music_data;
    struct QueueNode *next;
};

struct Queue
{
    struct QueueNode *head, *tail;
};

extern struct Queue *track_queue;

// Gets a path to the track, and enqueues it along with its 
// metadata to the track queue. 
void Enqueue (struct Queue *queue, char *value);
void Dequeue (struct Queue *queue);

struct Queue *InitializeQueue();

// Return 0 on success and -1 if the queue was empty.
// music_data] with the all data of the file.
int Peek(struct Queue *queue, char ***music_data);

// Return 1 if the queue is empty, and 0 if it's not.
int EmptyQueue(struct Queue *queue);

#endif