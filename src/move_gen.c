#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "bit_utils.h"
#include "chess.h"

#define SHIFT(v, s) ((s) < 0 ? (v) >> -(s) : (v) << (s))

static Move *generate_pawn_moves(const struct board *board, Move *moves);
static Move *generate_knight_moves(const struct board *board, Move *moves);

static Move *
generate_pawn_moves(const struct board *board, Move *moves)
{
  Bitboard move_1, move_2;
  int origin, dest;
  /* TODO: check the compiler optomises this function into 2 variations. */
  const int color = board->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  const Bitboard skip_rank        = color ? 0x0000000000ff0000 : 0x0000ff0000000000;
  const Bitboard no_promote_ranks = color ? 0x0000ffffffffffff : 0xffffffffffff0000;
  const Bitboard promote_rank     = color ? 0x00ff000000000000 : 0x000000000000ff00;
  const Bitboard my_pawns         = board->type_bitboards[PIECE_TYPE_PAWN]
    & board->color_bitboards[color];
  const Bitboard their_all        = board->color_bitboards[!color];
  const int      forward          = color ? 8 : -8;
  const Bitboard empty = ~(board->color_bitboards[0] | board->color_bitboards[1]);
  /* regular move 1/2 */
  move_1 = SHIFT(my_pawns & no_promote_ranks, forward) & empty;
  move_2 = SHIFT(move_1 & skip_rank, forward) & empty;
  while (move_1) {
    dest = pop_lss(&move_1);
    origin = dest - forward;
    *moves++ = basic_move(origin, dest);
  }
  while (move_2) {
    dest = pop_lss(&move_2);
    origin = dest - forward * 2;
    *moves++ = basic_move(origin, dest);
  }
  /* regular capture left/right */
  move_1 = SHIFT(my_pawns & no_promote_ranks, forward + 1) & 0xfefefefefefefefe & their_all;
  while (move_1) {
    dest = pop_lss(&move_1);
    origin = dest - (forward + 1);
    *moves++ = basic_move(origin, dest);
  }
  move_1 = SHIFT(my_pawns & no_promote_ranks, forward - 1) & 0x7f7f7f7f7f7f7f7f & their_all;
  while (move_1) {
    dest = pop_lss(&move_1);
    origin = dest - (forward - 1);
    *moves++ = basic_move(origin, dest);
  }
  /* en passant */
  if (board->en_passant_square >= 0) {
    dest = board->en_passant_square;
    move_1
      = ((SHIFT(set_bit(dest), -forward + 1) & 0xfefefefefefefefe)
      | (SHIFT(set_bit(dest), -forward - 1) & 0x7f7f7f7f7f7f7f7f)) & my_pawns;
    while (move_1) {
      origin = pop_lss(&move_1);
      *moves++ = en_passant_move(origin, dest);
    }
  }
  /* promotion */
  move_1 = SHIFT(my_pawns & promote_rank, forward) & empty;
  while (move_1) {
    dest = pop_lss(&move_1);
    origin = dest - forward;
    *moves++ = promote_move(origin, dest, PIECE_TYPE_KNIGHT);
    *moves++ = promote_move(origin, dest, PIECE_TYPE_BISHOP);
    *moves++ = promote_move(origin, dest, PIECE_TYPE_ROOK);
    *moves++ = promote_move(origin, dest, PIECE_TYPE_QUEEN);
  }
  move_1 = SHIFT(my_pawns & promote_rank, forward + 1) & 0xfefefefefefefefe & their_all;
  while (move_1) {
    dest = pop_lss(&move_1);
    origin = dest - (forward + 1);
    *moves++ = promote_move(origin, dest, PIECE_TYPE_KNIGHT);
    *moves++ = promote_move(origin, dest, PIECE_TYPE_BISHOP);
    *moves++ = promote_move(origin, dest, PIECE_TYPE_ROOK);
    *moves++ = promote_move(origin, dest, PIECE_TYPE_QUEEN);
  }
  move_1 = SHIFT(my_pawns & promote_rank, forward - 1) & 0x7f7f7f7f7f7f7f7f & their_all;
  while (move_1) {
    dest = pop_lss(&move_1);
    origin = dest - (forward - 1);
    *moves++ = promote_move(origin, dest, PIECE_TYPE_KNIGHT);
    *moves++ = promote_move(origin, dest, PIECE_TYPE_BISHOP);
    *moves++ = promote_move(origin, dest, PIECE_TYPE_ROOK);
    *moves++ = promote_move(origin, dest, PIECE_TYPE_QUEEN);
  }
  return moves;
}

static Move *
generate_knight_moves(const struct board *board, Move *moves)
{
  uint64_t knights, attacks;
  int origin, dest;
  const int color = board->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  knights = board->type_bitboards[PIECE_TYPE_KNIGHT] & board->color_bitboards[color];
  while (knights) {
    origin = pop_lss(&knights);
    attacks = knight_attack_table[origin] & ~board->color_bitboards[color];
    while (attacks) {
      dest = pop_lss(&attacks);
      *moves++ = basic_move(origin, dest);
    }
  }
  return moves;
}

static Move *
generate_king_moves(const struct board *board, Move *moves)
{
  uint64_t king, attacks;
  int origin, dest;
  const int color = board->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  king = board->type_bitboards[PIECE_TYPE_KING] & board->color_bitboards[color];
  assert(king);
  origin = lss(king);
  attacks = king_attack_table[origin] & ~board->color_bitboards[color];
  while (attacks) {
    dest = pop_lss(&attacks);
    *moves++ = basic_move(origin, dest);
  }
  return moves;
}

static Move *
generate_bishop_moves(const struct board *board, Move *moves)
{
  uint64_t bishops, attacks;
  int origin, dest;
  const int color = board->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  bishops = board->type_bitboards[PIECE_TYPE_BISHOP] & board->color_bitboards[color];
  while (bishops) {
    origin = pop_lss(&bishops);
    attacks
      = get_bishop_attack_set(origin, board->color_bitboards[0] | board->color_bitboards[1])
      & ~board->color_bitboards[color];
    while (attacks) {
      dest = pop_lss(&attacks);
      *moves++ = basic_move(origin, dest);
    }
  }
  return moves;
}

static Move *
generate_rook_moves(const struct board *board, Move *moves)
{
  uint64_t rooks, attacks;
  int origin, dest;
  const int color = board->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  rooks = board->type_bitboards[PIECE_TYPE_ROOK] & board->color_bitboards[color];
  while (rooks) {
    origin = pop_lss(&rooks);
    attacks
      = get_rook_attack_set(origin, board->color_bitboards[0] | board->color_bitboards[1])
      & ~board->color_bitboards[color];
    while (attacks) {
      dest = pop_lss(&attacks);
      *moves++ = basic_move(origin, dest);
    }
  }
  return moves;
}

static Move *
generate_queen_moves(const struct board *board, Move *moves)
{
  uint64_t queens, attacks;
  int origin, dest;
  const int color = board->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  queens = board->type_bitboards[PIECE_TYPE_QUEEN] & board->color_bitboards[color];
  while (queens) {
    origin = pop_lss(&queens);
    attacks
      = (get_rook_attack_set(origin, board->color_bitboards[0] | board->color_bitboards[1])
      | get_bishop_attack_set(origin, board->color_bitboards[0] | board->color_bitboards[1]))
      & ~board->color_bitboards[color];
    while (attacks) {
      dest = pop_lss(&attacks);
      *moves++ = basic_move(origin, dest);
    }
  }
  return moves;
}

static Bitboard
attack_set(const struct board *board, int color)
{
  const int forward = color ? 8 : -8;
  const Bitboard their_all = board->color_bitboards[!color];
  uint64_t pieces, attacks;
  int origin;
  attacks = 0;
  /* pawns */
  pieces = board->type_bitboards[PIECE_TYPE_PAWN] & board->color_bitboards[color];
  attacks |= SHIFT(pieces, forward + 1) & 0xfefefefefefefefe & their_all;
  attacks |= SHIFT(pieces, forward - 1) & 0x7f7f7f7f7f7f7f7f & their_all;
  /* knights */
  pieces = board->type_bitboards[PIECE_TYPE_KNIGHT] & board->color_bitboards[color];
  while (pieces) {
    origin = pop_lss(&pieces);
    attacks |= knight_attack_table[origin] & ~board->color_bitboards[color];
  }
  /* bishop-like */
  pieces
    = (board->type_bitboards[PIECE_TYPE_BISHOP]
      | board->type_bitboards[PIECE_TYPE_QUEEN])
    & board->color_bitboards[color];
  while (pieces) {
    origin = pop_lss(&pieces);
    attacks
      |= get_bishop_attack_set(origin, board->color_bitboards[0] | board->color_bitboards[1])
      & ~board->color_bitboards[color];
  }
  /* rook-like */
  pieces
    = (board->type_bitboards[PIECE_TYPE_ROOK]
      | board->type_bitboards[PIECE_TYPE_QUEEN])
    & board->color_bitboards[color];
  while (pieces) {
    origin = pop_lss(&pieces);
    attacks
      |= get_rook_attack_set(origin, board->color_bitboards[0] | board->color_bitboards[1])
      & ~board->color_bitboards[color];
  }
  /* king */
  pieces = board->type_bitboards[PIECE_TYPE_KING] & board->color_bitboards[color];
  origin = pop_lss(&pieces);
  attacks |= king_attack_table[origin] & ~board->color_bitboards[color];

  return attacks;
}

long
perft(const struct board *board, int depth, int print)
{
  Move moves[256];
  struct board next_board;
  int move_count, c;
  long sum;
  move_count = generate_moves(board, moves) - moves;
  if (depth == 1) {
    if (print) {
      for (int i = 0; i < move_count; i++) {
        print_move(moves[i]);
        printf(": 1\n");
      }
      printf("%d\n", move_count);
    }
    return move_count;
  }
  sum = 0;
  for (int i = 0; i < move_count; i++) {
    next_board = *board;
    make_move(&next_board, moves[i]);
    c = perft(&next_board, depth - 1, 0);
    if (print) {
      print_move(moves[i]);
      printf(": %d\n", c);
    }
    sum += c;
  }
  if (print) printf("%ld\n", sum);
  return sum;
}

Move *
generate_moves(const struct board *board, Move *moves)
{
  const int color = board->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  int king_sq;
  long in_check;
  struct board copy;
  Move *base = moves;
  Bitboard attacks, new_attacks;

  moves = generate_pawn_moves(board, moves);
  moves = generate_knight_moves(board, moves);
  moves = generate_bishop_moves(board, moves);
  moves = generate_rook_moves(board, moves);
  moves = generate_queen_moves(board, moves);
  moves = generate_king_moves(board, moves);
  attacks = attack_set(board, !color); /* Expensive operation, avoid if possible. */
  in_check = board->type_bitboards[PIECE_TYPE_KING] & attacks;
  king_sq = lss(board->type_bitboards[PIECE_TYPE_KING] & board->color_bitboards[color]);
  while (base != moves) {
    if (!in_check && get_move_origin(*base) != king_sq
        && ( set_bit(get_move_origin(*base)) & attacks ) == 0
        && get_move_special_type(*base) != SPECIAL_MOVE_EN_PASSANT) {
      base++;
      continue;
    }
    copy = *board;
    make_move(&copy, *base);
    new_attacks = attack_set(&copy, !color);
    if (new_attacks & copy.type_bitboards[PIECE_TYPE_KING] & copy.color_bitboards[color])
      *base = *(--moves);
    else
      base++;
  }
  return moves;
}
