#!/bin/bash

CFLAGS=""
LDFLAGS=""

set -e
mkdir -p ./build

for arg in $@
do
    echo "[ gcc $arg ]"
    gcc -c "./src/$arg" -o "./build/$arg.o"
done

ld 
