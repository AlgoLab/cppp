#!/bin/bash

ack-grep -l non-zero exp/ | xargs rm -f
echo "Type, n, m, id, time" > results.csv
for f in `find exp -name '*.log'`
do
    echo -n `echo "$f" | perl -e 's/\/|\.log/,/g' -p` >> results.csv
    cat "$f" >> results.csv
done
