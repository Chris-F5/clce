import logging, json, sys, getopt
import chess
import chess.pgn
from clce import CLCE, Engine
from stockfish import Stockfish

class EngineTest:
  def configure(self):
    pass
  def run_test(self, engine: Engine) -> {}:
    raise NotImplementedError()
  def print_results(results: {}):
    pass

class PerftTest(EngineTest):
  def legal_moves(board:chess.Board, quiet:bool):
    if quiet:
      return set(move for move in board.legal_moves \
           if not board.is_capture(move) \
          and not move.promotion \
          or board.piece_type_at(move.from_square) == chess.KING)
    else:
      return set(board.legal_moves)
  def perft(board:chess.Board, depth:int, quiet=False):
    if depth == 0:
      return 1
    count = 0
    for move in PerftTest.legal_moves(board, quiet):
      board.push(move)
      count += PerftTest.perft(board, depth-1, quiet)
      board.pop()
    return count
  def locate_perft_error(engine:CLCE, board:chess.Board, depth:int, quiet:bool):
    if depth == 0:
      return
    reported = engine.perft(board, depth, quiet)
    reported_moves = set(reported.keys())
    true_moves =  PerftTest.legal_moves(board, quiet)
    if reported_moves != true_moves:
      print("PERFT ERROR:")
      if quiet: print("  quiet moves only")
      print(f"  position: {board.fen()}")
      print(f"  false negative: {true_moves.difference(reported_moves)}")
      print(f"  false positive: {reported_moves.difference(true_moves)}")
    for move in true_moves:
      board.push(move)
      if PerftTest.perft(board, depth-1, quiet) != reported[move]:
        logging.warning("perft discontinuity detected at {board.fen()}")
        PerftTest.locate_perft_error(engine, board, depth-1, quiet)
      board.pop()
    
  def configure(self):
    self.perfts = [
      {'board': "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 'depth': 4, 'value': 197281},
      {'board': "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 'depth': 4, 'value': 4085603},
      {'board': "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 'depth': 5, 'value': 674624},
      {'board': "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 'depth': 5, 'value': 15833292},
      {'board': "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 'depth': 5, 'value': 15833292},
      {'board': "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 'depth': 4, 'value': 2103487},
      {'board': "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 'depth': 4, 'value': 3894594},
      {'board': '2r2rk1/pbq1bppp/8/8/2p1N3/P1Bn2P1/2Q2PBP/1R3RK1 w - - 4 24', 'depth': 4, 'value': 3188425, 'quiet': True}
    ]
  def run_test(self, engine: CLCE):
    for i,perft in enumerate(self.perfts):
      board = chess.Board(perft['board'])
      depth = perft['depth']
      expected = perft['value']
      quiet = perft.get('quiet', False)
      logging.info(f"perft {i+1}/{len(self.perfts)}")
      result = sum(engine.perft(board, depth, quiet).values())
      if result != expected:
        logging.warning("perft failed, locating error...")
        PerftTest.locate_perft_error(engine, board, depth, quiet)
        raise AssertionError()
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
      fen = board.fen()
      color = board.turn
      move = engine.go(board)
      board.push(move)
      success = bool(move == puzzle['moves'][1] or (board.outcome() and board.outcome().winner == color))
      results['puzzles'].append({'fen': fen, 'rating': puzzle['rating'], 'success': success})
      logging.info(f"puzzle {i+1}/{len(self.puzzles)} {'success' if success else 'fail'}")
    return results
  def print_results(results):
    puzzles = results['puzzles']
    successes = sum(p['success'] for p in puzzles)
    print(f"Succeeded {successes}/{len(puzzles)} puzzles.")

class StockfishTest(EngineTest):
  def __init__(self, game_count, pgn_fname, elo):
    self.game_count = game_count
    self.pgn_fname = pgn_fname
    self.elo = elo
  def configure(self):
    self.stockfish = Stockfish()
    self.stockfish.set_elo_rating(self.elo)
    self.start_boards = []
    pgn_file = open(self.pgn_fname, 'r')
    for i in range(self.game_count):
      game = chess.pgn.read_game(pgn_file)
      assert game != None
      board = game.board()
      for move in list(game.mainline_moves())[:20]:
        board.push(move)
      self.start_boards.append(board)
    pgn_file.close()
  def play_game(self, start_board, engine: Engine, engine_color):
    moves = []
    board = start_board.copy()
    while not board.outcome():
      if board.turn == engine_color:
        move = engine.go(board)
      else:
        self.stockfish.set_fen_position(board.fen())
        uci = self.stockfish.get_best_move()
        move = chess.Move.from_uci(uci)
      moves.append(move)
      board.push(move)
    pgn = "[Variant \"From Position\"]\n"
    pgn += f"[FEN \"{start_board.fen()}\"]\n"
    pgn += start_board.variation_san(moves) + "\n"
    return board.outcome(),pgn
  def run_test(self, engine: Engine):
    results = {'games': []}
    for i,start_board in enumerate(self.start_boards):
      logging.info(f"stockfish game {i+1}/{self.game_count}")
      outcome,san = self.play_game(start_board, engine, chess.WHITE)
      results['games'].append({'won': outcome.winner == chess.WHITE, 'pgn':san})
    return results
  def print_results(results):
    print(results)

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

def die_usage():
  print("test.py -mode {fast|slow}")
  exit(1)


logging.basicConfig(level=logging.INFO)
binary = "./clce"
output = "./test_result"
tests = None
fast_tests = [
  PerftTest(),
  PuzzleTest("./db/lichess_db_puzzle.csv", 5),
]
game_tests = [
  StockfishTest(1, "./db/SaintLouis2023.pgn", 1000),
]
slow_tests = [
  PerftTest(),
  PuzzleTest("./db/lichess_db_puzzle.csv", 100),
]
verbose = False

try:
  opts, args = getopt.getopt(sys.argv[1:], "m:e:o:v", ["mode="])
except getopt.GetoptError:
  die_usage()
for opt, arg in opts:
  if opt in ("-m", "--mode"):
    if arg == "fast":
      tests = fast_tests
    elif arg == "slow":
      tests = slow_tests
    elif arg == "games":
      tests = game_tests
    else:
      die_usage()
  elif opt in ("-e"):
    binary = arg
  elif opt in ("-o"):
    output = arg
  elif opt in ("-v"):
    verbose = True
if tests == None:
  die_usage()

engine = CLCE(binary, 0.1, verbose=verbose)
test_outcome = run_tests(engine, tests)
engine.close()
save_output(output, test_outcome)
print(f"Results saved to '{output}'.")
summarise_results(test_outcome)
