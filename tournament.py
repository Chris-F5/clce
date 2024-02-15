from clce import *
import chess
import chess.pgn

def play_game(white: CLCE, black: CLCE, move_time: int):
  moves = []
  board = chess.Board()
  while not board.outcome():
    if board.turn == chess.WHITE:
      move = white.find_move(board, move_time)
    else:
      move = black.find_move(board, move_time)
    moves.append(move)
    print(board.fen())
    print(move, flush=True)
    board.push(move)
  print(board.outcome())
  print(chess.Board().variation_san(moves), flush=True)
  return board.outcome()

play_game(CLCE('./clce'), CLCE('./clce'), 200)
