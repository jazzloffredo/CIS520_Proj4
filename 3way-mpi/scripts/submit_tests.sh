#!/bin/sh

module load OpenMPI

cd .. && make clean && make all

for j in {1..10}
do
    sbatch --output=Scorecard_PTHREAD_.1.$j._%j.data $HOME/CIS520/Proj4/3way-mpi/scripts/run_single_node.sh
done

for j in {1..10}
do
    sbatch --output=Scorecard_PTHREAD_.2.$j._%j.data $HOME/CIS520/Proj4/3way-mpi/scripts/run_double_node.sh
done

for j in {1..10}
do
    sbatch --output=Scorecard_PTHREAD_.4.$j._%j.data $HOME/CIS520/Proj4/3way-mpi/scripts/run_quad_node.sh
done

for j in {1..10}
do
    sbatch --output=Scorecard_PTHREAD_.8.$j._%j.data $HOME/CIS520/Proj4/3way-mpi/scripts/run_octa_node.sh
done

for j in {1..10}
do
    sbatch --output=Scorecard_PTHREAD_.16.$j._%j.data $HOME/CIS520/Proj4/3way-mpi/scripts/run_sixteen_node.sh
done
