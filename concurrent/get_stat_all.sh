#!/bin/bash

dir0=1M4t_slb0
dir1=1M4t_slb32
dir2=1M4t_slb0_bf
dir3=1M4t_slb32_bf

btrees=(btree_concurrent)
wlats=(0 200 400 600 800 1000)

for bt in ${btrees[@]}
do
	echo "test done, process performance data ... begin!"
	outf="$bt"_perf_stat_all.csv
	echo "$outf"
	ds0="$bt"_"$dir0"
	ds1="$bt"_"$dir1"
	ds2="$bt"_"$dir2"
	ds3="$bt"_"$dir3"
	ds=($ds0 $ds1 $ds2 $ds3)

	echo "average latency(ns) of insert, read and delete operations" > $outf
	for dir in ${ds[@]}
	do
		echo "$dir, w_lat, insert_lat, search_lat, delete_lat" >> $outf
		for lat in ${wlats[@]}
		do
			log=btree_perf_"$lat"ns.log
			insert_lat=$(grep inserting $dir/$log |cut -d":" -f3|awk '{sum += $1} END {print sum/NR}')
			search_lat=$(grep searching $dir/$log |cut -d":" -f3|awk '{sum += $1} END {print sum/NR}')
			delete_lat=$(grep deleting $dir/$log |cut -d":" -f3|awk '{sum += $1} END {print sum/NR}')
			echo ", $lat, $insert_lat, $search_lat, $delete_lat" >> $outf
		done
		echo "" >> $outf
	done
	echo "test done, process performance data ... done!"
done
