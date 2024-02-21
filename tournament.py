from clce import *
import chess
import chess.pgn

def play_game(board: chess.Board, white: CLCE, black: CLCE) -> chess.Outcome:
  moves = []
  board_copy = board.copy()
  while not board.outcome():
    if board.turn == chess.WHITE:
      move = white.go(board)
    else:
      move = black.go(board)
    moves.append(move)
    board.push(move)
  print(board.outcome())
  print(board_copy.variation_san(moves), flush=True)
  return board.outcome()

def play_games(pgn_fname: str, old: CLCE, new: CLCE):
  games = open(pgn_fname, 'r')
  game_count = 0
  while game := chess.pgn.read_game(games):
    game_count += 1
  games.seek(0)
  results = []
  i = 0
  while game := chess.pgn.read_game(games):
    i += 1
    print(f"board {i}/{game_count}")
    board = game.board()
    for move in list(game.mainline_moves())[:20]:
      board.push(move)
    old.restart()
    new.restart()
    white_outcome = play_game(board.copy(), new, old)
    old.restart()
    new.restart()
    black_outcome = play_game(board.copy(), old, new)
    results.append((white_outcome, black_outcome))
  return results

def print_results(results: []):
  wins = draws = losses = 0
  for white_outcome, black_outcome in results:
    wins += 1 if white_outcome.winner == chess.WHITE else 0
    losses += 1 if white_outcome.winner == chess.BLACK else 0
    draws += 1 if white_outcome.winner == None else 0
    wins += 1 if black_outcome.winner == chess.BLACK else 0
    losses += 1 if black_outcome.winner == chess.WHITE else 0
    draws += 1 if black_outcome.winner == None else 0
  print(f"wins {wins}")
  print(f"draws {draws}")
  print(f"losses {losses}")

old = CLCE("./clce", 0.5, verbose=False)
new = CLCE("./clce", 0.5, verbose=False)
results = play_games("SaintLouis2023.pgn", old, new)
print_results(results)
