/* Standard libraries. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/* Parallel libraries. */
#include <mpi.h>

/* Custom definitions. */
#define WIKI_FILE_PATH "/homes/dan/625/wiki_dump.txt"
#define MAX_ENTRIES_PER_READ 10000

/* For measuring performance. */
double overall_elapsed;

/* Custom op for reduce. */
MPI_Op op;

int NUM_COMPUTE_NODES;        // Number of individual nodes performing computations using MPI.
int NUM_BATCHES_READ;         // Number of batches read in from wiki file.
long **line_scores;           // Data structure to hold batch reads.

/* Function prototypes. */
void input_scores();
void output_scores();
void compute_scores(int pID);
void *calc_line_diffs(void *);
FILE *try_open_file(char *);
int try_close_file(FILE *f);

void init_vars()
{
    /* Initialize timer vars. */
    NUM_BATCHES_READ = 0;
    overall_elapsed = 0;
}

void subem(long *invec, long *inoutvec, int *len, MPI_Datatype *dtype)
{
    for (int i = 0; i < *len; i++)
    {
        inoutvec[i] = invec[i] - invec[i + 1];
    }
}

void compute_scores(int pID)
{
    int startPos, endPos;

    startPos = pID * (NUM_BATCHES_READ / NUM_COMPUTE_NODES);
    endPos = startPos + (NUM_BATCHES_READ / NUM_COMPUTE_NODES);

    if (pID == NUM_COMPUTE_NODES - 1)
        endPos = NUM_BATCHES_READ;

    for (int i = startPos; i < endPos; i++)
    {
        if (pID != 0)
        {
            MPI_Reduce(line_scores[i], line_scores[i], MAX_ENTRIES_PER_READ - 1, MPI_LONG, op, 0, MPI_COMM_WORLD);
        }
        else
        {
            for (int j = 0; j < MAX_ENTRIES_PER_READ - 1; j++)
            {
                line_scores[i][j] -= line_scores[i][j + 1];
            }
        }
        
    }
}

void input_scores()
{
    FILE *file = try_open_file(WIKI_FILE_PATH);
    if (file == NULL)
    {
        printf("Attempt to open file at - " WIKI_FILE_PATH " - failed! Program exiting!\n");
        exit(EXIT_FAILURE);
    }

    int ch = 0;
    int line_counter = 0;
    long score_counter = 0;

    /* Malloc to add space for first batch. */
    line_scores = (long **)malloc(sizeof(long *));
    line_scores[NUM_BATCHES_READ] = (long *)malloc(MAX_ENTRIES_PER_READ * sizeof(long));

    while (!feof(file))
    {
        int ch = fgetc(file);
        if (ch == '\n')
        {
            line_scores[NUM_BATCHES_READ][(line_counter++) % MAX_ENTRIES_PER_READ] = score_counter;
            score_counter = 0;
            if (line_counter % MAX_ENTRIES_PER_READ == 0)
            {
                ++NUM_BATCHES_READ;

                /* Peek at the next char. */
                int c = fgetc(file);
                if (c != EOF)
                {
                    /* Prep a new batch. */
                    line_scores = (long **)realloc(line_scores, ((NUM_BATCHES_READ) + 1) * sizeof(long *));
                    line_scores[NUM_BATCHES_READ] = (long *)malloc(MAX_ENTRIES_PER_READ * sizeof(long));
                }
                ungetc(c, file);
            }
        }
        else
        {
            score_counter += ch;
        }
    }

    /* Close file stream. */
    try_close_file(file);
}

void output_scores()
{
    for (int i = 0; i < NUM_BATCHES_READ; i++)
    {
        for (int j = 0; j < MAX_ENTRIES_PER_READ; j++)
        {
            printf("%d-%d: %ld\n", (MAX_ENTRIES_PER_READ * i) + j, (MAX_ENTRIES_PER_READ * i) + j + 1, line_scores[i][j]);
            fflush(stdout);
        }
    }
}

void output_performance()
{
    printf("TIME, OVERALL, %f", overall_elapsed);
    fflush(stdout);
}

FILE *try_open_file(char *path)
{
    return fopen(path, "r");
}

int try_close_file(FILE *f)
{
    return fclose(f);
}

int main(int argc, char *argv[])
{
    struct timeval start, end;
    gettimeofday(&start, NULL);

    /* Get MPI all setup. */
    int i, rc;
    int rank;
    MPI_Status Status;

    rc = MPI_Init(&argc, &argv);
    if (rc != MPI_SUCCESS)
    {
        printf("Error starting MPI program. Terminating.\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    MPI_Comm_size(MPI_COMM_WORLD, &NUM_COMPUTE_NODES);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Op_create((MPI_User_function *)subem, 0, &op);

    /* Perform some standard initialization. */
    init_vars();

    /* Main thread inputs scores. */
    if (rank == 0)
    {
        input_scores();
    }

    /* All threads wait for main thread to complete. */
    MPI_Barrier(MPI_COMM_WORLD);

    /* Broadcast num of batches read to all threads, used to initalize 2D arrays. */
    MPI_Bcast(&NUM_BATCHES_READ, 1, MPI_INT, 0, MPI_COMM_WORLD);

    /* Initialize 2D arrays on all threads that are not main thread. */
    if (rank != 0)
    {
        line_scores = (long **)malloc(NUM_BATCHES_READ * sizeof(long *));
        for (int i = 0; i < NUM_BATCHES_READ; i++)
        {
            line_scores[i] = (long *)malloc(MAX_ENTRIES_PER_READ * sizeof(long));
        }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    compute_scores(rank);

    if (rank == 0)
    {
        gettimeofday(&end, NULL);
        overall_elapsed = ((end.tv_sec - start.tv_sec) * 1000) + ((end.tv_usec - start.tv_usec) / 1000);

        output_scores();
        output_performance();
    }

    MPI_Finalize();

    return 0;
}
