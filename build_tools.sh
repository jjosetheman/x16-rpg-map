#!/bin/bash

gcc -o asc2bin.exe asc2bin.c
gcc -o vgm2x16opm.exe vgm2x16opm.c
gcc -o make4bitbin.exe make4bitbin.c
gcc -o pal12bit.exe pal12bit.c
gcc -o retile.exe retile.c
gcc -o maketilemap.exe -I/usr/include/libxml2  maketilemap.c -ljson-c -lxml2
