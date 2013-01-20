# Gabriel Grill(1025120)

all: hash

tests: prof cov cycle perf

run: hash
	hash input input2

qcycles: hash
	perf stat -e cycles hash input input2

cycles: hash
	perf stat -r 5 -e cycles hash input input2

prof: hash.c
	gcc -pg -O hash.c -lm -o hash_gprof
	hash_gprof input input2 > /dev/null
	gprof hash_gprof

cov: hash.c
	gcc -O --coverage -lm hash.c -o hash_cov
	hash_cov input input2 > /dev/null
	gcov hash
	cat hash.c.gcov

qperf: hash
	perf stat -e instructions -e branch-misses hash input input2

perf: hash
	perf stat -r 5 -e instructions -e branch-misses hash input input2

hash: hash.c
	gcc -Wall -finline-functions -O3 hash.c -o hash

deploy: clean
	scp Makefile *.c g0:

clean:
	rm -rf hash; rm -rf hash_gprof; rm -rf hash_cov; rm -rf gmon.out; rm -rf hash.c.gcov
	rm -rf hash.gcda; rm -rf hash.gcno; rm -rf hash.stat.h.gcov; rm -f stat.h.gcov

