#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "chess.h"

static int minimax(struct search_state *state, int depth, int alpha, int beta,
    Move *best_move, clock_t deadline);

static int
minimax(struct search_state *state, int depth, int alpha, int beta,
    Move *best_move, clock_t deadline)
{
  Move moves[256];
  int move_count, i, best_score, score, color;
  struct board_state *bstate;
  bstate = &state->board_states[state->search_ply];
  color = bstate->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  best_score = color ? -CHECKMATE_EVALUATION : CHECKMATE_EVALUATION;
  move_count = generate_moves(state, moves) - moves;
  for (i = 0; i < move_count; i++) {
    make_move(state, moves[i]);
    if (depth > 1)
      score = minimax(state, depth - 1, alpha, beta, NULL, deadline);
    else
      score = evaluate_board(state);
    unmake_move(state, moves[i]);
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
find_move(struct search_state *state, int milliseconds)
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
    minimax(state, depth, -CHECKMATE_EVALUATION, CHECKMATE_EVALUATION, &move, deadline);
  } while(move);
  return best_move;
}
