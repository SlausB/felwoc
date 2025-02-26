#!/bin/bash
clear
reset

set -e

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build

rm -rf ./build/out

#gdb -ex run ./build/xls2xj
./build/xls2xj


