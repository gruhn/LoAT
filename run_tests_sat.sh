#!/bin/bash
repo=../chc-comp22-benchmarks
while read line; do
    if [ "$line" = "unsat" ] || [ "$line" = "sat" ] || [ "$line" == "unknown" ]; then
        # line contains the expected result for the following benchmarks
        expected=$line
    else
        if [[ $line =~ ^#.*$ ]] || [ "$line" = "" ] ; then
            # skip comments and empty lines
            continue
        else
            if [[ $line =~ ^[0-9]+$ ]]; then
                # line contains the number of the next benchmark -- run it
                ex=$line
                echo "running chc-${path}_$ex.smt2"
                start=`date +%s`
                actual=$(./build/static/release/loat-static --mode satisfiability --format horn --proof-level 0 --timeout 10 "$repo/$path/chc-${path}_$ex.smt2" | head -n 1)
                end=`date +%s`
                runtime=$((end-start))
                if [ "$expected" = "$actual" ]; then
                    echo "test passed in ${runtime}s"
                else
                    echo "test failed after ${runtime}s -- expected $expected, got $actual"
                    # exit 0
                fi
            else
                if [ "$line" = "LIA-Lin" ]; then
                    # line contains the folder of the following benchmarks
                    path=$line
                else
                    echo "failed to parse $line"
                    exit -1;
                fi
            fi
        fi
    fi
done < test_data_sat.txt
