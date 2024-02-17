#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include "chess.h"

#include <stdio.h>

void
make_move(struct search_state *state, Move move)
{
  int capture, promote;
  Bitboard their_pawns;
  state->board_states[state->search_ply + 1] = state->board_states[state->search_ply];
  struct board_state *bstate = &state->board_states[++state->search_ply];
  const int color = (bstate->flags & BOARD_FLAG_WHITE_TO_PLAY) ? COLOR_WHITE : COLOR_BLACK;
  const int forward = color ? 8 : -8;
  const int origin = get_move_origin(move);
  const int dest = get_move_dest(move);
  const int piece_type = get_piece_type(bstate->mailbox, origin);
  int castle_origin, castle_dest;

  /* capture */
  if (bstate->color_bitboards[!color] & set_bit(dest)) {
    /* clear captured piece */
    capture = get_piece_type(bstate->mailbox, dest);
    bstate->type_bitboards[capture] ^= set_bit(dest);
    bstate->color_bitboards[!color] ^= set_bit(dest);
    if (capture == PIECE_TYPE_PAWN)
      bstate->pawn_hash
        ^= get_zobrist_piece_number(!color, PIECE_TYPE_PAWN, dest);
    else
      bstate->non_pawn_hash
        ^= get_zobrist_piece_number(!color, capture, dest);
  }

  /* move piece */
  bstate->en_passant_square = -1;
  bstate->type_bitboards[piece_type] ^= set_bit(origin) | set_bit(dest);
  bstate->color_bitboards[color] ^= set_bit(origin) | set_bit(dest);
  set_piece_type(bstate->mailbox, dest, piece_type);
  if (piece_type == PIECE_TYPE_PAWN) {
    bstate->pawn_hash ^= get_zobrist_piece_number(color, PIECE_TYPE_PAWN, origin);
    bstate->pawn_hash ^= get_zobrist_piece_number(color, PIECE_TYPE_PAWN, dest);
  } else {
    bstate->non_pawn_hash ^= get_zobrist_piece_number(color, piece_type, origin);
    bstate->non_pawn_hash ^= get_zobrist_piece_number(color, piece_type, dest);
  }

  /* pawn specials */
  if (piece_type == PIECE_TYPE_PAWN) {
    if ((origin ^ dest) == 16) { /* move two */
      their_pawns = bstate->color_bitboards[!color] & bstate->type_bitboards[PIECE_TYPE_PAWN];
      if ((set_bit(dest) << 1) & 0xfefefefefefefefe & their_pawns
          || (set_bit(dest) >> 1) & 0x7f7f7f7f7f7f7f7f & their_pawns)
        bstate->en_passant_square = origin + forward;
    } else if (get_move_special_type(move) == SPECIAL_MOVE_PROMOTE) {
      promote = get_move_promote_piece(move);
      set_piece_type(bstate->mailbox, dest, promote);
      bstate->type_bitboards[PIECE_TYPE_PAWN] ^= set_bit(dest);
      bstate->pawn_hash ^= get_zobrist_piece_number(color, PIECE_TYPE_PAWN, dest);
      bstate->type_bitboards[promote] ^= set_bit(dest);
      bstate->non_pawn_hash ^= get_zobrist_piece_number(color, promote, dest);
    } else if (get_move_special_type(move) == SPECIAL_MOVE_EN_PASSANT) {
      bstate->type_bitboards[PIECE_TYPE_PAWN] ^= set_bit(dest - forward);
      bstate->color_bitboards[!color] ^= set_bit(dest - forward);
      bstate->pawn_hash ^= get_zobrist_piece_number(!color, PIECE_TYPE_PAWN, dest - forward);
    }
  }

  if (piece_type == PIECE_TYPE_ROOK) {
    bstate->non_pawn_hash ^= (bstate->flags >> 1) & 0x0f;
    switch(get_move_origin(move)) {
    case 0:
      bstate->flags |= BOARD_FLAG_WHITE_CASTLE_QUEEN;
      break;
    case 7:
      bstate->flags |= BOARD_FLAG_WHITE_CASTLE_KING;
      break;
    case 56:
      bstate->flags |= BOARD_FLAG_BLACK_CASTLE_QUEEN;
      break;
    case 63:
      bstate->flags |= BOARD_FLAG_BLACK_CASTLE_KING;
      break;
    }
    bstate->non_pawn_hash ^= (bstate->flags >> 1) & 0x0f;
  }

  /* castling */
  if (get_move_special_type(move) == SPECIAL_MOVE_CASTLING) {
    bstate->non_pawn_hash ^= (bstate->flags >> 1) & 0x0f;
    bstate->flags |= color
      ? BOARD_FLAG_WHITE_CASTLE_KING | BOARD_FLAG_WHITE_CASTLE_QUEEN
      : BOARD_FLAG_BLACK_CASTLE_KING | BOARD_FLAG_BLACK_CASTLE_QUEEN;
    bstate->non_pawn_hash ^= (bstate->flags >> 1) & 0x0f;
    switch(dest) {
    case 2:
      castle_origin = 0;
      castle_dest = 3;
      break;
    case 6:
      castle_origin = 7;
      castle_dest = 5;
      break;
    case 58:
      castle_origin = 56;
      castle_dest = 59;
      break;
    case 62:
      castle_origin = 63;
      castle_dest = 61;
      break;
    default:
      goto no_castle;
    }
    set_piece_type(bstate->mailbox, castle_dest, PIECE_TYPE_ROOK);
    bstate->color_bitboards[color]          ^= set_bit(castle_origin) | set_bit(castle_dest);
    bstate->type_bitboards[PIECE_TYPE_ROOK] ^= set_bit(castle_origin) | set_bit(castle_dest);
    bstate->non_pawn_hash ^= get_zobrist_piece_number(color, PIECE_TYPE_ROOK, castle_origin);
    bstate->non_pawn_hash ^= get_zobrist_piece_number(color, PIECE_TYPE_ROOK, castle_dest);
  }
no_castle:

  bstate->flags ^= BOARD_FLAG_WHITE_TO_PLAY;
  bstate->non_pawn_hash ^= zobrist_black_number;
  state->fullmove_clock++;
  /* TODO: halfmove clocks. */
}

void
unmake_move(struct search_state *state, Move move)
{
  assert(state->search_ply > 0);
  assert(state->fullmove_clock > 0);
  state->search_ply--;
  state->fullmove_clock--;
}
