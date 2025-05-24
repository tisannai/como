#!/bin/sh

mkdir -p build
gcc -Wall -fPIC -O2 -c -o build/como.o src/como.c
gcc -shared -o build/libcomo.so build/como.o -l plinth
# a2x -v --doctype manpage --format manpage man/como.3.txt
