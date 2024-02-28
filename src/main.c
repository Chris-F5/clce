#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "chess.h"

#include <stdio.h>

#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

static long
perft(struct board *board, int depth, int gen_flags, int print)
{
  Move moves[256];
  long sum;
  int move_count, c, i;
  if (depth == 0)
    return 1;
  move_count = board_moves(board, moves, gen_flags);
  if (move_count == 0 && print)
    printf("\n");
  sum = 0;
  for (i = 0; i < move_count; i++) {
    board_push(board, moves[i]);
    c = perft(board, depth - 1, gen_flags, 0);
    board_pop(board, moves[i]);
    if (print) {
      print_move(moves[i]);
      printf(":%d%c", c, i == move_count - 1 ? '\n' : ' ');
    }
    sum += c;
  }
  return sum;
}

static void
tok_int(int *v, int *err)
{
  char *s, *end;
  long l;
  s = strtok(NULL, ":");
  if (s == NULL) {
    *err = 1;
    return;
  }
  l = strtol(s, &end, 10);
  if (*end != '\0') {
    *err = 1;
    return;
  }
  *v = l;
}

static void
tok_fen(struct board *board, int *err)
{
  char *s;
  s = strtok(NULL, ":");
  if (s == NULL) {
    *err = 1;
    return;
  }
  if (create_board(board, s)) {
    *err = 1;
    return;
  }
}

static void
tok_char(char *c, int *err)
{
  char *s;
  s = strtok(NULL, ":");
  if (s == NULL) {
    *err = 1;
    return;
  }
  if (strlen(s) != 1) {
    *err = 1;
    return;
  }
  *c = s[0];
}

static void
repl_command(char *command)
{
  struct board board;
  Move move;
  char *cmd;
  int err, d1;
  char c, v;
  if ( (cmd = strchr(command, '\n')) ) *cmd = '\0';
  cmd = strtok(command, ":");
  err = 0;
  if (strcmp(cmd, "go") == 0) {
    tok_fen(&board, &err);
    tok_int(&d1, &err);
    tok_char(&v, &err);
    if (err) goto invalid_command;
    move = find_move(&board, d1, v == 'v');
    print_move(move);
    printf("\n");
  } else if (strcmp(cmd, "perft") == 0) {
    tok_fen(&board, &err);
    tok_int(&d1, &err);
    tok_char(&c, &err);
    if (err) goto invalid_command;
    perft(&board, d1, c == 'q' ? ~GEN_FLAG_CAPTURES : ~0, 1);
  }  else {
    goto invalid_command;
  }
  return;
invalid_command:
  fprintf(stderr, "Invalid command. Exiting...\n");
  exit(1);
}

void
repl_start(void)
{
  char buffer[MAX_FEN_SIZE + 1];
  for (;;) {
    if (fgets(buffer, sizeof(buffer), stdin) == NULL)
      break;
    repl_command(buffer);
    fflush(stdout);
  }
}

int
main(int argc, char **argv)
{
  /* print_best_magics(); */
  init_bitboards();
  printf("READY\n");
  fflush(stdout);
  repl_start();
  return 0;
}
