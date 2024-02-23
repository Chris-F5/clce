from clce import CLCE, Engine
import chess
import logging

class EngineTest:
  def configure(self):
    pass
  def name(self):
    raise NotImplementedError()
  def run_test(self, engine: Engine) -> {}:
    raise NotImplementedError()
  def print_results(self, results: {}):
    pass

class PerftTest(EngineTest):
  def name(self):
    return "perft"
  def configure(self):
    self.perfts = [
      {'board': chess.Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"), 'depth': 4, 'expected': 197281},
      {'board': chess.Board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"), 'depth': 4, 'expected': 4085603},
      {'board': chess.Board("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"), 'depth': 5, 'expected': 674624},
      {'board': chess.Board("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"), 'depth': 5, 'expected': 15833292},
      {'board': chess.Board("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1"), 'depth': 5, 'expected': 15833292},
      {'board': chess.Board("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"), 'depth': 4, 'expected': 2103487},
      {'board': chess.Board("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"), 'depth': 4, 'expected': 3894594},
    ]
  def run_test(self, engine: Engine):
    for i,perft in enumerate(self.perfts):
      logging.info(f"perft {i+1}/{len(self.perfts)}")
      result = sum(engine.perft(perft['board'], perft['depth']).values())
      assert(result == perft['expected'])
    return {}

class PuzzleTest(EngineTest):
  def __init__(self, database: str, count: int):
    self.database = database
    self.count = count
  def name(self):
    return "puzzle"
  def configure(self):
    self.puzzles = []
    f = open(self.database, 'r')
    f.readline()
    for i,line in enumerate(f):
      if i == self.count: break
      id, fen, uci_moves, rating = line.strip().split(",")[:4]
      board = chess.Board(fen)
      moves = [chess.Move.from_uci(uci) for uci in uci_moves.split(" ")]
      self.puzzles.append({'board': board, 'moves': moves})
    f.close()
  def run_test(self, engine: Engine):
    results = {'puzzles': []}
    for i,puzzle in enumerate(self.puzzles):
      board = puzzle['board']
      board.push(puzzle['moves'][0])
      color = board.turn
      move = engine.go(board)
      board.push(move)
      success = (move == puzzle['moves'][1] or (board.outcome() and board.outcome().winner == color))
      results['puzzles'].append({'puzzle': puzzle, 'success': success})
      logging.info(f"puzzle {i+1}/{len(self.puzzles)} {'success' if success else 'fail'}")
    return results
  def print_results(self, results):
    puzzles = results['puzzles']
    successes = sum(bool(p['success']) for p in puzzles)
    print(f"Succeeded {successes}/{len(puzzles)} puzzles.")

def run_tests(engine: Engine, tests: []) -> []:
  fail = 0
  logging.info(f"TESTING '{engine.name()}'...")
  results = []
  for test in tests:
    assert(test.name() not in results)
    logging.info(f"PREPARING {test.name()}...")
    test.configure()
    logging.info(f"RUNNING {test.name()}...")
    result = None
    try:
      result = test.run_test(engine)
      logging.info(f"TEST {test.name()} SUCCESS")
    except AssertionError:
      logging.error(f"TEST {test.name()} FAILED")
      fail += 1
    results.append(result)
  logging.info("TESTING DONE")
  if fail > 0:
    logging.info(f"{fail} tests failed.")
  else:
    logging.info(f"All tests succeeded.")
  return results

logging.basicConfig(level=logging.INFO)
binary = "./clce"
engine = CLCE(binary, 1, verbose=False)
tests = [
  PerftTest(),
  PuzzleTest("./db/lichess_db_puzzle.csv", 10),
]

test_outcome = run_tests(engine, tests)
for test,result in zip(tests, test_outcome):
  test.print_results(result)

engine.close()
