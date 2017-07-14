#!/bin/sh
clang *.c 3rdparty/*.c -framework OpenGL -l SDL2 -I/usr/local/include -L/usr/local/lib
