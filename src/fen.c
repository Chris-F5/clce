#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include "chess.h"

#include <stdio.h>
#include <string.h>

int
parse_fen(struct board *board, const char *fen)
{
  int r, f;
  unsigned char c;

  memset(board, 0, sizeof(struct board));
  r = 7;
  f = 0;
  while (!(r == 0 && f == 8)) {
    c = *fen++;
    switch (c) {
    case 'p':
      board->type_bitboards[PIECE_TYPE_PAWN] |= (uint64_t)1 << (r * 8 + f);
      board->color_bitboards[COLOR_BLACK] |= (uint64_t)1 << (r * 8 + f);
      set_piece_type(board->mailbox, r * 8 + f, PIECE_TYPE_PAWN);
      f++;
      break;
    case 'n':
      board->type_bitboards[PIECE_TYPE_KNIGHT] |= (uint64_t)1 << (r * 8 + f);
      board->color_bitboards[COLOR_BLACK] |= (uint64_t)1 << (r * 8 + f);
      set_piece_type(board->mailbox, r * 8 + f, PIECE_TYPE_KNIGHT);
      f++;
      break;
    case 'b':
      board->type_bitboards[PIECE_TYPE_BISHOP] |= (uint64_t)1 << (r * 8 + f);
      board->color_bitboards[COLOR_BLACK] |= (uint64_t)1 << (r * 8 + f);
      set_piece_type(board->mailbox, r * 8 + f, PIECE_TYPE_BISHOP);
      f++;
      break;
    case 'r':
      board->type_bitboards[PIECE_TYPE_ROOK] |= (uint64_t)1 << (r * 8 + f);
      board->color_bitboards[COLOR_BLACK] |= (uint64_t)1 << (r * 8 + f);
      set_piece_type(board->mailbox, r * 8 + f, PIECE_TYPE_ROOK);
      f++;
      break;
    case 'q':
      board->type_bitboards[PIECE_TYPE_QUEEN] |= (uint64_t)1 << (r * 8 + f);
      board->color_bitboards[COLOR_BLACK] |= (uint64_t)1 << (r * 8 + f);
      set_piece_type(board->mailbox, r * 8 + f, PIECE_TYPE_QUEEN);
      f++;
      break;
    case 'k':
      board->type_bitboards[PIECE_TYPE_KING] |= (uint64_t)1 << (r * 8 + f);
      board->color_bitboards[COLOR_BLACK] |= (uint64_t)1 << (r * 8 + f);
      set_piece_type(board->mailbox, r * 8 + f, PIECE_TYPE_KING);
      f++;
      break;
    case 'P':
      board->type_bitboards[PIECE_TYPE_PAWN] |= (uint64_t)1 << (r * 8 + f);
      board->color_bitboards[COLOR_WHITE] |= (uint64_t)1 << (r * 8 + f);
      set_piece_type(board->mailbox, r * 8 + f, PIECE_TYPE_PAWN);
      f++;
      break;
    case 'N':
      board->type_bitboards[PIECE_TYPE_KNIGHT] |= (uint64_t)1 << (r * 8 + f);
      board->color_bitboards[COLOR_WHITE] |= (uint64_t)1 << (r * 8 + f);
      set_piece_type(board->mailbox, r * 8 + f, PIECE_TYPE_KNIGHT);
      f++;
      break;
    case 'B':
      board->type_bitboards[PIECE_TYPE_BISHOP] |= (uint64_t)1 << (r * 8 + f);
      board->color_bitboards[COLOR_WHITE] |= (uint64_t)1 << (r * 8 + f);
      set_piece_type(board->mailbox, r * 8 + f, PIECE_TYPE_BISHOP);
      f++;
      break;
    case 'R':
      board->type_bitboards[PIECE_TYPE_ROOK] |= (uint64_t)1 << (r * 8 + f);
      board->color_bitboards[COLOR_WHITE] |= (uint64_t)1 << (r * 8 + f);
      set_piece_type(board->mailbox, r * 8 + f, PIECE_TYPE_ROOK);
      f++;
      break;
    case 'Q':
      board->type_bitboards[PIECE_TYPE_QUEEN] |= (uint64_t)1 << (r * 8 + f);
      board->color_bitboards[COLOR_WHITE] |= (uint64_t)1 << (r * 8 + f);
      set_piece_type(board->mailbox, r * 8 + f, PIECE_TYPE_QUEEN);
      f++;
      break;
    case 'K':
      board->type_bitboards[PIECE_TYPE_KING] |= (uint64_t)1 << (r * 8 + f);
      board->color_bitboards[COLOR_WHITE] |= (uint64_t)1 << (r * 8 + f);
      set_piece_type(board->mailbox, r * 8 + f, PIECE_TYPE_KING);
      f++;
      break;
    case '/':
      if (f != 8) {
        fprintf(stderr, "failed to parse fen: unexpected slash\n");
        return 1;
      }
      f = 0;
      r--;
      break;
    case '1':
      f += 1;
      break;
    case '2':
      f += 2;
      break;
    case '3':
      f += 3;
      break;
    case '4':
      f += 4;
      break;
    case '5':
      f += 5;
      break;
    case '6':
      f += 6;
      break;
    case '7':
      f += 7;
      break;
    case '8':
      f += 8;
      break;
    default:
      fprintf(stderr, "failed to parse fen: unexpected piece character '%c'\n", c);
      return 1;
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
    board->flags |= BOARD_FLAG_WHITE_TO_PLAY;
    break;
  case 'b':
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
  board->flags |= BOARD_FLAGS_WHITE_CASTLE | BOARD_FLAGS_BLACK_CASTLE;
  while ( (c = *fen++) != ' ') {
    switch (c) {
    case 'k':
      board->flags &= ~BOARD_FLAG_BLACK_CASTLE_KING;
      break;
    case 'q':
      board->flags &= ~BOARD_FLAG_BLACK_CASTLE_QUEEN;
      break;
    case 'K':
      board->flags &= ~BOARD_FLAG_WHITE_CASTLE_KING;
      break;
    case 'Q':
      board->flags &= ~BOARD_FLAG_WHITE_CASTLE_QUEEN;
      break;
    case '-':
      break;
    default:
      fprintf(stderr, "failed to parse fen: unexpected castling character '%c'\n", c);
      return 1;
    }
  }
  c = *fen++;
  board->en_passant_square = -1;
  if (c >= 'a' && c <= 'h') {
    board->en_passant_square = c - 'a';
    c = *fen++;
    if (c != '3' && c != '6') {
      fprintf(stderr, "failed to parse fen: unexpected en passant character '%c'\n", c);
      return 1;
    }
    board->en_passant_square += (c - '1') * 8;
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
      board->halfmove_clock = board->halfmove_clock * 10 + c - '0';
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
