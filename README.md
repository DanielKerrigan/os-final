# CD-GY 6223 Fall 2021 Operating Systems - Final Project

Racquel Fygenson (rlf9859)

Daniel Kerrigan (djk525)

## Todo

- Make benchmark more efficient by minimizing the number of times we call `callWrite`.
- Make sure `./fast` doesn't break on small files.
- Find correct block size and number of threads for `./fast`.
- Measure other system calls without repeatedly calling `gettimeofday`, since that is also a system call.
- Caching.
- Make visualizations.
- Make report.

## Questions

- Currently we are measuring how long it takes to `exec` our `run` program. Is this approach okay or should we only be measuring the time that our run program spends reading?
  - Is measuring how long it takes to `exec` the `run` program comparable to directly calling other system calls, like `lseek`?
- Are the measurements in part 3 with or without caching?
- For part 5, why are we reporting both MB/s and B/s?
- For the other system calls in part 5, are we reporting the number of system calls that we can make per second?