#!/bin/bash
impleTypes=( 0 1 2 3 )
threadNums=( 4 32 256 1024 )
dataSizes=( 100 10000 1000000 )

for t in "${impleTypes[@]}"
do
    for i in "${threadNums[@]}"
    do
	for j in "${dataSizes[@]}"
	do
		echo "Execute: ./benchmark.exe -n $i -m $j -t $t";
                ./benchmark.exe -n $i -m $j -t $t;
	done
    done
done
