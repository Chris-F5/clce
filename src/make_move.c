#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include "chess.h"

#include <stdio.h>

void
make_move(struct board *board, Move move)
{
  int capture, promote;
  Bitboard their_pawns;
  const int color = (board->flags & BOARD_FLAG_WHITE_TO_PLAY) ? COLOR_WHITE : COLOR_BLACK;
  const int forward = color ? 8 : -8;
  const int origin = get_move_origin(move);
  const int dest = get_move_dest(move);
  const int piece_type = get_piece_type(board->mailbox, origin);

  /* capture */
  if (board->color_bitboards[!color] & set_bit(dest)) {
    /* clear captured piece */
    capture = get_piece_type(board->mailbox, dest);
    board->type_bitboards[capture] ^= set_bit(dest);
    board->color_bitboards[!color] ^= set_bit(dest);
  }

  /* move piece */
  board->en_passant_square = -1;
  board->type_bitboards[piece_type] ^= set_bit(origin) | set_bit(dest);
  board->color_bitboards[color] ^= set_bit(origin) | set_bit(dest);
  set_piece_type(board->mailbox, dest, piece_type);

  /* pawn specials */
  if (piece_type == PIECE_TYPE_PAWN) {
    if ((get_move_origin(move) ^ get_move_dest(move)) == 16) {
      their_pawns = board->color_bitboards[!color] & board->type_bitboards[PIECE_TYPE_PAWN];
      if ((set_bit(dest) << 1) & 0xfefefefefefefefe & their_pawns
          || (set_bit(dest) >> 1) & 0x7f7f7f7f7f7f7f7f & their_pawns)
        board->en_passant_square = origin + forward;
    } else if (get_move_special_type(move) == SPECIAL_MOVE_PROMOTE) {
      promote = get_move_promote_piece(move);
      set_piece_type(board->mailbox, dest, promote);
      board->type_bitboards[PIECE_TYPE_PAWN] ^= set_bit(dest);
      board->type_bitboards[promote] ^= set_bit(dest);
    } else if (get_move_special_type(move) == SPECIAL_MOVE_EN_PASSANT) {
      board->type_bitboards[PIECE_TYPE_PAWN] ^= set_bit(dest - forward);
      board->color_bitboards[!color] ^= set_bit(dest - forward);
    }
  }

  board->flags ^= BOARD_FLAG_WHITE_TO_PLAY;
}
