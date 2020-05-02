#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define MAX_LINES_PER_READ 1000

long line_scores[MAX_LINES_PER_READ];

FILE *try_open_file (char *);
void try_close_file (FILE *);
void calculate_scorecard (FILE *);
int batch_read (FILE *);
void print_batch_results (int);
void print_time_elapsed (struct timeval *, struct timeval *);

int main (int argc, char *argv[])
{
    struct timeval start, end;

    /* Grab file path from cmdline argument. Default to wiki_dump. */
    char *path = "/homes/dan/625/wiki_dump.txt";
    if (argc > 1)
    {
        path = argv[1];
    }
    
    /* Try opening file. If file does not exist, exit. */
    FILE *f = try_open_file (path);
    if (f == NULL)
    {
        printf ("Attempt to open file at - %s - failed! Program exiting!\n", path);
        exit (EXIT_FAILURE);
    }

    /* Get start time. */
    gettimeofday (&start, NULL);
    
    /* Calculate "scorecard" for file. */
    calculate_scorecard (f);
    
    /* Get end time. */
    gettimeofday (&end, NULL);
    
    /* Close file stream. Ignore any errors. */
    try_close_file (f);

    /* Print time elapsed during calculation to stdout. */
    print_time_elapsed (&start, &end);

    return 0;
}

FILE *try_open_file (char *path)
{
    return fopen (path, "r");
}

void try_close_file (FILE *f)
{
    fclose (f);
}

void calculate_scorecard (FILE *f)
{
    while (!feof (f))
    {
        print_batch_results (batch_read (f));
    }
}

int batch_read (FILE *f)
{
    int ch = 0;
    int index = 0;
    int line_counter = 0;
    long score_counter = 0;

    /* Read in up to MAX_LINES_PER_READ wiki entries. */
    while (line_counter < MAX_LINES_PER_READ && (ch = fgetc (f)) != EOF)
    {
        if (ch == '\n')
        {
            line_scores[index++] = score_counter;
            ++line_counter;
            score_counter = 0;
        }
        else
        {
            score_counter += ch;
        }
    }

    return line_counter;
}


void print_batch_results (int lines_read)
{
    static int line_num = 0;
    for (int i = 0; i < lines_read; i++, line_num++)
    {
        printf ("%d-%d: %ld\n", line_num, line_num + 1, line_scores[i] - line_scores[i + 1]);
    }
}

void print_time_elapsed (struct timeval *s, struct timeval *e)
{
    double elapsed_time = e->tv_sec - s->tv_sec;
    printf ("DATA: %d seconds\n", (int) elapsed_time);
}