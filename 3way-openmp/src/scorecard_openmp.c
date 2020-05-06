/* Standard libraries. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Parallel libraries. */
#include <omp.h>
#include <pthread.h>
#include <semaphore.h>

/* Custom libraries. */
#include "../include/queue.h"

#define MAX_ENTRIES_PER_READ 10000
#define NUM_COMPUTE_THREADS 4

struct Queue *input_queue;  // Stores datasets that are ready to be computed with.
struct Queue *output_queue; // Stores datasets that are ready to be output to stdout.

pthread_mutex_t inq_lock;  // Mutex lock to protect input_queue when multiple threads are enq/deq.
pthread_mutex_t outq_lock; // Mutex lock to protect output_queue when multiple threads are enq/deq.

sem_t comp_barrier;

int input_complete_flag;
int computation_complete_flag;

struct dataset
{
    int line_start;
    int line_count;
    long line_scores[MAX_ENTRIES_PER_READ];
};

FILE *try_open_file(char *);
int try_close_file(FILE *);
void batch_compute();
void batch_calc_line_diffs(int, struct dataset *);
void *input_scores(void *);
void *output_scores(void *);
void safe_add_batch_to_queue(struct Queue *, struct dataset *);
struct dataset *safe_remove_batch_from_queue(struct Queue *);

int main(int argc, char *argv[])
{
    /* Initialize OMP. */
    omp_set_num_threads(NUM_COMPUTE_THREADS);

    /* Initialize locks. */
    pthread_mutex_init(&inq_lock, NULL);
    pthread_mutex_init(&outq_lock, NULL);

    /* Initialize queues. */
    input_queue = create_queue();
    output_queue = create_queue();

    /* Initialize flags. */
    input_complete_flag = 0;
    computation_complete_flag = 0;

    /* Grab file path from cmdline argument. Default to wiki_dump. */
    char *path = "/homes/dan/625/wiki_dump.txt";
    if (argc > 1)
    {
        path = argv[1];
    }

    /* Try opening file. If file does not exist, exit. */
    FILE *f = try_open_file(path);
    if (f == NULL)
    {
        printf("Attempt to open file at - %s - failed! Program exiting!\n", path);
        exit(EXIT_FAILURE);
    }

    /* I/O thread creation. */
    int in_ret_code, out_ret_code;
    pthread_t input_thread, output_thread;
    pthread_attr_t out_attr;
    void *out_status;

    pthread_attr_init(&out_attr);
    pthread_attr_setdetachstate(&out_attr, PTHREAD_CREATE_JOINABLE);

    /* Begin threads dedicated to I/O. */
    in_ret_code = pthread_create(&input_thread, NULL, input_scores, (void *)f);
    out_ret_code = pthread_create(&output_thread, &out_attr, output_scores, NULL);

    if (in_ret_code)
    {
        printf("ERROR: Return code from pthread_create(&input_thread) is %d.\n", in_ret_code);
        exit(-1);
    }

    if (out_ret_code)
    {
        printf("ERROR: Return code from pthread_create(&output_thread) is %d.\n", out_ret_code);
        exit(-1);
    }

    /* Begin computing! */
    batch_compute();

    /* Wait for output thread to finish. */
    out_ret_code = pthread_join(output_thread, &out_status);
    if (out_ret_code)
    {
        printf("ERROR: Return code from pthread_join(&output_thread) is %d.\n", out_ret_code);
        exit(-1);
    }

    /* Cleanup. */
    pthread_mutex_destroy(&inq_lock);
    pthread_mutex_destroy(&outq_lock);
    pthread_attr_destroy(&out_attr);

    return 0;
}

FILE *try_open_file(char *path)
{
    return fopen(path, "r");
}

int try_close_file(FILE *f)
{
    return fclose(f);
}

void batch_compute()
{
    while (!input_complete_flag || input_queue->count != 0)
    {
        struct dataset *b = safe_remove_batch_from_queue(input_queue);

        if (b != NULL)
        {
            sem_init(&comp_barrier, 0, NUM_COMPUTE_THREADS);
            #pragma omp parallel
            {
                batch_calc_line_diffs(omp_get_thread_num(), b);
            }
            sem_destroy(&comp_barrier);

            safe_add_batch_to_queue(output_queue, b);
        }
    }

    computation_complete_flag = 1;

    pthread_exit(NULL);
}

void batch_calc_line_diffs(int myID, struct dataset *b)
{
    int i, startPos, endPos;
    int local_score_diffs[MAX_ENTRIES_PER_READ - 1];

    #pragma omp private(myID, local_score_diffs, startPos, endPos, i)
    {
        startPos = myID * (MAX_ENTRIES_PER_READ / NUM_COMPUTE_THREADS);
        endPos = startPos + (MAX_ENTRIES_PER_READ / NUM_COMPUTE_THREADS);

        /* Protects against going outside bounds of array. */
        if (myID == NUM_COMPUTE_THREADS - 1)
            endPos -= 1;

        for (int i = startPos; i < endPos; i++)
        {
            local_score_diffs[i] = b->line_scores[i] - b->line_scores[i + 1];
        }

        sem_wait(&comp_barrier);

        int sem_val;
        sem_getvalue(&comp_barrier, &sem_val);

        while (sem_val > 0)
        {
            #pragma omp taskyield
            sem_getvalue(&comp_barrier, &sem_val);
        }

        for (i = startPos; i < endPos; i++)
        {
            b->line_scores[i] = local_score_diffs[i];
        }
    }
}

void *input_scores(void *f)
{
    FILE *file = (FILE *)f;

    int ch = 0;
    int line_counter = 0;
    long score_counter = 0;

    struct dataset *batch = (struct dataset *)malloc(sizeof(struct dataset));
    batch->line_start = 0;
    batch->line_count = 0;

    while (!feof(f))
    {
        int ch = fgetc(f);
        if (ch == EOF && batch->line_count > 0)
        {
            safe_add_batch_to_queue(input_queue, batch);
        }
        else if (ch == '\n')
        {
            batch->line_scores[(line_counter++) % MAX_ENTRIES_PER_READ] = score_counter;
            ++batch->line_count;
            score_counter = 0;
            if (line_counter % MAX_ENTRIES_PER_READ == 0)
            {
                safe_add_batch_to_queue(input_queue, batch);

                /* Prep a new batch. */
                batch = (struct dataset *)malloc(sizeof(struct dataset));
                batch->line_start = line_counter;
                batch->line_count = 0;
            }
        }
        else
        {
            score_counter += ch;
        }
    }

    input_complete_flag = 1;

    pthread_exit(NULL);
}

void *output_scores(void *v)
{
    while (!computation_complete_flag || output_queue->count != 0)
    {
        struct dataset *b = safe_remove_batch_from_queue(output_queue);

        if (b != NULL)
        {
            for (int i = b->line_start; i < b->line_start + MAX_ENTRIES_PER_READ; i++)
            {
                printf("%d-%d: %ld\n", i, i + 1, b->line_scores[i % MAX_ENTRIES_PER_READ]);
                fflush(stdout);
            }

            /* Cleanup. No longer need dataset. */
            free(b);
        }
    }

    pthread_exit(NULL);
}

void safe_add_batch_to_queue(struct Queue *q, struct dataset *b)
{
    /* Grab lock to protect queue, enqueue, release lock. */
    pthread_mutex_lock(&inq_lock);
    enqueue(q, (void *)b);
    pthread_mutex_unlock(&inq_lock);
}

struct dataset *safe_remove_batch_from_queue(struct Queue *q)
{
    /* Grab lock to protect queue, dequeue, release lock. */
    pthread_mutex_lock(&inq_lock);
    struct dataset *b = (struct dataset *)dequeue(q);
    pthread_mutex_unlock(&inq_lock);

    return b;
}
