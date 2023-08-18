#!/bin/bash

set -e

pushd $(dirname ${BASH_SOURCE[0]})

# cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# gdb --args \
# ./build/loat-static \
#   --mode reachability \
#   --format horn \
#   --proof-level 0 \
#   "../test.smt2"
  # "../chc-comp22-benchmarks/LIA/chc-LIA_349.smt2"
  # --log \

# for file in ../chc-comp22-benchmarks/LIA/*.smt2
# do
#   idx=$(basename $file)
#   idx=${idx#"chc-LIA_"}
#   idx=${idx%".smt2"}
#   # Convert the number string to an integer
#   idx_num=$((10#$idx))
#   if [[ $idx_num -ge 0 ]]; then
#     set +e
#     result=$(timeout 3 ./build/loat-static --mode reachability --format horn --proof-level 0 "$file")
#     exit_status=$?
#     set -e

#     result=$(echo "$result" | head -n 1)

#     if [ $exit_status -eq 0 ]; then
#         printf "$idx $result \n"
#     elif [ $exit_status -eq 124 ]; then
#         printf "$idx timeout \n"
#     else
#         printf "$idx error \n"
#     fi
#   fi
# done

while IFS= read -r line
do
  read idx prev_result <<< "$line"
  file=../chc-comp22-benchmarks/LIA/chc-LIA_$idx.smt2

  if [[ "$prev_result" == "unknown" ]]; then

    set +e
    result=$(timeout 10 ./build/loat-static --mode reachability --format horn --proof-level 0 "$file")
    exit_status=$?
    set -e

    if [ $exit_status -eq 0 ]; then
      result=$(echo "$result" | head -n 1)
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
done < "./test_data_lia.txt"

popd

