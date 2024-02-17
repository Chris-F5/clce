#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "chess.h"

#define SHIFT(v, s) ((s) < 0 ? (v) >> -(s) : (v) << (s))

static Move *generate_pawn_moves(const struct search_state *state, Move *moves);
static Move *generate_knight_moves(const struct search_state *state, Move *moves);
static Move * generate_king_moves(const struct search_state *state, Move *moves,
    uint64_t opponent_attack_set);
static Move * generate_bishop_moves(const struct search_state *state, Move *moves);
static Move * generate_rook_moves(const struct search_state *state, Move *moves);
static Move * generate_queen_moves(const struct search_state *state, Move *moves);
static Bitboard attack_set(const struct search_state *state, int color);

static Move *
generate_pawn_moves(const struct search_state *state, Move *moves)
{
  Bitboard move_1, move_2;
  int origin, dest;
  const struct board_state *bstate = &state->board_states[state->search_ply];
  /* TODO: check the compiler optomises this function into 2 variations. */
  const int color = bstate->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  const Bitboard skip_rank        = color ? 0x0000000000ff0000 : 0x0000ff0000000000;
  const Bitboard no_promote_ranks = color ? 0x0000ffffffffffff : 0xffffffffffff0000;
  const Bitboard promote_rank     = color ? 0x00ff000000000000 : 0x000000000000ff00;
  const Bitboard my_pawns         = bstate->type_bitboards[PIECE_TYPE_PAWN]
    & bstate->color_bitboards[color];
  const Bitboard their_all        = bstate->color_bitboards[!color];
  const int      forward          = color ? 8 : -8;
  const Bitboard empty = ~(bstate->color_bitboards[0] | bstate->color_bitboards[1]);
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
  if (bstate->en_passant_square >= 0) {
    dest = bstate->en_passant_square;
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
generate_knight_moves(const struct search_state *state, Move *moves)
{
  uint64_t knights, attacks;
  int origin, dest;
  const struct board_state *bstate = &state->board_states[state->search_ply];
  const int color = bstate->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  knights = bstate->type_bitboards[PIECE_TYPE_KNIGHT] & bstate->color_bitboards[color];
  while (knights) {
    origin = pop_lss(&knights);
    attacks = knight_attack_table[origin] & ~bstate->color_bitboards[color];
    while (attacks) {
      dest = pop_lss(&attacks);
      *moves++ = basic_move(origin, dest);
    }
  }
  return moves;
}

static Move *
generate_king_moves(const struct search_state *state, Move *moves,
    uint64_t opponent_attack_set)
{
  uint64_t king, attacks;
  int origin, dest;
  const struct board_state *bstate = &state->board_states[state->search_ply];
  const int color = bstate->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  king = bstate->type_bitboards[PIECE_TYPE_KING] & bstate->color_bitboards[color];
  assert(king);
  origin = lss(king);
  attacks = king_attack_table[origin] & ~bstate->color_bitboards[color];
  while (attacks) {
    dest = pop_lss(&attacks);
    *moves++ = basic_move(origin, dest);
  }
  /* castling */
  if ((opponent_attack_set & (color ? set_bit(4) : set_bit(60))) == 0 &&
      ~bstate->flags & (color ? BOARD_FLAGS_WHITE_CASTLE : BOARD_FLAGS_BLACK_CASTLE)) {
    attacks = opponent_attack_set | bstate->color_bitboards[0] | bstate->color_bitboards[1];
    if ( (bstate->flags & (color ? BOARD_FLAG_WHITE_CASTLE_KING : BOARD_FLAG_BLACK_CASTLE_KING)) == 0
        && (attacks & (color ? WHITE_KING_CASTLE_BLOCKERS : BLACK_KING_CASTLE_BLOCKERS)) == 0)
      *moves++ = castle_move(color, 1);
    if ( (bstate->flags & (color ? BOARD_FLAG_WHITE_CASTLE_QUEEN : BOARD_FLAG_BLACK_CASTLE_QUEEN)) == 0
        && (attacks & (color ? WHITE_QUEEN_CASTLE_BLOCKERS : BLACK_QUEEN_CASTLE_BLOCKERS)) == 0)
      *moves++ = castle_move(color, 0);
  }
  return moves;
}

static Move *
generate_bishop_moves(const struct search_state *state, Move *moves)
{
  uint64_t bishops, attacks;
  int origin, dest;
  const struct board_state *bstate = &state->board_states[state->search_ply];
  const int color = bstate->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  bishops = bstate->type_bitboards[PIECE_TYPE_BISHOP] & bstate->color_bitboards[color];
  while (bishops) {
    origin = pop_lss(&bishops);
    attacks
      = get_bishop_attack_set(origin, bstate->color_bitboards[0] | bstate->color_bitboards[1])
      & ~bstate->color_bitboards[color];
    while (attacks) {
      dest = pop_lss(&attacks);
      *moves++ = basic_move(origin, dest);
    }
  }
  return moves;
}

static Move *
generate_rook_moves(const struct search_state *state, Move *moves)
{
  uint64_t rooks, attacks;
  int origin, dest;
  const struct board_state *bstate = &state->board_states[state->search_ply];
  const int color = bstate->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  rooks = bstate->type_bitboards[PIECE_TYPE_ROOK] & bstate->color_bitboards[color];
  while (rooks) {
    origin = pop_lss(&rooks);
    attacks
      = get_rook_attack_set(origin, bstate->color_bitboards[0] | bstate->color_bitboards[1])
      & ~bstate->color_bitboards[color];
    while (attacks) {
      dest = pop_lss(&attacks);
      *moves++ = basic_move(origin, dest);
    }
  }
  return moves;
}

static Move *
generate_queen_moves(const struct search_state *state, Move *moves)
{
  uint64_t queens, attacks;
  int origin, dest;
  const struct board_state *bstate = &state->board_states[state->search_ply];
  const int color = bstate->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  queens = bstate->type_bitboards[PIECE_TYPE_QUEEN] & bstate->color_bitboards[color];
  while (queens) {
    origin = pop_lss(&queens);
    attacks
      = (get_rook_attack_set(origin, bstate->color_bitboards[0] | bstate->color_bitboards[1])
      | get_bishop_attack_set(origin, bstate->color_bitboards[0] | bstate->color_bitboards[1]))
      & ~bstate->color_bitboards[color];
    while (attacks) {
      dest = pop_lss(&attacks);
      *moves++ = basic_move(origin, dest);
    }
  }
  return moves;
}

static Bitboard
attack_set(const struct search_state *state, int color)
{
  const int forward = color ? 8 : -8;
  const struct board_state *bstate = &state->board_states[state->search_ply];
  const Bitboard their_all = bstate->color_bitboards[!color];
  uint64_t pieces, attacks;
  int origin;
  attacks = 0;
  /* pawns */
  pieces = bstate->type_bitboards[PIECE_TYPE_PAWN] & bstate->color_bitboards[color];
  attacks |= SHIFT(pieces, forward + 1) & 0xfefefefefefefefe & their_all;
  attacks |= SHIFT(pieces, forward - 1) & 0x7f7f7f7f7f7f7f7f & their_all;
  /* knights */
  pieces = bstate->type_bitboards[PIECE_TYPE_KNIGHT] & bstate->color_bitboards[color];
  while (pieces) {
    origin = pop_lss(&pieces);
    attacks |= knight_attack_table[origin] & ~bstate->color_bitboards[color];
  }
  /* bishop-like */
  pieces
    = (bstate->type_bitboards[PIECE_TYPE_BISHOP]
      | bstate->type_bitboards[PIECE_TYPE_QUEEN])
    & bstate->color_bitboards[color];
  while (pieces) {
    origin = pop_lss(&pieces);
    attacks
      |= get_bishop_attack_set(origin, bstate->color_bitboards[0] | bstate->color_bitboards[1])
      & ~bstate->color_bitboards[color];
  }
  /* rook-like */
  pieces
    = (bstate->type_bitboards[PIECE_TYPE_ROOK]
      | bstate->type_bitboards[PIECE_TYPE_QUEEN])
    & bstate->color_bitboards[color];
  while (pieces) {
    origin = pop_lss(&pieces);
    attacks
      |= get_rook_attack_set(origin, bstate->color_bitboards[0] | bstate->color_bitboards[1])
      & ~bstate->color_bitboards[color];
  }
  /* king */
  pieces = bstate->type_bitboards[PIECE_TYPE_KING] & bstate->color_bitboards[color];
  origin = pop_lss(&pieces);
  attacks |= king_attack_table[origin] & ~bstate->color_bitboards[color];

  return attacks;
}

long
perft(struct search_state *state, int depth, int print)
{
  Move moves[256];
  int move_count, c, i;
  long sum;
  move_count = generate_moves(state, moves) - moves;
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
  for (i = 0; i < move_count; i++) {
    make_move(state, moves[i]);
    c = perft(state, depth - 1, 0);
    unmake_move(state, moves[i]);
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
generate_moves(struct search_state *state, Move *moves)
{
  const struct board_state *bstate = &state->board_states[state->search_ply];
  const int color = bstate->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  int king_sq;
  long in_check;
  Move *base = moves;
  Bitboard opponent_attack_set, new_attacks;

  opponent_attack_set = attack_set(state, !color);
  in_check = bstate->type_bitboards[PIECE_TYPE_KING] & opponent_attack_set;
  king_sq = lss(bstate->type_bitboards[PIECE_TYPE_KING] & bstate->color_bitboards[color]);

  moves = generate_pawn_moves(state, moves);
  moves = generate_knight_moves(state, moves);
  moves = generate_bishop_moves(state, moves);
  moves = generate_rook_moves(state, moves);
  moves = generate_queen_moves(state, moves);
  moves = generate_king_moves(state, moves, opponent_attack_set);
  while (base != moves) {
    if (!in_check && get_move_origin(*base) != king_sq
        && ( set_bit(get_move_origin(*base)) & opponent_attack_set ) == 0
        && get_move_special_type(*base) != SPECIAL_MOVE_EN_PASSANT) {
      base++;
      continue;
    }
    make_move(state, *base);
    new_attacks = attack_set(state, !color);
    if (new_attacks & (bstate+1)->type_bitboards[PIECE_TYPE_KING] & (bstate+1)->color_bitboards[color])
      *base = *(--moves);
    else
      base++;
    unmake_move(state, *base);
  }
  return moves;
}
