#!/usr/bin/env bash

# only print real time
TIMEFORMAT=%R

printf "\nclear cache\n"
sudo purge

printf "\nthreads, cleared cache\n"
time ./fast ubuntu.iso

printf "\nthreads, cached\n"
time ./fast ubuntu.iso
