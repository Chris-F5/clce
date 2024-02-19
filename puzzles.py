import chess
from clce import CLCE

binary = "./clce"
puzzles = "lichess_db_puzzle.csv"
e = CLCE(binary, verbose=False)

think_time=1
count = 200
results = []
f = open(puzzles, "r")
f.readline()
for line in f:
  id, fen, moves, rating = line.strip().split(",")[:4]
  board = chess.Board(fen)
  moves = [chess.Move.from_uci(uci) for uci in moves.split(" ")]
  board.push(moves[0])
  fen = board.fen()
  color = board.turn
  chosen = e.go(board, think_time)
  board.push(chosen)
  checkmate = board.outcome() and board.outcome().winner == color
  correct = (chosen == moves[1] or checkmate)
  print(f"{len(results)}/{count} {correct}, rating {rating}, expected {moves[1]}, suggested {chosen}, fen {fen}")
  results.append((id, fen, int(rating), correct))
  if len(results) == count:
    break

correct = sum(1 if r[3] else 0 for r in results)
mean_rating = sum(r[2] for r in results)/len(results)
print(f"correct {correct/count}, mean rating {mean_rating}")

e.close()
