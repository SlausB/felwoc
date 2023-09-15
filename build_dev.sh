#!/bin/bash
clear
reset

set -e

bazel build -c dbg xls2xj
mv -f ./bazel-bin/xls2xj ./

rm -rf out

#gdb -ex run ./xls2xj
./xls2xj
