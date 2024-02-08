#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>

#include "chess.h"

static void count_pieces(Bitboard bitboard, int value, int *count);
static int material_count(const struct board *board);

static void
count_pieces(Bitboard bitboard, int value, int *count)
{
  while(bitboard) {
    pop_lss(&bitboard);
    *count += value;
  }
}

static int
material_count(const struct board *board)
{
  int material;
  Bitboard bitboard;

  material = 0;
  bitboard = board->type_bitboards[PIECE_TYPE_PAWN];
  count_pieces(bitboard & board->color_bitboards[COLOR_WHITE], 1, &material);
  count_pieces(bitboard & board->color_bitboards[COLOR_BLACK], -1, &material);
  bitboard
    = board->type_bitboards[PIECE_TYPE_BISHOP]
    | board->type_bitboards[PIECE_TYPE_KNIGHT];
  count_pieces(bitboard & board->color_bitboards[COLOR_WHITE], 3, &material);
  count_pieces(bitboard & board->color_bitboards[COLOR_BLACK], -3, &material);
  bitboard = board->type_bitboards[PIECE_TYPE_ROOK];
  count_pieces(bitboard & board->color_bitboards[COLOR_WHITE], 5, &material);
  count_pieces(bitboard & board->color_bitboards[COLOR_BLACK], -5, &material);
  bitboard = board->type_bitboards[PIECE_TYPE_QUEEN];
  count_pieces(bitboard & board->color_bitboards[COLOR_WHITE], 9, &material);
  count_pieces(bitboard & board->color_bitboards[COLOR_BLACK], -9, &material);
  return material;
}

int
evaluate_board(const struct board *board)
{
  return material_count(board);
}
