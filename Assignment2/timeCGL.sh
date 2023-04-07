#!/bin/bash

gcc ConwaysGameOfLife.c -o game_of_life

iterations=10
grid_size=1000

for PROCESSES in 1 2 4 8; do
  TIME=$(time ./game_of_life $GRID_SIZE $ITERATIONS $PROCESSES 2>&1 >/dev/null)
  echo "$PROCESSES,$TIME" >> times.csv
done

gnuplot << EOF
set terminal pngcairo enhanced font 'Verdana,10'
set output 'speedup.png'
set xlabel 'Number of Processes'
set ylabel 'Execution Time (seconds)'
set title 'Game of Life Speedup'
plot 'times.csv' with linespoints title 'Execution Time'
EOF
