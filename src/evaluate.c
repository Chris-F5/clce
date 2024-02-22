#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>

#include "chess.h"

static int
material_count(struct board *board)
{
  struct position *pos;
  Bitboard bitboard;
  int material;
  pos = board_position(board);

  material = 0;
  bitboard = pos->type_bitboards[PIECE_TYPE_PAWN];
  material += count_bits(bitboard & pos->color_bitboards[COLOR_WHITE]) * 1;
  material += count_bits(bitboard & pos->color_bitboards[COLOR_BLACK]) * -1;
  bitboard
    = pos->type_bitboards[PIECE_TYPE_BISHOP]
    | pos->type_bitboards[PIECE_TYPE_KNIGHT];
  material += count_bits(bitboard & pos->color_bitboards[COLOR_WHITE]) * 3;
  material += count_bits(bitboard & pos->color_bitboards[COLOR_BLACK]) * -3;
  bitboard = pos->type_bitboards[PIECE_TYPE_ROOK];
  material += count_bits(bitboard & pos->color_bitboards[COLOR_WHITE]) * 5;
  material += count_bits(bitboard & pos->color_bitboards[COLOR_BLACK]) * -5;
  bitboard = pos->type_bitboards[PIECE_TYPE_QUEEN];
  material += count_bits(bitboard & pos->color_bitboards[COLOR_WHITE]) * 9;
  material += count_bits(bitboard & pos->color_bitboards[COLOR_BLACK]) * -9;
  return material;
}

int
evaluate_board(struct board *board)
{
  return material_count(board);
}
