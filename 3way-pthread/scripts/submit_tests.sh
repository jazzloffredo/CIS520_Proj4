#!/bin/sh

cd .. && make clean && make all

for i in 1 2 4 8 16 32
do
    for j in {1..10}
    do
        /usr/bin/time -o Scorecard_PTHREAD_.1.$i.$j.out sbatch --export=THREADS=$i --output=Scorecard_PTHREAD_.1.$i._%j.data $HOME/CIS520/Proj4/3way-pthread/scripts/run_single_core.sh
    done
done

for i in 1 2 4 8 16 32
do
    for j in {1..10}
    do
        /usr/bin/time -o Scorecard_PTHREAD_.2.$i.$j.out sbatch --export=THREADS=$i --output=Scorecard_PTHREAD_.2.$i._%j.data $HOME/CIS520/Proj4/3way-pthread/scripts/run_double_core.sh
    done
done

for i in 1 2 4 8 16 32
do
    for j in {1..10}
    do
        /usr/bin/time -o Scorecard_PTHREAD_.4.$i.$j.out sbatch --export=THREADS=$i --output=Scorecard_PTHREAD_.4.$i._%j.data $HOME/CIS520/Proj4/3way-pthread/scripts/run_quad_core.sh
    done
done

for i in 1 2 4 8 16 32
do
    for j in {1..10}
    do
        /usr/bin/time -o Scorecard_PTHREAD_.8.$i.$j.out sbatch --export=THREADS=$i --output=Scorecard_PTHREAD_.8.$i._%j.data $HOME/CIS520/Proj4/3way-pthread/scripts/run_octa_core.sh
    done
done

for i in 1 2 4 8 16 32
do
    for j in {1..10}
    do
        /usr/bin/time -o Scorecard_PTHREAD_.16.$i.$j.out sbatch --export=THREADS=$i --output=Scorecard_PTHREAD_.16.$i._%j.data $HOME/CIS520/Proj4/3way-pthread/scripts/run_sixteen_core.sh
    done
done

for i in 1 2 4 8 16 32
do
    for j in {1..10}
    do
        /usr/bin/time -o Scorecard_PTHREAD_.32.$i.$j.out sbatch --export=THREADS=$i --output=Scorecard_PTHREAD_.32.$i._%j.data $HOME/CIS520/Proj4/3way-pthread/scripts/run_thirtytwo_core.sh
    done
done