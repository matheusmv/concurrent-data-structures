#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "src/queue/queue.h"

void *add_msm_to_queue(void *arg);
void *stop_all(void *arg);
void *rmv_msg_of_queue(void *arg);

void *print_message(void *message);

int main(void)
{
        Queue_t *q = Queue_new();

        pthread_t producer, threadCancel, consumers[5];

        pthread_create(&producer, NULL, add_msm_to_queue, q);
        pthread_create(&threadCancel, NULL, stop_all, q);

        for (int i = 0; i < 5; ++i) {
                pthread_create(&consumers[i], NULL, rmv_msg_of_queue, q);
        }

        assert(Queue_free(&q) == -1); /* only after Queue_cancel is called */

        pthread_join(producer, NULL);
        pthread_join(threadCancel, NULL);

        for (int i = 0; i < 5; ++i) {
                pthread_join(consumers[i], NULL);
        }

        assert(Queue_free(&q) == 0);

        return EXIT_SUCCESS;
}

void *
add_msm_to_queue(void *arg)
{
        Queue_t *q = arg;
        Task_t task = Task_create(print_message, "hello");

        while(Queue_enqueue(q, &task) != -1) {
                printf("task added successfully\n");
                sleep(1);
        }

        return NULL;
}

void *
stop_all(void *arg)
{
        Queue_t *q = arg;

        sleep(10);

        printf("thread %ld will cancel queue\n", pthread_self());

        Queue_cancel(q);

        return NULL;
}

void *
rmv_msg_of_queue(void *arg)
{
        Queue_t *q = arg;
        Task_t task;

        while (Queue_dequeue(q, &task) != -1) {
                printf("thread %ld say ", pthread_self());
                Task_execute(&task);
        }

        return NULL;
}

void *
print_message(void *message)
{
        char *msg = message;

        printf("%s\n", msg);

        return NULL;
}
