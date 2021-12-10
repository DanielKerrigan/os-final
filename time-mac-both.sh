#!/usr/bin/env bash

# only print real time
TIMEFORMAT=%R

printf "clear cache\n"
sudo purge

printf "\nnormal, cleared cache\n"
time ./run ubuntu.iso -r 1024 2752674

printf "\nnormal, cached\n"
time ./run ubuntu.iso -r 1024 2752674

printf "\nclear cache\n"
sudo purge

printf "\nthreads, cleared cache\n"
time ./fast ubuntu.iso

printf "\nthreads, cached\n"
time ./fast ubuntu.iso