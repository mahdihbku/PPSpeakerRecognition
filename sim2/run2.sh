#!/bin/bash
gcc -O3 sim2.c -lcrypto -pthread -lm

for m in 10 20 50 100 200 500 1000
do
	for i in {1..4}
	do
		./a.out $m 4
	done
done

datamash --group 2 mean 4 mean 6 <sim2_results.txt > out.csv
