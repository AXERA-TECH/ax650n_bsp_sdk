#!/bin/bash

LIST=`find .. -name '*.h' | grep custom`
for f in $LIST; do
    echo processing $f
    ./makebin $f;
done
