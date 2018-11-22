#!/bin/bash
#./plotXY.sh $in 2 3

if [ $# -lt 3 ]; then
  echo "Usage: $0 [results] [col1] [col2]"
  exit
fi

f=$1
c1=$2
c2=$3

offset=1

#echo "set key outside left top"
echo "set key off"

echo "set logscale x"
echo "set logscale y"
#echo "set title \"Total Time\""

echo "f(x)=x"
echo "g(x)=10*x"
echo "h(x)=x/10"
echo "timeout=5000"

echo "set datafile separator ';'"
echo "set size square 1,1"

t1=`head -n 1 $f | cut -d";" -f$c1`
t2=`head -n 1 $f | cut -d";" -f$c2`
echo "set xlabel '$t1'"
echo "set ylabel '$t2'"

echo "set xrange [$offset:6000]"
echo "set yrange [$offset:6000]"

echo "set style line 1 lt 1 lw 1 lc rgb '#ff9966'"

echo "plot timeout ls 1 title 'Timeout'"
echo "set arrow from 5000,1 to 5000,6000 nohead ls 1"
echo "replot f(x) lc rgb '#aaddff' lt 1 lw 1 notitle"
echo "replot g(x) lc rgb '#aaddff' lt 0 lw 1 notitle"
echo "replot h(x) lc rgb '#aaddff' lt 0 lw 1 notitle"
#echo "replot \"$f\" u (\$${c1}==0?1:\$$c1):(\$${c2}<${offset}?${offset}:\$$c2) title \"$title\" with points lc rgb '#1d0a42' pt 2"
echo "replot \"$f\" u (strcol(3) eq \"UNKNOWN\")?(\$${c1}<${offset}?${offset}:\$$c1):(1/0):(\$${c2}==0?1:\$$c2) title \"$title\" with points lc rgb '#000000' pt 2"
echo "replot \"$f\" u (strcol(3) eq \"UNSAT\")?(\$${c1}<${offset}?${offset}:\$$c1):(1/0):(\$${c2}==0?1:\$$c2) title \"$title\" with points lc rgb '#1d0a42' pt 6"
echo "replot \"$f\" u (strcol(3) eq \"SAT\")?(\$${c1}<${offset}?${offset}:\$$c1):(1/0):(\$${c2}==0?1:\$$c2) title \"$title\" with points lc rgb '#88226a' pt 2"

echo "set terminal postscript eps enhanced color"
#echo "set term postscript eps enhanced color"
echo "set output '$f-$c1-$c2.eps'"
echo "replot"


