#!/bin/bash
clear
reset

set -e

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
