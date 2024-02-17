#!/bin/bash
set -e

CFLAGS="-Wall"

if echo "$1" | grep -q "o"; then
    CFLAGS="$CFLAGS -O2"
fi
if echo "$1" | grep -q "d"; then
    CFLAGS="$CFLAGS -g -pg"
fi

set -x
rm -rf obj
rm -f clce
mkdir obj
gcc src/main.c -o obj/main.o -c $CFLAGS
gcc src/tests.c -o obj/tests.o -c $CFLAGS
gcc src/bitboards.c -o obj/bitboards.o -c $CFLAGS
gcc src/fen.c -o obj/fen.o -c $CFLAGS
gcc src/make_move.c -o obj/make_move.o -c $CFLAGS
gcc src/move_gen.c -o obj/move_gen.o -c $CFLAGS
gcc src/evaluate.c -o obj/evaluate.o -c $CFLAGS
gcc src/find_move.c -o obj/find_move.o -c $CFLAGS
gcc src/utils.c -o obj/utils.o -c $CFLAGS
gcc src/magic_numbers.c -o obj/magic_numbers.o -c $CFLAGS
gcc src/zobrist_numbers.c -o obj/zobrist_numbers.o -c $CFLAGS
gcc obj/*.o -o clce -pg
