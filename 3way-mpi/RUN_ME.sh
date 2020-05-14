#!/bin/sh

module load OpenMPI

make clean && make all

mpirun mpi
