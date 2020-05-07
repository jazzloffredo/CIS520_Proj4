#!/bin/sh

cd .. && make clean && make all

for i in 1 2 4 8 16 32
do
    for j in {1..10}
    do
        sbatch --export=THREADS=$i $HOME/CIS520/Proj4/3way-openmp/scripts/run_single_core.sh
    done
done

for i in 1 2 4 8 16 32
do
    for j in {1..10}
    do
        sbatch --export=THREADS=$i $HOME/CIS520/Proj4/3way-openmp/scripts/run_double_core.sh
    done
done

for i in 1 2 4 8 16 32
do
    for j in {1..10}
    do
        sbatch --export=THREADS=$i $HOME/CIS520/Proj4/3way-openmp/scripts/run_quad_core.sh
    done
done

for i in 1 2 4 8 16 32
do
    for j in {1..10}
    do
        sbatch --export=THREADS=$i $HOME/CIS520/Proj4/3way-openmp/scripts/run_octa_core.sh
    done
done

for i in 1 2 4 8 16 32
do
    for j in {1..10}
    do
        sbatch --export=THREADS=$i $HOME/CIS520/Proj4/3way-openmp/scripts/run_sixteen_core.sh
    done
done

for i in 1 2 4 8 16 32
do
    for j in {1..10}
    do
        sbatch --export=THREADS=$i $HOME/CIS520/Proj4/3way-openmp/scripts/run_thirtytwo_core.sh
    done
done