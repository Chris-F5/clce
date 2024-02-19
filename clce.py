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

class CLCE:
  def __init__(self, binary: str, verbose: bool=False):
    self.binary = binary
    self.verbose = verbose
    self.proc = subprocess.Popen([binary], stdin=subprocess.PIPE,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    self.wait_ready()
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

  def go(self, board: chess.Board, seconds: float=4) -> chess.Move:
    milliseconds = (int)(seconds * 1000)
    self.send_command(f"go:{board.fen()}:{milliseconds}")
    result = self.wait_line(seconds + 2)
    move = chess.Move.from_uci(result)
    return move
  def perft(self, board: chess.Board, depth: int) -> Dict[chess.Move, int]:
    self.send_command(f"perft:{board.fen()}:{depth}")
    table = {}
    line = self.wait_line(16)
    if line.strip() == "":
      return {}
    for pair in line.split(" "):
      move,count = pair.split(":")
      table[chess.Move.from_uci(move)] = int(count)
    return table

  def close(self):
    self.proc.stdin.close()
    self.proc.stdout.close()
    self.proc.terminate()
