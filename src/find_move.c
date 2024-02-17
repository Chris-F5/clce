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
is_repetition(const struct search_state *state)
{
  int i;
  uint64_t pawn_hash, non_pawn_hash;
  const struct board_state *bstate;
  bstate = &state->board_states[state->search_ply];
  pawn_hash = bstate->pawn_hash;
  non_pawn_hash = bstate->non_pawn_hash;
  for (i = state->search_ply - 1; i >= 0; i--) {
    bstate = &state->board_states[i];
    if (bstate->pawn_hash == pawn_hash && bstate->non_pawn_hash == non_pawn_hash)
      return 1;
  }
  return 0;
}

static int
minimax(struct search_state *state, int depth, int alpha, int beta,
    Move *best_move, clock_t deadline)
{
  Move moves[256];
  int move_count, i, best_score, score, color;
  struct board_state *bstate;
  bstate = &state->board_states[state->search_ply];
  color = bstate->flags & BOARD_FLAG_WHITE_TO_PLAY ? 1 : 0;
  best_score = color ? -CHECKMATE_EVALUATION-1 : CHECKMATE_EVALUATION+1;
  move_count = generate_moves(state, moves) - moves;
  if (move_count == 0) {
    assert(best_move == NULL);
    return color ? -CHECKMATE_EVALUATION : CHECKMATE_EVALUATION;
  }
  for (i = 0; i < move_count; i++) {
    make_move(state, moves[i]);
    if (is_repetition(state))
      score = 0;
    else if (depth == 0)
      score = evaluate_board(state);
    else
      score = minimax(state, depth - 1, alpha, beta, NULL, deadline);
    unmake_move(state, moves[i]);
    if (state->search_ply < 4 && clock() > deadline) {
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
  depth = 1;
  move = 0;
  do {
    best_move = move;
    depth++;
    minimax(state, depth, -CHECKMATE_EVALUATION-1, CHECKMATE_EVALUATION+1, &move, deadline);
  } while(move);
  assert(best_move);
  /* TODO: handle case when no move found before deadline */
  return best_move;
}
