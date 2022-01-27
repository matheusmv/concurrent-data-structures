#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

typedef void *(*task_fn)(void *);

/* Task to be executed */
typedef struct Task Task_t;
struct Task {
        task_fn function;   /* function to be run         */
        void    *argument;  /* parameters of the function */
        Task_t  *next;      /* pointer to the next task   */
};

/* Queue of tasks to be executed -> 1 producer, N consumers */
typedef struct Queue Queue_t;
struct Queue {
        int8_t          cancel;   /* does not allow enqueue & dequeue     */
        pthread_cond_t  notify;   /* conditional variable                 */
        pthread_mutex_t lock;     /* mutex                                */
        size_t          length;   /* queue length                         */
        Task_t          *head;    /* points to the beginning of the queue */
        Task_t          *tail;    /* points to the end of the queue       */
};

Task_t Task_create(const task_fn function, void *const argument);
void *Task_execute(const Task_t *const task);

Queue_t *Queue_new(void);
int8_t Queue_enqueue(Queue_t *const queue, const Task_t *task);
int8_t Queue_dequeue(Queue_t *const queue, Task_t *task);
int8_t Queue_cancel(Queue_t *const queue);
int8_t Queue_free(Queue_t **queue);

#endif /* QUEUE_H */
