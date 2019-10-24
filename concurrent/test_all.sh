#!/bin/bash

dir0=1M4t_slb0
dir1=1M4t_slb32
dir2=1M4t_slb0_bf
dir3=1M4t_slb32_bf

btrees=(btree_concurrent)
num=1000000
round=5
wlats=(0 200 400 600 800 1000)

for bt in ${btrees[@]}
do
	ds0="$bt"_"$dir0"
	ds1="$bt"_"$dir1"
	ds2="$bt"_"$dir2"
	ds3="$bt"_"$dir3"
	ds=($ds0 $ds1 $ds2 $ds3)
	mkdir $ds0 
	mkdir $ds1 
	mkdir $ds2 
	mkdir $ds3 

	echo "test btree with $num kvs ..."
	echo "test btree without SLB ..."
	for lat in ${wlats[@]}
	do
		echo "test btree with $lat ns additional write latency ..."
		for i in $(seq 1 $round)
		do
			sudo ./$bt -n $num -w $lat -c 0 -t 4 -i 100_million_normal.txt >>$ds0/btree_perf_"$lat"ns.log
			sleep 5
		done
		echo 3 >/proc/sys/vm/drop_caches
	done

	echo "test btree with 32MB SLB ..."
	for lat in ${wlats[@]}
	do
		echo "test btree with $lat ns additional write latency ..."
		for i in $(seq 1 $round)
		do
			sudo ./$bt -n $num -w $lat -c 32 -t 4 -i 100_million_normal.txt >>$ds1/btree_perf_"$lat"ns.log
			sleep 5
		done
		echo 3 >/proc/sys/vm/drop_caches
	done

	echo "test btree without SLB but with BloomFilter ..."
	for lat in ${wlats[@]}
	do
		echo "test btree with $lat ns additional write latency ..."
		for i in $(seq 1 $round)
		do
			sudo ./$bt -n $num -w $lat -c 0 -b -t 4 -i 100_million_normal.txt >>$ds2/btree_perf_"$lat"ns.log
			sleep 5
		done
		echo 3 >/proc/sys/vm/drop_caches
	done

	echo "test btree with 32MB SLB and BloomFilter ..."
	for lat in ${wlats[@]}
	do
		echo "test btree with $lat ns additional write latency ..."
		for i in $(seq 1 $round)
		do
			sudo ./$bt -n $num -w $lat -c 32 -b -t 4 -i 100_million_normal.txt >>$ds3/btree_perf_"$lat"ns.log
			sleep 5
		done
		echo 3 >/proc/sys/vm/drop_caches
	done
done
