#!/bin/bash

# Specify the amount of RAM needed _per_core_.
#SBATCH --mem-per-cpu=1G

# Specify the maximum runtime in DD-HH:MM:SS form.
#SBATCH --time=00-00:10:00

# Number of cores/nodes.
#SBATCH --nodes=8 --ntasks-per-node=1

# Constraints for this job. Maybe you need to run on the elves.
#SBATCH --constraint=elves

# Name my job, to make it easier to find in the queue.
#SBATCH -J Scorecard_MPI_8

module load OpenMPI

# And finally, we run the job we came here to do.
mpirun -n 8 $HOME/CIS520/Proj4/3way-mpi/mpi