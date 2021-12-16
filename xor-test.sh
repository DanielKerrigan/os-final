#!/usr/bin/env bash

./run ubuntu.iso -r 512 5505348
./run ubuntu.iso -r 1024 2752674
./run ubuntu.iso -r 2048 1376337

./fast ubuntu.iso

./run ubuntu.iso -f 1024 4
./run ubuntu.iso -f 2048 16
./run ubuntu.iso -f 1048576 32