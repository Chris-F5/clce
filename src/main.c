#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "chess.h"

#include <stdio.h>

#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

static long
perft(struct board *board, int depth, int print)
{
  Move moves[256];
  long sum;
  int move_count, c, i;
  if (depth == 0)
    return 1;
  move_count = board_moves(board, moves);
  if (move_count == 0 && print)
    printf("\n");
  sum = 0;
  for (i = 0; i < move_count; i++) {
    board_push(board, moves[i]);
    c = perft(board, depth - 1, 0);
    board_pop(board, moves[i]);
    if (print) {
      print_move(moves[i]);
      printf(":%d%c", c, i == move_count - 1 ? '\n' : ' ');
    }
    sum += c;
  }
  return sum;
}

void
repl_command(char *command)
{
  int d;
  struct board board;
  Move move;
  char *arg;
  if ( (arg = strchr(command, '\n')) ) *arg = '\0';
  arg = strtok(command, ":");
  if (strcmp(arg, "go") == 0) {
    arg = strtok(NULL, ":");
    if (arg == NULL) arg = DEFAULT_FEN;
    if (create_board(&board, arg)) goto invalid_command;
    arg = strtok(NULL, ":");
    d = arg ? atoi(arg) : 4000;
    move = find_move(&board, d);
    print_move(move);
    printf("\n");
  } else if (strcmp(arg, "perft") == 0) {
    arg = strtok(NULL, ":");
    if (arg == NULL) arg = DEFAULT_FEN;
    if (create_board(&board, arg)) goto invalid_command;
    arg = strtok(NULL, ":");
    d = arg ? atoi(arg) : 4;
    arg = strtok(NULL, ":");
    perft(&board, d, 1);
  } else {
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
