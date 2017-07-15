#!/bin/sh
clang main.c obj.c helpers.c \
  3rdparty/*.c thread_t/threads_posix.c queue_t/queue.c \
  -framework OpenGL -lSDL2 -lpthread -I/usr/local/include -L/usr/local/lib
