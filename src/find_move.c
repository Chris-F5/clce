#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "chess.h"

static int
minimax(struct board *board, int depth, int alpha, int beta, Move *best_move,
    clock_t deadline)
{
  Move moves[256];
  int col, move_count, i, best_score, score;
  col = board_turn(board);
  best_score = col ? -CHECKMATE_EVALUATION-1 : CHECKMATE_EVALUATION+1;
  move_count = board_moves(board, moves);
  if (move_count == 0) {
    assert(best_move == NULL);
    return col ? -CHECKMATE_EVALUATION : CHECKMATE_EVALUATION;
  }
  for (i = 0; i < move_count; i++) {
    board_push(board, moves[i]);
    if (board_is_repetition(board))
      score = 0;
    else if (depth == 0)
      score = evaluate_board(board);
    else
      score = minimax(board, depth - 1, alpha, beta, NULL, deadline);
    board_pop(board, moves[i]);
    if (board->ply < 4 && clock() > deadline) {
      if (best_move != NULL )
        *best_move = 0;
      return 0;
    }
    if (col) {
      if (score > best_score) {
        best_score = score;
        alpha = score;
        if (best_move != NULL)
          *best_move = moves[i];
        if (score >= beta)
          break;
      }
    } else {
      if (score < best_score) {
        best_score = score;
        beta = score;
        if (best_move != NULL)
          *best_move = moves[i];
        if (score <= alpha)
          break;
      }
    }
  }
  return best_score;
}

Move
find_move(struct board *board, int milliseconds)
{
  Move best_move, move;
  clock_t deadline;
  int depth;
  deadline = clock() + (milliseconds * CLOCKS_PER_SEC)/1000;
  depth = 1;
  move = 0;
  do {
    best_move = move;
    depth++;
    minimax(board, depth, -CHECKMATE_EVALUATION-1, CHECKMATE_EVALUATION+1, &move, deadline);
  } while(move);
  assert(best_move);
  /* TODO: handle case when no move found before deadline */
  return best_move;
}
