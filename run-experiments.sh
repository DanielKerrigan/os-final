#!/usr/bin/env bash
echo "running experiments with file" $1

echo "running sys_calls"
./benchmark --sys_calls $1 | tee results/sys_calls.csv

echo "running perf_cache"
./benchmark --perf_cache $1 | tee results/perf_cache.csv

echo "running perf_no_cache"
./benchmark --perf_no_cache $1 | tee results/perf_no_cache.csv

echo "running perf_cache_threads"
./benchmark --perf_cache_threads $1 | tee results/perf_cache_threads.csv

echo "running perf_no_cache_threads"
./benchmark --perf_no_cache_threads $1 | tee results/perf_no_cache_threads.csv