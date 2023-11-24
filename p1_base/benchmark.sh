#!/bin/bash

# Loop from 1 to 50
for i in {1..20}
do
    echo "Running test $i"
    (/usr/bin/time -f "Time: %e" ./ems ./jobs $i) 2>> time_results.txt
done

echo "Test results:"
cat time_results.txt