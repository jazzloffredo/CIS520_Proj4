#!/bin/sh

module load OpenMPI

cd .. && make clean && make all

for j in {1..10}
do
    sbatch --output=Scorecard_PTHREAD_.1.$i._%j.data $HOME/CIS520/Proj4/3way-mpi/scripts/run_single_node.sh
done
