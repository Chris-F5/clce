#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "chess.h"

static void test_perft(void);
static void test_evaluation(void);

static void
test_perft(void)
{
  struct board board;
  if (parse_fen(&board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"))
    exit(1);
  assert(perft(&board, 4, 0) == 197281);
  if (parse_fen(&board, "6k1/1p3qb1/2pPb1p1/5p1p/3Pp2P/PP4P1/K3B3/2Q2nR1 b - - 0 32"))
    exit(1);
  assert(perft(&board, 4, 0) == 971796);
}

static void
test_evaluation(void)
{
  struct board board;
  if (parse_fen(&board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"))
    exit(1);
  assert(evaluate_board(&board) == 0);
  if (parse_fen(&board, "rnbqkb2/p6p/2p1pn1p/3p1p2/p1PP4/2N2NP1/PP2P1BP/R3R1K1 b q - 0 11"))
    exit(1);
  assert(evaluate_board(&board) == -8);
  if (parse_fen(&board, "6Q1/8/8/8/8/8/8/5k1K w - - 0 1"))
    exit(1);
  assert(evaluate_board(&board) == 9);
}

static void
test_minimax(void)
{
  struct board board;
  int score;
  Move move;
  if (parse_fen(&board, "6rk/1R3Qpp/5p1B/1P1p4/3P2PK/6R1/5q2/8 b - - 3 35"))
    exit(1);
  score = minimax(&board, 8, -CHECKMATE_EVALUATION, CHECKMATE_EVALUATION, &move);
  print_move(move);
  printf("\nscore: %d\n", score);
}

void
run_tests(void)
{
  test_perft();
  test_evaluation();
  test_minimax();
}
