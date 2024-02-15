#!/bin/sh

rm -rf obj
rm -f clce
mkdir obj

CFLAGS="-O2 -g -pg -Wall"
gcc src/main.c -o obj/main.o -c $CFLAGS
gcc src/tests.c -o obj/tests.o -c $CFLAGS
gcc src/bitboards.c -o obj/bitboards.o -c $CFLAGS
gcc src/fen.c -o obj/fen.o -c $CFLAGS
gcc src/make_move.c -o obj/make_move.o -c $CFLAGS
gcc src/move_gen.c -o obj/move_gen.o -c $CFLAGS
gcc src/evaluate.c -o obj/evaluate.o -c $CFLAGS
gcc src/find_move.c -o obj/find_move.o -c $CFLAGS
gcc src/utils.c -o obj/utils.o -c $CFLAGS
gcc obj/*.o -o clce -pg
