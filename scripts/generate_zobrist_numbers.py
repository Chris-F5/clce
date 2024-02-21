import random

random.seed(982)

s = 'uint64_t zobrist_piece_numbers[2 * 6 * 64] = {'
for i in range(0, 2 * 6 * 64):
  n = random.randint(0, 2**64)
  if i % 3 == 0:
    s += f'\n  {n}U, '
  elif i % 3 == 1:
    s += f'{n}U, '
  elif i % 3 == 2:
    s += f'{n}U,'
s += '\n};\n'
print(s)

s = 'uint64_t zobrist_castling_numbers[16] = {'
for i in range(0, 16):
  n = random.randint(0, 2**64)
  if i % 3 == 0:
    s += f'\n  {n}U, '
  elif i % 3 == 1:
    s += f'{n}U, '
  elif i % 3 == 2:
    s += f'{n}U,'
s += '\n};\n'
print(s)

s = 'uint64_t zobrist_en_passant_numbers[8] = {'
for i in range(0, 8):
  n = random.randint(0, 2**64)
  if i % 3 == 0:
    s += f'\n  {n}U, '
  elif i % 3 == 1:
    s += f'{n}U, '
  elif i % 3 == 2:
    s += f'{n}U,'
s += '\n};\n'
print(s)

print(f'uint64_t zobrist_black_number = {random.randint(0, 2**64)}U;')
