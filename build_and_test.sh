#!/bin/bash
clear
reset

set -e

./build_dev.sh
./test.sh
