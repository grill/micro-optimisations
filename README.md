micro-optimisations
===================

A collection of micro optimisations to improve a programm that does hash table lookups.

Performance testing:
--------------------

  gcc -Wall -O3 hash.c -o hash
  perf stat -r 5 -e instructions -e branch-misses hash input input2
  perf stat -r 5 -e cycles hash input input2

[Presentation](http://www.slideshare.net/ggrill/efficient-code)
