#!/bin/bash -e

tests/internal/perfect_phylogeny.o
for t in tests/api/t*.json
do
    tests/internal/perfect_phylogeny.o "$t"
done
