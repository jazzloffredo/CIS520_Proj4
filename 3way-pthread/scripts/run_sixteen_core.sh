#!/bin/bash

# Specify the amount of RAM needed _per_core_.
#SBATCH --mem-per-cpu=1G

# Specify the maximum runtime in DD-HH:MM:SS form.
#SBATCH --time=00-00:10:00

# Number of cores/nodes.
#SBATCH --nodes=1 --tasks=1 --cpus-per-task=16

# Constraints for this job. Maybe you need to run on the elves.
#SBATCH --constraint=elves

# Name my job, to make it easier to find in the queue.
#SBATCH -J Scorecard_PTHREAD_16

# And finally, we run the job we came here to do.
$HOME/CIS520/Proj4/3way-pthread/pthread $THREADS