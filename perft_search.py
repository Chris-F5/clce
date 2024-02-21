# This file is usefull for debugging move generation. If a test finds a perft
# function fail. Then perft_search can be used to locate the errors.

from clce import CLCE
import chess

def perft(board: chess.Board, depth: int) -> int:
  if depth == 0:
    return 1
  count = 0
  for move in board.legal_moves:
    board.push(move)
    count += perft(board, depth-1)
    board.pop()
  return count

def perft_search(e: CLCE, board: chess.Board, depth: int):
  if depth == 0:
    return
  result = e.perft(board, depth)
  true = set(board.legal_moves)
  found = set(result.keys())
  if found != true:
    print("PERFT FAIL:")
    print(f"  position: {board.fen()}")
    print(f"  false negative: {true.difference(found)}")
    print(f"  false positive: {found.difference(true)}")
    return False
  for move in true:
    board.push(move)
    if perft(board, depth-1) != result[move]:
      print(f"discontinuity {move.uci()} in {board.fen()}")
      perft_search(e, board, depth-1)
    board.pop()
