#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include "chess.h"

#include <stdio.h>
#include <string.h>

int
parse_fen(struct search_state *state, const char *fen)
{
  int r, f;
  int piece_type, piece_color;
  unsigned char c;

  memset(state, 0, sizeof(struct search_state));
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
      state->board_states[0].type_bitboards[piece_type] |= (uint64_t)1 << (r * 8 + f);
      state->board_states[0].color_bitboards[piece_color] |= (uint64_t)1 << (r * 8 + f);
      set_piece_type(state->board_states[0].mailbox, r * 8 + f, piece_type);
      if (piece_type == PIECE_TYPE_PAWN)
        state->board_states[0].pawn_hash
          ^= get_zobrist_piece_number(piece_color, piece_type, r * 8 + f);
      else
        state->board_states[0].non_pawn_hash
          ^= get_zobrist_piece_number(piece_color, piece_type, r * 8 + f);
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
    state->board_states[0].flags |= BOARD_FLAG_WHITE_TO_PLAY;
    break;
  case 'b':
    state->board_states[0].non_pawn_hash ^= zobrist_black_number;
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
  state->board_states[0].flags |= BOARD_FLAGS_WHITE_CASTLE | BOARD_FLAGS_BLACK_CASTLE;
  while ( (c = *fen++) != ' ') {
    switch (c) {
    case 'k':
      state->board_states[0].flags &= ~BOARD_FLAG_BLACK_CASTLE_KING;
      break;
    case 'q':
      state->board_states[0].flags &= ~BOARD_FLAG_BLACK_CASTLE_QUEEN;
      break;
    case 'K':
      state->board_states[0].flags &= ~BOARD_FLAG_WHITE_CASTLE_KING;
      break;
    case 'Q':
      state->board_states[0].flags &= ~BOARD_FLAG_WHITE_CASTLE_QUEEN;
      break;
    case '-':
      break;
    default:
      fprintf(stderr, "failed to parse fen: unexpected castling character '%c'\n", c);
      return 1;
    }
  }
  state->board_states[0].non_pawn_hash
    ^= zobrist_castling_numbers[(state->board_states[0].flags >> 1) & 0x0f];
  c = *fen++;
  state->board_states[0].en_passant_square = -1;
  if (c >= 'a' && c <= 'h') {
    state->board_states[0].en_passant_square = c - 'a';
    c = *fen++;
    if (c != '3' && c != '6') {
      fprintf(stderr, "failed to parse fen: unexpected en passant character '%c'\n", c);
      return 1;
    }
    state->board_states[0].en_passant_square += (c - '1') * 8;
    state->board_states[0].non_pawn_hash
      ^= zobrist_en_passant_numbers[state->board_states[0].en_passant_square % 8];
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
      state->board_states[0].halfmove_clock = state->board_states[0].halfmove_clock * 10 + c - '0';
    } else {
      fprintf(stderr, "failed to parse fen: unexpected halfmove clock character '%c'\n", c);
      return 1;
    }
  }
  while ( (c = *fen++) != '\0') {
    if (c == '\n')
      break;
    if (c >= '0' && c <= '9') {
      state->fullmove_clock = state->fullmove_clock * 10 + c - '0';
    } else {
      fprintf(stderr, "failed to parse fen: unexpected fullmove clock character '%c'\n", c);
      return 1;
    }
  }
  return 0;
}

/*
void
write_fen(const struct board *board, char *buf)
{
  int i;
  unsigned char run;
  run = 0;
  for (i = 63; i >= 0; i--) {
    if (i % 8 == 7) {
      if (run)
        *buf++ = '0' + run;
      if (i < 63)
        *buf++ = '/';
      run = 0;
    }
    if ( (((uint64_t)1 << i) & (board->all_white | board->all_black)) == 0) {
      run++;
      continue;
    }
    if (run)
      *buf++ = '0' + run;
    run = 0;
    if ( ((uint64_t)1 << i) & board->pawns_white)
      *buf++ = 'P';
    else if ( ((uint64_t)1 << i) & board->knights_white)
      *buf++ = 'N';
    else if ( ((uint64_t)1 << i) & board->bishops_white)
      *buf++ = 'B';
    else if ( ((uint64_t)1 << i) & board->rooks_white)
      *buf++ = 'R';
    else if ( ((uint64_t)1 << i) & board->queens_white)
      *buf++ = 'Q';
    else if ( ((uint64_t)1 << i) & board->kings_white)
      *buf++ = 'K';
    else if ( ((uint64_t)1 << i) & board->pawns_black)
      *buf++ = 'p';
    else if ( ((uint64_t)1 << i) & board->knights_black)
      *buf++ = 'n';
    else if ( ((uint64_t)1 << i) & board->bishops_black)
      *buf++ = 'b';
    else if ( ((uint64_t)1 << i) & board->rooks_black)
      *buf++ = 'r';
    else if ( ((uint64_t)1 << i) & board->queens_black)
      *buf++ = 'q';
    else if ( ((uint64_t)1 << i) & board->kings_black)
      *buf++ = 'k';
  }
  if (run)
    *buf++ = '0' + run;
  *buf++ = ' ';
  *buf++ = board->flags & BOARD_FLAG_WHITE_TO_PLAY ? 'w' : 'b';
  *buf++ = ' ';
  if (board->flags & BOARD_FLAG_WHITE_CASTLE_KING)
    *buf++ = 'K';
  if (board->flags & BOARD_FLAG_WHITE_CASTLE_QUEEN)
    *buf++ = 'Q';
  if (board->flags & BOARD_FLAG_BLACK_CASTLE_KING)
    *buf++ = 'k';
  if (board->flags & BOARD_FLAG_BLACK_CASTLE_QUEEN)
    *buf++ = 'q';
  if (*(buf - 1) == ' ')
    *buf++ = '-';
  *buf++ = ' ';
  if (board->en_passant_file < 9) {
    *buf++ = 'a' + board->en_passant_file;
    *buf++ = board->flags & BOARD_FLAG_WHITE_TO_PLAY ? '6' : '3';
  } else {
    *buf++ = '-';
  }
  *buf++ = ' ';
  sprintf(buf, "%u %u", board->halfmove_clock, board->fullmove_clock);
}
*/
