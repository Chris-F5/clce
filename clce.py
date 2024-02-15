import chess
import subprocess

class CLCE:
  def __init__(self, binary: str):
    self.binary = binary
    self.process = subprocess.Popen([binary], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  def find_move(self, board: chess.Board, milliseconds: int):
    command = f'{board.fen()}:{milliseconds}\n'
    self.process.stdin.write(command.encode())
    self.process.stdin.flush()
    result = self.process.stdout.readline().decode().strip()
    move = chess.Move.from_uci(result)
    return move
  def close(self):
    self.process.stdin.close()
    self.process.stdout.close()
    self.process.stderr.close()
    self.process.terminate()

#board = chess.Board("5kr1/8/3K3p/7P/8/1n6/2qn4/8 w - - 18 61")
#e = CLCE('./clce')
#print(e.find_move(board, 200))
#e.close()
