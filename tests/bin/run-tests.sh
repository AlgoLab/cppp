#!/bin/bash

regdir="tests/regression"
test -d "${regdir}/output" || mkdir -p "${regdir}/output"
test -d "${regdir}/diffs" || mkdir -p "${regdir}/diffs"
for t in ${regdir}/ok/*
do
    f=$(basename "$t")
    o="${regdir}/output/${f}"
    test -f "$o" || echo "Could not find $o"
    test -f "${regdir}/input/${f}" || echo "Could not find ${regdir}/input/${f}"
    echo "Solving ${regdir}/input/${f}"
    bin/cppp -o "$o" "${regdir}/input/${f}"
    diff -uNaw --strip-trailing-cr --ignore-all-space "${o}" "$t" >  "${regdir}/diffs/${f}"

    # Remove empty diffs
    test -s "${regdir}/diffs/${f}" || rm -f "${regdir}/diffs/${f}"
    bin/cppp-sat -o "$o".sat "${regdir}/input/${f}"
done
touch "${regdir}/diffs"/placeholder
cat "${regdir}/diffs/"*
