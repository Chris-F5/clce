from clce import CLCE, Engine
import chess
import logging
import json

class EngineTest:
  def configure(self):
    pass
  def run_test(self, engine: Engine) -> {}:
    raise NotImplementedError()
  def print_results(results: {}):
    pass

class PerftTest(EngineTest):
  def configure(self):
    self.perfts = [
      {'board': "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 'depth': 4, 'value': 197281},
      {'board': "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 'depth': 4, 'value': 4085603},
      {'board': "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 'depth': 5, 'value': 674624},
      {'board': "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 'depth': 5, 'value': 15833292},
      {'board': "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 'depth': 5, 'value': 15833292},
      {'board': "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 'depth': 4, 'value': 2103487},
      {'board': "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 'depth': 4, 'value': 3894594},
    ]
  def run_test(self, engine: Engine):
    for i,perft in enumerate(self.perfts):
      logging.info(f"perft {i+1}/{len(self.perfts)}")
      board = chess.Board(perft['board'])
      result = sum(engine.perft(board, perft['depth']).values())
      assert(result == perft['value'])
    return self.perfts

class PuzzleTest(EngineTest):
  def __init__(self, database: str, count: int):
    self.database = database
    self.count = count
  def configure(self):
    self.puzzles = []
    f = open(self.database, 'r')
    f.readline()
    for i,line in enumerate(f):
      if i == self.count: break
      id, fen, uci_moves, rating = line.strip().split(",")[:4]
      board = chess.Board(fen)
      moves = [chess.Move.from_uci(uci) for uci in uci_moves.split(" ")]
      self.puzzles.append({'board': board, 'moves': moves, 'rating': rating})
    f.close()
  def run_test(self, engine: Engine):
    results = {'puzzles': []}
    for i,puzzle in enumerate(self.puzzles):
      board = puzzle['board']
      board.push(puzzle['moves'][0])
      color = board.turn
      move = engine.go(board)
      board.push(move)
      success = bool(move == puzzle['moves'][1] or (board.outcome() and board.outcome().winner == color))
      results['puzzles'].append({'fen': puzzle['board'].fen(), 'rating': puzzle['rating'], 'success': success})
      logging.info(f"puzzle {i+1}/{len(self.puzzles)} {'success' if success else 'fail'}")
    return results
  def print_results(results):
    puzzles = results['puzzles']
    successes = sum(p['success'] for p in puzzles)
    print(f"Succeeded {successes}/{len(puzzles)} puzzles.")

def run_tests(engine: Engine, tests: []) -> []:
  fail = 0
  logging.info(f"TESTING '{engine.name()}'...")
  results = {}
  for test in tests:
    name = test.__class__.__name__
    assert(name not in results)
    logging.info(f"PREPARING {name}...")
    test.configure()
    logging.info(f"RUNNING {name}...")
    result = None
    try:
      result = test.run_test(engine)
      logging.info(f"TEST {name} SUCCESS")
    except AssertionError:
      logging.error(f"TEST {name} FAILED")
      fail += 1
    results[name] = result
  logging.info("TESTING DONE")
  if fail > 0:
    logging.info(f"{fail} tests failed.")
  else:
    logging.info(f"All tests succeeded.")
  return results

def save_output(output: str, results):
  f = open(output, 'w')
  f.write(json.dumps(results, indent=2))
  f.close()

def summarise_results(results):
  for test_name,result in results.items():
    test_class = globals()[test_name]
    test_class.print_results(result)

logging.basicConfig(level=logging.INFO)
binary = "./clce"
output = "./test_result"
tests = [
  PerftTest(),
  PuzzleTest("./db/lichess_db_puzzle.csv", 10),
]

engine = CLCE(binary, 1, verbose=False)
test_outcome = run_tests(engine, tests)
engine.close()
save_output(output, test_outcome)
print(f"Results saved to '{output}'.")
summarise_results(test_outcome)
