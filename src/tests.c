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
  struct search_state state;
  if (parse_fen(&state, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"))
    exit(1);
  assert(perft(&state, 4, 0) == 197281);
  if (parse_fen(&state, "6k1/1p3qb1/2pPb1p1/5p1p/3Pp2P/PP4P1/K3B3/2Q2nR1 b - - 0 32"))
    exit(1);
  assert(perft(&state, 4, 0) == 971796);
}

static void
test_evaluation(void)
{
  struct search_state state;
  if (parse_fen(&state, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"))
    exit(1);
  assert(evaluate_board(&state) == 0);
  if (parse_fen(&state, "rnbqkb2/p6p/2p1pn1p/3p1p2/p1PP4/2N2NP1/PP2P1BP/R3R1K1 b q - 0 11"))
    exit(1);
  assert(evaluate_board(&state) == -8);
  if (parse_fen(&state, "6Q1/8/8/8/8/8/8/5k1K w - - 0 1"))
    exit(1);
  assert(evaluate_board(&state) == 9);
}

static void
test_zobrist_hash(void)
{
  struct search_state state;
  Move move;
  uint64_t pawn_hash, non_pawn_hash;
  if (parse_fen(&state, "1rbqk2r/1p1ppp1p/p5pP/n1pN1n2/2P5/3P1NP1/PP2PPB1/R2QK2R w KQk - 0 13"))
    exit(1);
  move = basic_move(9, 25);
  make_move(&state, move);
  move = basic_move(34, 25);
  make_move(&state, move);
  move = basic_move(3, 11);
  make_move(&state, move);
  pawn_hash = state.board_states[state.search_ply].pawn_hash;
  non_pawn_hash = state.board_states[state.search_ply].non_pawn_hash;
  if (parse_fen(&state, "1rbqk2r/1p1ppp1p/p5pP/n2N1n2/1pP5/3P1NP1/P2QPPB1/R3K2R b KQk - 1 14"))
    exit(1);
  assert(pawn_hash == state.board_states[state.search_ply].pawn_hash);
  assert(non_pawn_hash == state.board_states[state.search_ply].non_pawn_hash);
}

static void
test_minimax(void)
{
  /*
  struct board board;
  int score;
  Move move;
  if (parse_fen(&board, "6rk/1R3Qpp/5p1B/1P1p4/3P2PK/6R1/5q2/8 b - - 3 35"))
    exit(1);
  score = minimax(&board, 8, -CHECKMATE_EVALUATION, CHECKMATE_EVALUATION, &move);
  print_move(move);
  printf("\nscore: %d\n", score);
  */
}

void
run_tests(void)
{
  test_perft();
  test_evaluation();
  test_minimax();
  test_zobrist_hash();
}
