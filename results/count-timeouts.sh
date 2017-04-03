awk -F";" '{for(i=3;i<=NF;i++){if(NR==1)h[i]=$i;else if($i==5000)x[i]++}}END{for(i in x){print h[i]": "x[i]}}' $1
