#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include "chess.h"

#include <stdio.h>

int
minimax(struct board *board, int depth, int alpha, int beta, Move *best_move)
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
      score = minimax(&next_board, depth - 1, alpha, beta, NULL);
    else
      score = evaluate_board(&next_board);
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
