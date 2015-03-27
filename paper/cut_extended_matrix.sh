#!/bin/bash

l=`grep -nam 1  "Matrice estesa" $1 | cut -d : -f 1`
x=$(($l + 1))
w=`tail -n +${x}  $1 | grep -vnaPm1 "^([01]\s*)+$" | cut -d : -f 1`
z=$(($w - 1))
tail -n +${x}  $1 | head -n ${z} | perl -e 's/\h//g' -p
