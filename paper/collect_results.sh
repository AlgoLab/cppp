#!/bin/bash

for f in `ack-grep -l non-zero exp/` `ack-grep -l terminated exp/`
do
    echo "900" > "$f"
done
echo "Type, n, m, id, time" > results.csv
for f in `find exp -name '*.log'`
do
    echo -n `echo "$f" | perl -e 's/\/|\.log/,/g' -p` >> results.csv
    cat "$f" >> results.csv
done

# The same for modified matrices
for f in `ack-grep -l non-zero mod/` `ack-grep -l terminated mod/`
do
    echo "900" > "$f"
done

echo "Type, n, m, id, nummod, id2, time" > modified.csv
for f in `find mod -name '*.log'`
do
    echo -n `echo "$f" | perl -e 's/\/|\.log/,/g' -p` >> modified.csv
    cat "$f" >> modified.csv
done

cp results.csv modified.csv ~/temp/tmp/
