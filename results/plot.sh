#!/bin/bash
#./plotXY.sh $in 2 3

if [ $# -lt 3 ]; then
  echo "Usage: $0 [results] [col1] [col2]"
  exit
fi

f=$1
c1=$2
c2=$3

title="Total Time"

echo "set key outside left top"
#echo "set key off"

echo "set logscale x"
echo "set logscale y"
echo "set title \"$title\""

echo "f(x)=x"
echo "timeout=5000"

echo "set datafile separator ';'"
echo "set size square 1,1"

echo "set xlabel '$c1'"
echo "set ylabel '$c2'"

echo "set xrange [1:6000]"
echo "set yrange [1:6000]"

echo "set style line 1 lt 1 lw 1 lc 26" #rgb 'red'"
echo "set style line 2 lt 1 lw 1 lc 27" #rgb 'green'"

echo "plot timeout ls 1 title 'Timeout'"
echo "set arrow from 5000,1 to 5000,6000 nohead ls 1"
echo "replot f(x) ls 2 notitle"
echo "replot \"$f\" u (\$${c1}==0?1:\$$c1):(\$${c2}==0?1:\$$c2) title \"$title\" with points lc 1"

echo "set term postscript eps enhanced color"
echo "set output 'result.pdf'"
echo "replot"


