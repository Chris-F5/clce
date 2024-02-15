#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "chess.h"

static int minimax(const struct board *board, int depth, int alpha, int beta,
    Move *best_move, clock_t deadline);

static int
minimax(const struct board *board, int depth, int alpha, int beta,
    Move *best_move, clock_t deadline)
{
  Move moves[256];
  struct board next_board;
  int move_count, i, best_score, score, color;
  color = board->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  best_score = color ? -CHECKMATE_EVALUATION : CHECKMATE_EVALUATION;
  move_count = generate_moves(board, moves) - moves;
  for (i = 0; i < move_count; i++) {
    next_board = *board;
    make_move(&next_board, moves[i]);
    if (depth > 1)
      score = minimax(&next_board, depth - 1, alpha, beta, NULL, deadline);
    else
      score = evaluate_board(&next_board);
    if (depth > 4 && clock() > deadline) {
      if (best_move != NULL )
        *best_move = 0;
      return 0;
    }
    if (color) {
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
find_move(const struct board *board, int milliseconds)
{
  Move best_move, move;
  clock_t deadline;
  int depth;
  deadline = clock() + (milliseconds * CLOCKS_PER_SEC)/1000;
  depth = 2;
  move = 0;
  do {
    best_move = move;
    depth++;
    minimax(board, depth, -CHECKMATE_EVALUATION, CHECKMATE_EVALUATION, &move, deadline);
  } while(move);
  return best_move;
}
