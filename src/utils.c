#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "chess.h"

const char *square_names[64] = {
  "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
  "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
  "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
  "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
  "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
  "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
  "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
  "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
};
const char piece_chars[] = {
  [PIECE_TYPE_PAWN]   = 'p',
  [PIECE_TYPE_KNIGHT] = 'n',
  [PIECE_TYPE_BISHOP] = 'b',
  [PIECE_TYPE_ROOK]   = 'r',
  [PIECE_TYPE_QUEEN]  = 'q',
  [PIECE_TYPE_KING]   = 'k',
};

void *
xmalloc(size_t len)
{
  void *p;
  if ( (p = malloc(len)) == NULL)
    perror("malloc");
  return p;
}

void *
xrealloc(void *p, size_t len)
{
  if ( (p = realloc(p, len)) == NULL)
    perror("realloc");
  return p;
}

void
print_bitmap(uint64_t bitmap)
{
  int r, f;
  for (r = 7; r >= 0; r--) {
    for (f = 0; f < 8; f++)
      printf("%c ", bitmap & ((uint64_t)1 << (r * 8 + f)) ? '1' : '.');
    putc('\n', stdout);
  }
}

void
print_move(Move move)
{
  printf("%s%s", square_names[(move >> 6) & 0x3f], square_names[move & 0x3f]);
  if (move_special_type(move) == SPECIAL_MOVE_PROMOTE)
    putchar(piece_chars[move_promote_piece(move)]);
}

void
print_board(struct board *board)
{
  struct position *pos;
  char piece_char;
  int r, f;
  pos = board_position(board);
  for (r = 7; r >= 0; r--) {
    printf("+---+---+---+---+---+---+---+---+\n");
    for (f = 0; f < 8; f++) {
      piece_char = piece_chars[get_piece_type(pos->mailbox, r * 8 + f)];
      if (pos->color_bitboards[COLOR_WHITE] & ((uint64_t)1 << (r * 8 + f)))
        piece_char = toupper(piece_char);
      else if (!(pos->color_bitboards[COLOR_BLACK] & ((uint64_t)1 << (r * 8 + f))))
        piece_char = ' ';
      printf("| %c ", piece_char);
    }
    printf("| %d\n", r + 1);
  }
  printf("+---+---+---+---+---+---+---+---+\n");
  printf("  a   b   c   d   e   f   g   h\n");
  fflush(stdout);
}

void
read_buffer(char *buffer, int len)
{
  int c, i;
  i = 0;
  while ( (c = fgetc(stdin)) != '\n' ) {
    if (c == EOF)
      break;
    if (i >= len)
      continue;
    buffer[i++] = c;
  }
  if (i >= len)
    return;
  buffer[i] = '\n';
}
