#!/bin/bash

# Specify the amount of RAM needed _per_core_.
#SBATCH --mem-per-cpu=1G

# Specify the maximum runtime in DD-HH:MM:SS form.
#SBATCH --time=00-00:01:00

# Number of cores/nodes.
#SBATCH --nodes=1 --tasks=1 --cpus-per-task=1

# Constraints for this job. Maybe you need to run on the elves.
#SBATCH --constraint=elves

# Output file name. Default is slurm-%j.out where %j is the job id.
#SBATCH --output=Scorecard_Serial_Linear_%j.data

# Name my job, to make it easier to find in the queue.
#SBATCH -J Scorecard_Serial_Linear_%j

# And finally, we run the job we came here to do.
$HOME/CIS520/Proj4/serial_base/execs/linear