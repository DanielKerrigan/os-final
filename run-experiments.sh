#!/usr/bin/env bash
echo "running perf_cache"
./benchmark --perf_cache $1 > results/perf_cache.csv
echo "running perf_no_cache"
./benchmark --perf_no_cache $1 > results/perf_no_cache.csv
echo "running perf_cache_threads"
./benchmark --perf_cache_threads $1 > results/perf_cache_threads.csv
echo "running perf_no_cache_threads"
./benchmark --perf_no_cache_threads $1 > results/perf_no_cache_threads.csv
echo "running sys_calls"
./benchmark --sys_calls $1 > results/sys_calls.csv