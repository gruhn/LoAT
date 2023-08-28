#!/bin/bash

set -e

pushd $(dirname ${BASH_SOURCE[0]})

cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
# cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# # # gdb --args \
# ./build/loat-static \
#   --mode reachability \
#   --format horn \
#   --proof-level 0 \
#   --log \
#   "../chc-comp22-benchmarks/LIA/chc-LIA_000.smt2"

# popd
# exit

benchmark="LIA"
# benchmark="LIA"
tool="z3"
# tool="adcl"

while IFS= read -r line
do
  read idx prev_result <<< "$line"
  file="../chc-comp22-benchmarks/${benchmark}/chc-${benchmark}_${idx}.smt2"

  if [[ "$prev_result" == "unsat" ]]; then

    set +e
    result=$(timeout 10 ./build/loat-static --mode reachability --format horn --proof-level 0 "$file")
    # result=$(timeout 3 z3 "$file")
    exit_status=$?
    set -e

    if [ $exit_status -eq 0 ]; then
      result=$(echo "$result" | head -n 1)
      # result="$result"
    elif [ $exit_status -eq 124 ]; then
      result="timeout"
    else
      result="error"
    fi

    if [[ "$result" == "$prev_result" ]]; then
      printf "$idx $result \n"
    else
      printf "$idx $prev_result --> $result \n"
    fi

  fi
done < "./benchmarks/${benchmark}_${tool}.txt"

popd
