#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include "chess.h"

#include <stdio.h>
#include <string.h>

#define SHIFT(v, s) ((s) < 0 ? (v) >> -(s) : (v) << (s))

static Move *
generate_pawn_moves(struct board *board, Move *moves)
{
  struct position *pos;
  Bitboard skip_rank, no_promote_ranks, promote_rank, my_pawns, their_all, empty;
  Bitboard move_1, move_2;
  int col, forward, origin, dest;
  pos = board_position(board);
  col = board_turn(board);
  forward = col ? 8 : -8;
  skip_rank        = col ? 0x0000000000ff0000 : 0x0000ff0000000000;
  no_promote_ranks = col ? 0x0000ffffffffffff : 0xffffffffffff0000;
  promote_rank     = col ? 0x00ff000000000000 : 0x000000000000ff00;
  my_pawns = pos->type_bitboards[PIECE_TYPE_PAWN] & pos->color_bitboards[col];
  their_all = pos->color_bitboards[!col];
  empty = ~(pos->color_bitboards[0] | pos->color_bitboards[1]);

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
  if (pos->en_passant_square >= 0) {
    dest = pos->en_passant_square;
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
generate_knight_moves(struct board *board, Move *moves)
{
  struct position *pos;
  uint64_t knights, attacks;
  int col, origin, dest;
  pos = board_position(board);
  col = board_turn(board);
  knights = pos->type_bitboards[PIECE_TYPE_KNIGHT] & pos->color_bitboards[col];
  while (knights) {
    origin = pop_lss(&knights);
    attacks = knight_attack_table[origin] & ~pos->color_bitboards[col];
    while (attacks) {
      dest = pop_lss(&attacks);
      *moves++ = basic_move(origin, dest);
    }
  }
  return moves;
}

static Move *
generate_king_moves(struct board *board, Move *moves)
{
  struct position *pos;
  uint64_t king, attacks;
  int col, origin, dest;
  pos = board_position(board);
  col = board_turn(board);
  king = pos->type_bitboards[PIECE_TYPE_KING] & pos->color_bitboards[col];
  assert(king);
  origin = lss(king);
  attacks = king_attack_table[origin] & ~pos->color_bitboards[col];
  while (attacks) {
    dest = pop_lss(&attacks);
    *moves++ = basic_move(origin, dest);
  }
  /* castling */
  if ( (pos->flags & KING_CASTLE_BOARD_FLAG(col)) == 0
  &&   ((pos->color_bitboards[0] | pos->color_bitboards[1]) & KING_CASTLE_GAP(col)) == 0
  &&   (pos->attack_sets[!col] & KING_CASTLE_CHECK_SQUARES(col)) == 0)
      *moves++ = castle_move(col, 1);
  if ( (pos->flags & QUEEN_CASTLE_BOARD_FLAG(col)) == 0
  &&   ((pos->color_bitboards[0] | pos->color_bitboards[1]) & QUEEN_CASTLE_GAP(col)) == 0
  &&   (pos->attack_sets[!col] & QUEEN_CASTLE_CHECK_SQUARES(col)) == 0)
      *moves++ = castle_move(col, 0);
  return moves;
}

static Move *
generate_bishop_moves(struct board *board, Move *moves)
{
  struct position *pos;
  uint64_t bishops, attacks;
  int col, origin, dest;
  pos = board_position(board);
  col = board_turn(board);
  bishops = pos->type_bitboards[PIECE_TYPE_BISHOP] & pos->color_bitboards[col];
  while (bishops) {
    origin = pop_lss(&bishops);
    attacks
      = get_bishop_attack_set(origin, pos->color_bitboards[0] | pos->color_bitboards[1])
      & ~pos->color_bitboards[col];
    while (attacks) {
      dest = pop_lss(&attacks);
      *moves++ = basic_move(origin, dest);
    }
  }
  return moves;
}

static Move *
generate_rook_moves(struct board *board, Move *moves)
{
  struct position *pos;
  uint64_t rooks, attacks;
  int col, origin, dest;
  pos = board_position(board);
  col = board_turn(board);
  rooks = pos->type_bitboards[PIECE_TYPE_ROOK] & pos->color_bitboards[col];
  while (rooks) {
    origin = pop_lss(&rooks);
    attacks
      = get_rook_attack_set(origin, pos->color_bitboards[0] | pos->color_bitboards[1])
      & ~pos->color_bitboards[col];
    while (attacks) {
      dest = pop_lss(&attacks);
      *moves++ = basic_move(origin, dest);
    }
  }
  return moves;
}

static Move *
generate_queen_moves( struct board *board, Move *moves)
{
  struct position *pos;
  uint64_t queens, attacks;
  int col, origin, dest;
  pos = board_position(board);
  col = board_turn(board);
  queens = pos->type_bitboards[PIECE_TYPE_QUEEN] & pos->color_bitboards[col];
  while (queens) {
    origin = pop_lss(&queens);
    attacks
      = (get_rook_attack_set(origin, pos->color_bitboards[0] | pos->color_bitboards[1])
      | get_bishop_attack_set(origin, pos->color_bitboards[0] | pos->color_bitboards[1]))
      & ~pos->color_bitboards[col];
    while (attacks) {
      dest = pop_lss(&attacks);
      *moves++ = basic_move(origin, dest);
    }
  }
  return moves;
}


static uint64_t
find_attack_set(uint64_t *color_bitboards, uint64_t *type_bitboards, int col)
{
  uint64_t pieces, set;
  int origin, forward;

  forward = col ? 8 : -8;
  set = 0;

  /* pawns */
  pieces = type_bitboards[PIECE_TYPE_PAWN] & color_bitboards[col];
  set |= SHIFT(pieces, forward + 1) & 0xfefefefefefefefe;
  set |= SHIFT(pieces, forward - 1) & 0x7f7f7f7f7f7f7f7f;
  /* knights */
  pieces = type_bitboards[PIECE_TYPE_KNIGHT] & color_bitboards[col];
  while (pieces) {
    origin = pop_lss(&pieces);
    set |= knight_attack_table[origin] & ~color_bitboards[col];
  }
  /* bishop-like */
  pieces
    = (type_bitboards[PIECE_TYPE_BISHOP]
      | type_bitboards[PIECE_TYPE_QUEEN])
    & color_bitboards[col];
  while (pieces) {
    origin = pop_lss(&pieces);
    set
      |= get_bishop_attack_set(origin, color_bitboards[0] | color_bitboards[1])
      & ~color_bitboards[col];
  }
  /* rook-like */
  pieces
    = (type_bitboards[PIECE_TYPE_ROOK]
      | type_bitboards[PIECE_TYPE_QUEEN])
    & color_bitboards[col];
  while (pieces) {
    origin = pop_lss(&pieces);
    set
      |= get_rook_attack_set(origin, color_bitboards[0] | color_bitboards[1])
      & ~color_bitboards[col];
  }
  /* king */
  pieces = type_bitboards[PIECE_TYPE_KING] & color_bitboards[col];
  origin = pop_lss(&pieces);
  set |= king_attack_table[origin] & ~color_bitboards[col];

  return set;
}

int
create_board(struct board *board, const char *fen)
{
  struct position *pos;
  int r, f;
  int piece_type, piece_color;
  unsigned char c;
  memset(board, 0, sizeof(struct board));
  pos = &board->stack[0];
  r = 7;
  f = 0;
  while (!(r == 0 && f == 8)) {
    c = *fen++;
    if (c >= '1' && c <= '8') {
      f += c - '0';
    } else if (c == '/') {
      if (f != 8) {
        fprintf(stderr, "failed to parse fen: unexpected slash\n");
        return 1;
      }
      f = 0;
      r--;
    } else {
      piece_color = c < 'a' ? COLOR_WHITE : COLOR_BLACK;
      switch (c < 'a' ? c + 0x20 : c) {
      case 'p':
        piece_type = PIECE_TYPE_PAWN;
        break;
      case 'n':
        piece_type = PIECE_TYPE_KNIGHT;
        break;
      case 'b':
        piece_type = PIECE_TYPE_BISHOP;
        break;
      case 'r':
        piece_type = PIECE_TYPE_ROOK;
        break;
      case 'q':
        piece_type = PIECE_TYPE_QUEEN;
        break;
      case 'k':
        piece_type = PIECE_TYPE_KING;
        break;
      default:
        fprintf(stderr, "failed to parse fen: unexpected piece character '%c'\n", c);
        return 1;
      }
      pos->type_bitboards[piece_type] |= set_bit(r * 8 + f);
      pos->color_bitboards[piece_color] |= set_bit(r * 8 + f);
      set_piece_type(pos->mailbox, r * 8 + f, piece_type);
      if (piece_type == PIECE_TYPE_PAWN)
        pos->pawn_hash ^= get_zobrist_piece_number(piece_color, piece_type, r * 8 + f);
      else
        pos->non_pawn_hash ^= get_zobrist_piece_number(piece_color, piece_type, r * 8 + f);
      f++;
    }
  }
  c = *fen++;
  if (c != ' ') {
    fprintf(stderr, "failed to parse fen: expected space after pieces but found '%c'\n", c);
    return 1;
  }
  c = *fen++;
  switch (c) {
  case 'w':
    pos->flags |= BOARD_FLAG_WHITE_TO_PLAY;
    break;
  case 'b':
    pos->non_pawn_hash ^= zobrist_black_number;
    break;
  default:
    fprintf(stderr, "failed to parse fen: unexpected turn character '%c'\n", c);
    return 1;
  }
  c = *fen++;
  if (c != ' ') {
    fprintf(stderr, "failed to parse fen: expected space after turn field but found '%c'\n", c);
    return 1;
  }
  pos->flags |= CASTLE_BOARD_FLAGS;
  while ( (c = *fen++) != ' ') {
    switch (c) {
    case 'k':
      pos->flags &= ~BOARD_FLAG_BLACK_CASTLE_KING;
      break;
    case 'q':
      pos->flags &= ~BOARD_FLAG_BLACK_CASTLE_QUEEN;
      break;
    case 'K':
      pos->flags &= ~BOARD_FLAG_WHITE_CASTLE_KING;
      break;
    case 'Q':
      pos->flags &= ~BOARD_FLAG_WHITE_CASTLE_QUEEN;
      break;
    case '-':
      break;
    default:
      fprintf(stderr, "failed to parse fen: unexpected castling character '%c'\n", c);
      return 1;
    }
  }
  pos->non_pawn_hash
    ^= zobrist_castling_numbers[(pos->flags >> 1) & 0x0f];
  c = *fen++;
  pos->en_passant_square = -1;
  if (c >= 'a' && c <= 'h') {
    pos->en_passant_square = c - 'a';
    c = *fen++;
    if (c != '3' && c != '6') {
      fprintf(stderr, "failed to parse fen: unexpected en passant character '%c'\n", c);
      return 1;
    }
    pos->en_passant_square += (c - '1') * 8;
    pos->non_pawn_hash
      ^= zobrist_en_passant_numbers[pos->en_passant_square % 8];
  } else if (c != '-') {
    fprintf(stderr, "failed to parse fen: unexpected en passant character '%c'\n", c);
    return 1;
  }
  c = *fen++;
  if (c != ' ') {
    fprintf(stderr, "failed to parse fen: expected space after en passant field but found '%c'\n", c);
    return 1;
  }
  while ( (c = *fen++) != ' ') {
    if (c >= '0' && c <= '9') {
      pos->halfmove_clock = pos->halfmove_clock * 10 + c - '0';
    } else {
      fprintf(stderr, "failed to parse fen: unexpected halfmove clock character '%c'\n", c);
      return 1;
    }
  }
  while ( (c = *fen++) != '\0') {
    if (c == '\n')
      break;
    if (c >= '0' && c <= '9') {
      board->fullmove_clock = board->fullmove_clock * 10 + c - '0';
    } else {
      fprintf(stderr, "failed to parse fen: unexpected fullmove clock character '%c'\n", c);
      return 1;
    }
  }
  pos->attack_sets[0] = find_attack_set(pos->color_bitboards, pos->type_bitboards, 0);
  pos->attack_sets[1] = find_attack_set(pos->color_bitboards, pos->type_bitboards, 1);
  return 0;
}

void
board_push(struct board *board, Move move)
{
  Bitboard their_pawns;
  struct position *pos;
  int col, forward, origin, dest, piece_type;
  int castle_origin, castle_dest;
  int other_piece;

  board->stack[board->ply + 1] = board->stack[board->ply];
  board->ply++;
  pos = board_position(board);
  col = board_turn(board);
  forward = col ? 8 : -8;
  origin = move_origin(move);
  dest = move_dest(move);
  piece_type = get_piece_type(pos->mailbox, origin);

  board->fullmove_clock++;
  pos->halfmove_clock++;

  /* capture */
  if (pos->color_bitboards[!col] & set_bit(dest)) {
    pos->halfmove_clock = 0;
    /* clear captured piece */
    other_piece = get_piece_type(pos->mailbox, dest);
    pos->type_bitboards[other_piece] ^= set_bit(dest);
    pos->color_bitboards[!col] ^= set_bit(dest);
    if (other_piece == PIECE_TYPE_PAWN)
      pos->pawn_hash
        ^= get_zobrist_piece_number(!col, PIECE_TYPE_PAWN, dest);
    else
      pos->non_pawn_hash
        ^= get_zobrist_piece_number(!col, other_piece, dest);
  }

  /* move piece */
  if (pos->en_passant_square >= 0) {
    pos->pawn_hash ^= zobrist_en_passant_numbers[pos->en_passant_square & 0x07];
    pos->en_passant_square = -1;
  }
  pos->type_bitboards[piece_type] ^= set_bit(origin) | set_bit(dest);
  pos->color_bitboards[col] ^= set_bit(origin) | set_bit(dest);
  set_piece_type(pos->mailbox, dest, piece_type);
  if (piece_type == PIECE_TYPE_PAWN) {
    pos->pawn_hash ^= get_zobrist_piece_number(col, PIECE_TYPE_PAWN, origin);
    pos->pawn_hash ^= get_zobrist_piece_number(col, PIECE_TYPE_PAWN, dest);
  } else {
    pos->non_pawn_hash ^= get_zobrist_piece_number(col, piece_type, origin);
    pos->non_pawn_hash ^= get_zobrist_piece_number(col, piece_type, dest);
  }

  /* pawn specials */
  if (piece_type == PIECE_TYPE_PAWN) {
    pos->halfmove_clock = 0;
    if ((origin ^ dest) == 16) { /* move two */
      their_pawns = pos->color_bitboards[!col] & pos->type_bitboards[PIECE_TYPE_PAWN];
      if ((set_bit(dest) << 1) & 0xfefefefefefefefe & their_pawns
          || (set_bit(dest) >> 1) & 0x7f7f7f7f7f7f7f7f & their_pawns) {
        pos->en_passant_square = origin + forward;
        pos->pawn_hash ^= zobrist_en_passant_numbers[pos->en_passant_square & 0x07];
      }
    } else if (move_special_type(move) == SPECIAL_MOVE_PROMOTE) {
      other_piece = move_promote_piece(move);
      set_piece_type(pos->mailbox, dest, other_piece);
      pos->type_bitboards[PIECE_TYPE_PAWN] ^= set_bit(dest);
      pos->pawn_hash ^= get_zobrist_piece_number(col, PIECE_TYPE_PAWN, dest);
      pos->type_bitboards[other_piece] ^= set_bit(dest);
      pos->non_pawn_hash ^= get_zobrist_piece_number(col, other_piece, dest);
    } else if (move_special_type(move) == SPECIAL_MOVE_EN_PASSANT) {
      pos->type_bitboards[PIECE_TYPE_PAWN] ^= set_bit(dest - forward);
      pos->color_bitboards[!col] ^= set_bit(dest - forward);
      pos->pawn_hash ^= get_zobrist_piece_number(!col, PIECE_TYPE_PAWN, dest - forward);
    }
  }

  if (piece_type == PIECE_TYPE_KING) {
    if (col == COLOR_WHITE)
      pos->flags |= BOARD_FLAG_WHITE_CASTLE_KING | BOARD_FLAG_WHITE_CASTLE_QUEEN;
    else
      pos->flags |= BOARD_FLAG_BLACK_CASTLE_KING | BOARD_FLAG_BLACK_CASTLE_QUEEN;
  }
  if (piece_type == PIECE_TYPE_ROOK) {
    pos->non_pawn_hash ^= (pos->flags >> 1) & 0x0f;
    switch(origin) {
    case 0:
      pos->flags |= BOARD_FLAG_WHITE_CASTLE_QUEEN;
      break;
    case 7:
      pos->flags |= BOARD_FLAG_WHITE_CASTLE_KING;
      break;
    case 56:
      pos->flags |= BOARD_FLAG_BLACK_CASTLE_QUEEN;
      break;
    case 63:
      pos->flags |= BOARD_FLAG_BLACK_CASTLE_KING;
      break;
    }
    pos->non_pawn_hash ^= (pos->flags >> 1) & 0x0f;
  }

  /* castling */
  if (move_special_type(move) == SPECIAL_MOVE_CASTLING) {
    pos->non_pawn_hash ^= (pos->flags >> 1) & 0x0f;
    pos->flags |= col
      ? BOARD_FLAG_WHITE_CASTLE_KING | BOARD_FLAG_WHITE_CASTLE_QUEEN
      : BOARD_FLAG_BLACK_CASTLE_KING | BOARD_FLAG_BLACK_CASTLE_QUEEN;
    pos->non_pawn_hash ^= (pos->flags >> 1) & 0x0f;
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
    set_piece_type(pos->mailbox, castle_dest, PIECE_TYPE_ROOK);
    pos->color_bitboards[col]          ^= set_bit(castle_origin) | set_bit(castle_dest);
    pos->type_bitboards[PIECE_TYPE_ROOK] ^= set_bit(castle_origin) | set_bit(castle_dest);
    pos->non_pawn_hash ^= get_zobrist_piece_number(col, PIECE_TYPE_ROOK, castle_origin);
    pos->non_pawn_hash ^= get_zobrist_piece_number(col, PIECE_TYPE_ROOK, castle_dest);
  }
no_castle:

  pos->attack_sets[0] = find_attack_set(pos->color_bitboards, pos->type_bitboards, 0);
  pos->attack_sets[1] = find_attack_set(pos->color_bitboards, pos->type_bitboards, 1);
  pos->flags ^= BOARD_FLAG_WHITE_TO_PLAY;
  pos->non_pawn_hash ^= zobrist_black_number;
}

void
board_pop(struct board *board, Move move)
{
  assert(board->ply > 0);
  assert(board->fullmove_clock > 0);
  board->ply--;
  board->fullmove_clock--;
}

int
board_is_repetition(struct board *board)
{
  struct position *pos;
  int i;
  uint64_t pawn_hash, non_pawn_hash;
  pos = &board->stack[board->ply];
  pawn_hash = pos->pawn_hash;
  non_pawn_hash = pos->non_pawn_hash;
  for (i = board->ply - 1; i >= 0; i--) {
    pos = &board->stack[i];
    if (pos->pawn_hash == pawn_hash && pos->non_pawn_hash == non_pawn_hash)
      return 1;
  }
  return 0;
}

int
board_moves(struct board *board, Move *moves)
{
  struct position *pos, *new_pos;
  int col, king_sq;
  Move *base, *legal_moves;
  pos = board_position(board);
  col = board_turn(board);
  base = legal_moves = moves;
  king_sq = lss(pos->type_bitboards[PIECE_TYPE_KING] & pos->color_bitboards[col]);

  moves = generate_pawn_moves(board, moves);
  moves = generate_knight_moves(board, moves);
  moves = generate_bishop_moves(board, moves);
  moves = generate_rook_moves(board, moves);
  moves = generate_queen_moves(board, moves);
  moves = generate_king_moves(board, moves);
  while (legal_moves != moves) {
    if (!board_in_check(board)
    &&  move_origin(*legal_moves) != king_sq
    &&  (set_bit(move_origin(*legal_moves)) & pos->attack_sets[!col] ) == 0
    &&  move_special_type(*legal_moves) != SPECIAL_MOVE_EN_PASSANT) {
      legal_moves++;
      continue;
    }
    board_push(board, *legal_moves);
    new_pos = board_position(board);
    if (new_pos->attack_sets[!col]
    &   new_pos->type_bitboards[PIECE_TYPE_KING]
    &   new_pos->color_bitboards[col])
      *legal_moves = *(--moves);
    else
      legal_moves++;
    board_pop(board, *legal_moves);
  }
  return moves - base;
}
