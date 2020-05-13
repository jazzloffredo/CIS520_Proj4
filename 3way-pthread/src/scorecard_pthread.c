/* Standard libraries. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/* Parallel libraries. */
#include <pthread.h>
#include <semaphore.h>

/* Custom libraries. */
#include "../include/queue.h"

/* Custom definitions. */
#define MAX_ENTRIES_PER_READ 10000

/* For measuring performance. */
double overall_elapsed, input_elapsed, compute_elapsed, output_elapsed;

int NUM_COMPUTE_THREADS;       // Number of threads to compute in parallel, taken from first cmdline arg, default is 1.
struct Queue *input_queue;     // Stores datasets that are ready to be computed with.
struct Queue *output_queue;    // Stores datasets that are ready to be output to stdout.
struct dataset *working_set;   // The current dataset that is being computed upon in parallel.
pthread_mutex_t inq_lock;      // Mutex lock to protect input_queue when multiple threads are enq/deq.
pthread_mutex_t outq_lock;     // Mutex lock to protect output_queue when multiple threads are enq/deq.
pthread_mutex_t barrier_lock;  // Barrier lock for pthreads to synch back up when calculating diffs.
pthread_cond_t barrier_cv;     // Barrier condition var for pthreads to synch back up when calculating diffs.
int num_threads_done;          // Counter variable to determine when to broadcast cond var.
int input_complete_flag;       // Signals entire file has been read.
int computation_complete_flag; // Signals all score diffs have been calculated.

/* Data structure to hold batch reads. */
struct dataset
{
    int line_start;
    int num_entries;
    long line_scores[MAX_ENTRIES_PER_READ];
};

/* Function prototypes. */
void init_vars();
void cleanup_vars();
void output_performance();
void *input_scores(void *);
void *compute_scores(void *);
void *output_scores(void *);
void *calc_line_diffs(void *); // Parallel function using PTHREADS.
FILE *try_open_file(char *);
int try_close_file(FILE *);
void safe_add_batch_to_queue(struct Queue *, pthread_mutex_t, struct dataset *);
struct dataset *safe_remove_batch_from_queue(struct Queue *, pthread_mutex_t);

void init_vars()
{
    /* Initialize timer vars. */
    overall_elapsed = 0;
    input_elapsed = 0;
    compute_elapsed = 0;
    output_elapsed = 0;

    /* Initialize locks. */
    pthread_mutex_init(&inq_lock, NULL);
    pthread_mutex_init(&outq_lock, NULL);

    /* Initialize queues. */
    input_queue = create_queue();
    output_queue = create_queue();

    /* Initialize barrier. */
    pthread_mutex_init(&barrier_lock, NULL);
    pthread_cond_init(&barrier_cv, NULL);

    /* Initialize flags. */
    input_complete_flag = 0;
    computation_complete_flag = 0;
}

void cleanup_vars()
{
    free(input_queue);
    free(output_queue);
    pthread_mutex_destroy(&inq_lock);
    pthread_mutex_destroy(&outq_lock);
}

void output_performance()
{
    printf("TIME, OVERALL, %f ms\n", overall_elapsed);
    printf("TIME, INPUT, %f ms\n", input_elapsed);
    printf("TIME, COMPUTE, %f ms\n", compute_elapsed);
    printf("TIME, OUTPUT, %f ms\n", output_elapsed);

    printf("DATA, VERSION, Pthread\n");
    printf("DATA, NUM OF CORES, %s\n", getenv("cpus-per-task"));
    printf("DATA, COMP THREADS, %d\n", NUM_COMPUTE_THREADS);

    fflush(stdout);
}

void *compute_scores(void *n)
{
    /* Thread initialization. */
    pthread_t compute_threads[NUM_COMPUTE_THREADS];
    pthread_attr_t attr;
    void *status;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    struct timeval compute_start, compute_end;

    while (!input_complete_flag || input_queue->count > 0)
    {
        working_set = safe_remove_batch_from_queue(input_queue, inq_lock);

        if (working_set != NULL)
        {
            /* Start compute timer. */
            gettimeofday(&compute_start, NULL);

            num_threads_done = 0;

            for (int i = 0; i < NUM_COMPUTE_THREADS; i++)
            {
                int rc = pthread_create(&compute_threads[i], &attr, calc_line_diffs, (void *)i);
                if (rc)
                {
                    printf("ERROR: Return code from pthread_create() was %d.\n", rc);
                    exit(-1);
                }
            }

            for (int j = 0; j < NUM_COMPUTE_THREADS; j++)
            {
                int rc = pthread_join(compute_threads[j], &status);
                if (rc)
                {
                    printf("ERROR: Return code from pthread_join() was %d.\n", rc);
                    exit(-1);
                }
            }

            safe_add_batch_to_queue(output_queue, outq_lock, working_set);

            /* Stop compute timer and add time elapsed. */
            gettimeofday(&compute_end, NULL);
            compute_elapsed += ((compute_end.tv_sec - compute_start.tv_sec) * 1000) + ((compute_end.tv_usec - compute_start.tv_usec) / 1000);
        }
    }

    computation_complete_flag = 1;

    pthread_attr_destroy (&attr);
    pthread_exit(NULL);
}

/* Parallel function using OMP. */
void *calc_line_diffs(void *myID)
{
    int startPos, endPos, i;
    int local_score_diffs[MAX_ENTRIES_PER_READ - 1];

    startPos = ((int) myID) * (MAX_ENTRIES_PER_READ / NUM_COMPUTE_THREADS);
    endPos = startPos + (MAX_ENTRIES_PER_READ / NUM_COMPUTE_THREADS);

    /* Protect against going outside bounds of array. */
    if (((int) myID) == NUM_COMPUTE_THREADS - 1)
        endPos = working_set->num_entries - 1;

    for (int i = startPos; i < endPos; i++)
        local_score_diffs[i] = working_set->line_scores[i] - working_set->line_scores[i + 1];

    /* Wait for all pthreads to finish diffs. */
    pthread_mutex_lock(&barrier_lock);
    num_threads_done += 1;
    if (num_threads_done < NUM_COMPUTE_THREADS)
    {
        pthread_cond_wait(&barrier_cv, &barrier_lock);
    }
    else 
    {
        pthread_cond_broadcast(&barrier_cv);
    }
    pthread_mutex_unlock(&barrier_lock);

    /* Update working set to reflect line diffs. */
    for (i = startPos; i < endPos; i++)
    {
        working_set->line_scores[i] = local_score_diffs[i];
    }

    pthread_exit(NULL);
}

void *input_scores(void *f)
{
    FILE *file = (FILE *)f;

    int ch = 0;
    int line_counter = 0;
    long score_counter = 0;

    struct dataset *batch = (struct dataset *)malloc(sizeof(struct dataset));
    batch->line_start = 0;
    batch->num_entries = 0;

    struct timeval input_start, input_end;
    gettimeofday(&input_start, NULL);

    while (!feof(f))
    {
        int ch = fgetc(f);
        if (ch == EOF && batch->num_entries > 0)
        {
            /* Add partial batch to queue. */
            safe_add_batch_to_queue(input_queue, inq_lock, batch);
        }
        else if (ch == '\n')
        {
            batch->line_scores[(line_counter++) % MAX_ENTRIES_PER_READ] = score_counter;
            batch->num_entries += 1;
            score_counter = 0;
            if (line_counter % MAX_ENTRIES_PER_READ == 0)
            {
                /* Add full batch to queue. */
                safe_add_batch_to_queue(input_queue, inq_lock, batch);

                /* Prep a new batch. */
                batch = (struct dataset *)malloc(sizeof(struct dataset));
                batch->line_start = line_counter;
                batch->num_entries = 0;

                /* Add time to read batch. */
                gettimeofday(&input_end, NULL);
                input_elapsed += ((input_end.tv_sec - input_start.tv_sec) * 1000) + ((input_end.tv_usec - input_start.tv_usec) / 1000);
                gettimeofday(&input_start, NULL);
            }
        }
        else
        {
            score_counter += ch;
        }
    }

    /* Add time to read last batch. */
    gettimeofday(&input_end, NULL);
    input_elapsed += ((input_end.tv_sec - input_start.tv_sec) * 1000) + ((input_end.tv_usec - input_start.tv_usec) / 1000);

    /* Signal to compute threads that input is complete. */
    input_complete_flag = 1;

    /* Close file stream. */
    try_close_file(file);

    pthread_exit(NULL);
}

void *output_scores(void *v)
{
    struct timeval output_start, output_end;

    while (!computation_complete_flag || output_queue->count != 0)
    {
        struct dataset *b = safe_remove_batch_from_queue(output_queue, outq_lock);

        if (b != NULL)
        {
            /* Start output timer. */
            gettimeofday(&output_start, NULL);

            for (int i = b->line_start; i < b->line_start + MAX_ENTRIES_PER_READ; i++)
            {
                printf("%d-%d: %ld\n", i, i + 1, b->line_scores[i % MAX_ENTRIES_PER_READ]);
            }

            /* Cleanup. No longer need dataset. */
            free(b);

            /* Stop output timer and add time elapsed. */
            gettimeofday(&output_end, NULL);
            output_elapsed += ((output_end.tv_sec - output_start.tv_sec) * 1000) + ((output_end.tv_usec - output_start.tv_usec) / 1000);
        }
    }

    /* Stop output timer. */
    gettimeofday(&output_end, NULL);

    pthread_exit(NULL);
}

FILE *try_open_file(char *path)
{
    return fopen(path, "r");
}

int try_close_file(FILE *f)
{
    return fclose(f);
}

void safe_add_batch_to_queue(struct Queue *q, pthread_mutex_t l, struct dataset *b)
{
    /* Grab lock to protect queue, enqueue, release lock. */
    pthread_mutex_lock(&l);
    enqueue(q, (void *)b);
    pthread_mutex_unlock(&l);
}

struct dataset *safe_remove_batch_from_queue(struct Queue *q, pthread_mutex_t l)
{
    /* Grab lock to protect queue, dequeue, release lock. */
    pthread_mutex_lock(&l);
    struct dataset *b = (struct dataset *)dequeue(q);
    pthread_mutex_unlock(&l);

    return b;
}

int main(int argc, char *argv[])
{
    /* Initialize number of compute threads. */
    if (argc > 1)
    {
        NUM_COMPUTE_THREADS = (int)strtol(argv[1], (char **)NULL, 10);
    }
    else
    {
        NUM_COMPUTE_THREADS = 1;
    }

    /* Grab file path from cmdline argument. Default to wiki_dump. */
    char *path = "/homes/dan/625/wiki_dump.txt";
    if (argc > 2)
    {
        path = argv[2];
    }

    /* Perform variable initialization. */
    init_vars();

    /* Start overall timer. */
    struct timeval overall_start, overall_end;
    gettimeofday(&overall_start, NULL);

    /* Try opening file. If file does not exist, exit. */
    FILE *f = try_open_file(path);
    if (f == NULL)
    {
        printf("Attempt to open file at - %s - failed! Program exiting!\n", path);
        exit(EXIT_FAILURE);
    }

    /* Thread initialization. */
    void *in_status, *comp_status, *out_status;
    int in_ret_code, comp_ret_code, out_ret_code;
    pthread_t input_thread, compute_thread, output_thread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    /* Begin I/O and computation threads. */
    in_ret_code = pthread_create(&input_thread, &attr, input_scores, (void *)f);
    comp_ret_code = pthread_create(&compute_thread, &attr, compute_scores, NULL);
    out_ret_code = pthread_create(&output_thread, &attr, output_scores, NULL);

    /* Standard error checking. */
    if (in_ret_code)
    {
        printf("ERROR: Return code from pthread_create(&input_thread) is %d.\n", in_ret_code);
        exit(EXIT_FAILURE);
    }

    if (comp_ret_code)
    {
        printf("ERROR: Return code from pthread_create(&compute_thread) is %d.\n", comp_ret_code);
        exit(EXIT_FAILURE);
    }

    if (out_ret_code)
    {
        printf("ERROR: Return code from pthread_create(&output_thread) is %d.\n", out_ret_code);
        exit(EXIT_FAILURE);
    }

    /* Wait for all threads to finish. Block main thread. */
    in_ret_code = pthread_join(input_thread, NULL);
    comp_ret_code = pthread_join(compute_thread, NULL);
    out_ret_code = pthread_join(output_thread, NULL);

    /* Standard error checking. */
    if (in_ret_code)
    {
        printf("ERROR: Return code from pthread_create(&input_thread) is %d.\n", in_ret_code);
        exit(EXIT_FAILURE);
    }

    if (comp_ret_code)
    {
        printf("ERROR: Return code from pthread_create(&compute_thread) is %d.\n", comp_ret_code);
        exit(EXIT_FAILURE);
    }

    if (out_ret_code)
    {
        printf("ERROR: Return code from pthread_create(&output_thread) is %d.\n", out_ret_code);
        exit(EXIT_FAILURE);
    }

    /* Stop overall timer and calculate time elapsed. */
    gettimeofday(&overall_end, NULL);
    overall_elapsed = ((overall_end.tv_sec - overall_start.tv_sec) * 1000) + ((overall_end.tv_usec - overall_start.tv_usec) / 1000);

    /* Perform cleanup. */
    cleanup_vars();

    /* Output TIME and DATA measurements. */
    output_performance();

    return 0;
}