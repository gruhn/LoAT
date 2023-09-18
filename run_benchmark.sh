#!/bin/bash

set -e

# Switch working directory to the folder, where this script is laying around.
# That way all paths are guaranteed to be relative to the script locaiton.
pushd $(dirname ${BASH_SOURCE[0]})

cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
# cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# # gdb --args \
# ./build/loat-static \
#   --mode reachability \
#   --format horn \
#   --proof-level 0 \
#   "../chc-comp22-benchmarks/LIA-Lin/chc-LIA-Lin_064.smt2"
#   # "../chc-comp22-benchmarks/LIA/chc-LIA_063.smt2"
#   # --log \

# popd
# exit

benchmark="LIA-Lin"
# benchmark="LIA"
# compare_with="z3"
compare_with="adcl"

while IFS= read -r line
do
  if [[ $line =~ ^#.*$ ]] || [ "$line" = "" ] ; then
    # skip comments and empty lines
    continue
  else
    read idx prev_result <<< "$line"
    file="../chc-comp22-benchmarks/${benchmark}/chc-${benchmark}_${idx}.smt2"

    # if [[ "$prev_result" == "unsat" ]]; then

      set +e
      result=$(timeout 3 ./build/loat-static --mode reachability --format horn --proof-level 0 "$file")
      # result=$(timeout 10 z3 "$file")
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

    # fi
  fi
done < "./benchmarks/${benchmark}_${compare_with}.txt"

# "undo" pushd
popd
