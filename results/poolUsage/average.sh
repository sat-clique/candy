if [ $# -lt 3 ]; then
  echo "Usage: $0 file column-from column-to"
fi

file=$1
from=$2
to=$3

for i in `seq $from $to`; do
  cat $file | awk -F";" -v col=$i '{ sum += $col } END { if (NR > 0) print sum / NR }'
done
