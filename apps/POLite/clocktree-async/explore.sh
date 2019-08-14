#!/bin/bash

function size {
  D=$1
  B=$2

  N=0
  for I in $(seq 0 $D); do
    N=$(($N + $B ** $I))
  done

  echo $N
}

CUTOFF=25000000
R="1 2 3 4 5 6 7 8 9 10 12 14 16"
for B in $R; do
  for D in $R; do
    N=$(size $D $B)
    if (( $N < $CUTOFF )); then
      echo $D $B $N
    fi
  done
  echo
done