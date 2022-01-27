#include "queue.h"

#include <stdlib.h>
#include <string.h>

/* Task */

Task_t
Task_create(const task_fn function, void *const argument)
{
        return (Task_t) {
                .function = function,
                .argument = argument,
                .next     = NULL
        };
}

void *
Task_execute(const Task_t *const task)
{
        if (task == NULL) {
                return NULL;
        }

        void *result = NULL;
        task_fn func = task->function;
        void *args = task->argument;

        result = func(args);

        return result;
}

/* Queue */

Queue_t *
Queue_new(void)
{
        Queue_t *new_queue = malloc(sizeof(Queue_t));
        if (new_queue == NULL) {
                return NULL;
        }

        *new_queue = (Queue_t) {
                .cancel  = 0,
                .length  = 0,
                .head    = NULL,
                .tail    = NULL
        };

        if (pthread_mutex_init(&new_queue->lock, NULL) != 0) {
                free(new_queue);
                return NULL;
        }

        if (pthread_cond_init(&new_queue->notify, NULL) != 0) {
                pthread_mutex_destroy(&new_queue->lock);
                free(new_queue);
                return NULL;
        }

        return new_queue;
}

static inline void
increase_queue_length(Queue_t *const queue)
{
        queue->length += 1;
}

static inline void
decrease_queue_length(Queue_t *const queue)
{
        if (queue->length > 0) {
                queue->length -= 1;
        }
}

static inline size_t
queue_length(const Queue_t *queue)
{
        return queue->length;
}

static inline int8_t
queue_is_empty(const Queue_t *queue)
{
        return queue->tail == NULL;
}

int8_t
Queue_enqueue(Queue_t *const queue, const Task_t *task)
{
        if (queue->cancel || queue == NULL || task == NULL) {
                return -1;
        }

        Task_t *new_task = malloc(sizeof(Task_t));
        if (new_task == NULL) {
                return -1;
        }

        *new_task = (Task_t) {
                .function = task->function,
                .argument = task->argument,
                .next     = NULL,
        };

        if (queue->head == NULL) {
                queue->head = new_task;
        }

        if (!queue_is_empty(queue)) {
                queue->tail->next = new_task;
        }

        queue->tail = new_task;

        increase_queue_length(queue);

        if(pthread_cond_signal(&queue->notify) != 0) {
                return -1;
        }

        return 0;
}

int8_t
Queue_dequeue(Queue_t *const queue, Task_t *dest)
{
        if (queue != NULL) {
                if(pthread_mutex_lock(&queue->lock) != 0) {
                        return -1;
                }

                while(queue_length(queue) == 0 && (!queue->cancel)) {
                        pthread_cond_wait(&queue->notify, &queue->lock);
                }

                if (queue->cancel) {
                        pthread_mutex_unlock(&queue->lock);
                        return -1;
                }

                Task_t *head = queue->head;

                if(queue_length(queue) == 1) {
                        queue->head = NULL;
                        queue->tail = NULL;
                } else {
                        queue->head = head->next;
                }

                if (dest != NULL && head != NULL) {
                        memmove(dest, head, sizeof(Task_t));
                }

                free(head);

                decrease_queue_length(queue);

                if(pthread_mutex_unlock(&queue->lock) != 0) {
                        return -1;
                }

                return 0;
        }

        return -1;
}

int8_t
Queue_cancel(Queue_t *const queue)
{
        if (queue == NULL) {
                return -1;
        }

        pthread_mutex_lock(&queue->lock);

        queue->cancel = 1;

        pthread_cond_broadcast(&queue->notify);
        pthread_mutex_unlock(&queue->lock);

        return 0;
}

int8_t
Queue_free(Queue_t **queue)
{
        if (queue != NULL && *queue != NULL && (*queue)->cancel) {
                pthread_mutex_lock(&(*queue)->lock);

                Task_t *head = (*queue)->head;
                while(head != NULL) {
                        Task_t *node = head;
                        head = head->next;
                        free(node);
                }

                (*queue)->tail = NULL;
                (*queue)->length = 0;

                pthread_mutex_destroy(&(*queue)->lock);
                pthread_cond_destroy(&(*queue)->notify);

                free(*queue);
                *queue = NULL;

                return 0;
        }

        return -1;
}
