from typing import Dict
import chess
import subprocess
import select
import sys
import time

def recv(stream, seconds: float) -> str | None:
    start_time = time.time()
    while True:
      if time.time() > start_time + seconds:
        return None
      ready, _, _ = select.select([stream], [], [], 0.01)
      if ready:
        return stream.readline().strip()

class Engine:
  def restart(self):
    pass
  def name(self):
    raise NotImplementedError()
  def close(self):
    pass
  def go(self, board: chess.Board):
    raise NotImplementedError()
  def perft(self, board: chess.Board, depth: int) -> Dict[chess.Move, int]:
    raise NotImplementedError()

class CLCE(Engine):
  def __init__(self, binary: str, default_move_time :float = 4, verbose: bool=False):
    self.binary = binary
    self.verbose = verbose
    self.default_move_time = default_move_time
    self.start_binary()
  def name(self):
    return self.binary
  def start_binary(self):
    self.proc = subprocess.Popen([self.binary], stdin=subprocess.PIPE,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    self.wait_ready()
  def restart(self):
    self.dump_stderr()
    self.close()
    self.start_binary()
  def close(self):
    self.proc.stdin.close()
    self.proc.stdout.close()
    self.proc.terminate()
  def dump_stderr(self):
    while line := recv(self.proc.stderr, 0.1):
      sys.stderr.write(line + "\n")
  def wait_line(self, seconds: float) -> str:
    line = recv(self.proc.stdout, seconds)
    if line == None:
      self.dump_stderr()
      sys.stderr.write(f"{self.binary} timed out.\n")
      self.close()
      exit(1)
    if self.verbose: print(f">{line}")
    return line
  def wait_ready(self):
    while line := self.wait_line(2):
      if line == "READY":
        break
  def send_command(self, cmd: str):
    if self.verbose: print(f">{cmd}")
    self.proc.stdin.write(cmd+"\n")
    self.proc.stdin.flush()

  def go(self, board: chess.Board, seconds: float=None) -> chess.Move:
    if seconds == None:
      seconds = self.default_move_time
    milliseconds = (int)(seconds * 1000)
    self.send_command(f"go:{board.fen()}:{milliseconds}:_")
    result = self.wait_line(seconds + 2)
    move = chess.Move.from_uci(result)
    return move
  def perft(self, board:chess.Board, depth:int, quiet:bool=False) -> Dict[chess.Move, int]:
    flag = 'q' if quiet else '_'
    self.send_command(f"perft:{board.fen()}:{depth}:{flag}")
    table = {}
    line = self.wait_line(60)
    if line.strip() == "":
      return {}
    for pair in line.split(" "):
      move,count = pair.split(":")
      table[chess.Move.from_uci(move)] = int(count)
    return table
