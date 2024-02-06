#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include "chess.h"
#include "magic_numbers.h"
#include "bit_utils.h"

static uint64_t rook_relevance_masks[64];
static uint64_t bishop_relevance_masks[64];

uint64_t knight_attack_table[64];
uint64_t king_attack_table[64];

static void
init_relevance_masks(void)
{
  int i, j;
  for (i = 0; i < 64; i++) {
    rook_relevance_masks[i] = 0;
    /* vertical */
    for (j = i % 8 + 8; j  < 8 * 7; j += 8) {
      if (j == i)
        continue;
      rook_relevance_masks[i] |= (uint64_t)1 << j;
    }
    /* horizontal */
    for (j = i - i % 8 + 1; j < i - i % 8 + 7; j++) {
      if (j == i)
        continue;
      rook_relevance_masks[i] |= (uint64_t)1 << j;
    }
  }
  for (i = 0; i < 64; i++) {
    bishop_relevance_masks[i] = 0;
    /* north east */
    if (i % 8 < 6)
      for (j = i + 9; j < 8 * 7 && j % 8 != 7; j += 9)
        bishop_relevance_masks[i] |= (uint64_t)1 << j;
    /* south west */
    if (i % 8 > 1)
      for (j = i - 9; j > 7 && j % 8; j -= 9)
        bishop_relevance_masks[i] |= (uint64_t)1 << j;
    /* north west */
    if (i % 8 > 1)
      for (j = i + 7; j < 8 * 7 && j % 8; j += 7)
        bishop_relevance_masks[i] |= (uint64_t)1 << j;
    /* south east */
    if (i % 8 < 6)
      for (j = i - 7; j > 7 && j % 8 != 7; j -= 7)
        bishop_relevance_masks[i] |= (uint64_t)1 << j;
  }
}

static void
init_knight_attack_table(void)
{
  int i, j;
  const int moves[] = {-17, -15, -10, -6, 6, 10, 15, 17};
  for (i = 0; i < 64; i++) {
    knight_attack_table[i] = 0;
    for (j = 0; j < sizeof(moves) / sizeof(moves[0]); j++)
      if (abs(i % 8 - (i + moves[j]) % 8) <= 2 && check_square(i + moves[j]))
        knight_attack_table[i] |= set_bit(i + moves[j]);
  }
}

static void
init_king_attack_table(void)
{
  int i, j;
  const int moves[] = {-9, -8, -7, -1, 1, 7, 8, 9};
  for (i = 0; i < 64; i++) {
    king_attack_table[i] = 0;
    for (j = 0; j < sizeof(moves) / sizeof(moves[0]); j++)
      if (abs(i % 8 - (i + moves[j]) % 8) <= 1 && check_square(i + moves[j]))
        king_attack_table[i] |= set_bit(i + moves[j]);
  }
}

static uint64_t
primitive_bishop_attack_squares(int square, uint64_t blockers)
{
  int i;
  uint64_t attacks;
  attacks = 0;
  /* north east */
  for (i = square + 9; i < 64 && i % 8 != 0; i += 9) {
    attacks |= (uint64_t)1 << i;
    if (blockers & ((uint64_t)1 << i))
      break;
  }
  /* south east */
  for (i = square - 7; i >= 0 && i % 8 != 0; i -= 7) {
    attacks |= (uint64_t)1 << i;
    if (blockers & ((uint64_t)1 << i))
      break;
  }
  /* north west */
  for (i = square + 7; i < 64 && i % 8 != 7; i += 7) {
    attacks |= (uint64_t)1 << i;
    if (blockers & ((uint64_t)1 << i))
      break;
  }
  /* south west */
  for (i = square - 9; i >= 0 && i % 8 != 7; i -= 9) {
    attacks |= (uint64_t)1 << i;
    if (blockers & ((uint64_t)1 << i))
      break;
  }
  return attacks;
}

static uint64_t
primitive_rook_attack_squares(int square, uint64_t blockers)
{
  int i;
  uint64_t attacks;
  attacks = 0;
  /* north */
  for (i = square + 8; i < 64; i += 8) {
    attacks|= (uint64_t)1 << i;
    if (blockers & ((uint64_t)1 << i))
      break;
  }
  /* south */
  for (i = square - 8; i >= 0; i -= 8) {
    attacks |= (uint64_t)1 << i;
    if (blockers & ((uint64_t)1 << i))
      break;
  }
  /* east */
  for (i = square + 1; i % 8 != 0; i++) {
    attacks |= (uint64_t)1 << i;
    if (blockers & ((uint64_t)1 << i))
      break;
  }
  /* west */
  for (i = square - 1; i % 8 != 7 && i >= 0; i--) {
    attacks |= (uint64_t)1 << i;
    if (blockers & ((uint64_t)1 << i))
      break;
  }
  return attacks;
}

static uint64_t
generate_mock_blockers(uint64_t mask, int index)
{
  uint64_t blockers, bit;
  blockers = 0;
  while (index) {
    if (mask == 0) {
      fprintf(stderr, "failed to generate blockers: index too high\n");
      exit(1);
    }
    bit = pop_lsb(&mask);
    if (index & 1)
      blockers |= bit;
    index >>= 1;
  }
  return blockers;
}

static int
init_magic_square(int square, int rook, uint64_t magic, int bits,
    uint64_t *attack_table)
{
  uint64_t relevance_mask, blockers, hash, attacks;
  int i, relevant_square_count, table_size;
  relevance_mask = rook
    ? rook_relevance_masks[square]
    : bishop_relevance_masks[square];
  relevant_square_count = count_bits(relevance_mask);
  memset(attack_table, 0, ((uint64_t)1 << bits) * sizeof(uint64_t));
  table_size = 0;
  for (i = 0; i < ((uint64_t)1 << relevant_square_count); i++) {
    blockers = generate_mock_blockers(relevance_mask, i);
    hash = (blockers * magic) >> (64 - bits);
    attacks = rook
      ? primitive_rook_attack_squares(square, blockers)
      : primitive_bishop_attack_squares(square, blockers);
    if (hash >= table_size)
      table_size = hash + 1;
    if (attack_table[hash] == 0)
      attack_table[hash] = attacks;
    else if (attack_table[hash] != attacks)
      return 0;
  }
  return table_size;
}

static void
print_best_magic(int square, int rook, int reduction_attempts,
    int optimisation_attempts, int *table_offset)
{
  int bits, table_size, best_table_size;
  uint64_t magic, best_magic;
  uint64_t *mock_table_memory;
  bits = 14;
  mock_table_memory = xmalloc( ((uint64_t)1 << bits) * sizeof(uint64_t) );
  for(; reduction_attempts ; reduction_attempts--) {
    magic = random_uint64() & random_uint64() & random_uint64();
    if ( (table_size = init_magic_square(square, rook, magic, bits, mock_table_memory)) ) {
      best_table_size = table_size;
      best_magic = magic;
      bits--;
    }
  }
  if (best_table_size == 0) {
    printf("%s %d NO MAGIC FOUND\n", rook ? "rook" : "bishop", square);
    return;
  }
  bits += 1;
  for (; optimisation_attempts; optimisation_attempts--) {
    magic = random_uint64() & random_uint64() & random_uint64();
    table_size = init_magic_square(square, rook, magic, bits, mock_table_memory);
    if (table_size && table_size < best_table_size) {
      best_table_size = table_size;
      best_magic = magic;
    }
  }
  printf("  [%3d] = {0x%016lx, %2d, %d},\n", rook ? 64 + square : square,
      best_magic, bits, *table_offset);
  fflush(stdout);
  *table_offset += best_table_size;
  free(mock_table_memory);
}

void
print_best_magics(void)
{
  int square, table_offset;
  init_relevance_masks();
  srand(time(NULL));
  printf("\
struct magic_square {\n\
  uint64_t magic; \n\
  int bits;\n\
  int attack_table_offset;\n\
} magic_squares[] = {\n");
  printf("  /* bishops */\n");
  for (square = 0; square < 64; square++)
    print_best_magic(square, 0, 20000, 20000, &table_offset);
  printf("  /* rooks */\n");
  for (square = 0; square < 64; square++)
    print_best_magic(square, 1, 20000, 20000, &table_offset);
  printf("\
};\n\
uint64_t attack_table[%d];\n", table_offset);
}

void
init_bitboards(void)
{
  int i;
  init_relevance_masks();
  memset(attack_table, 0, sizeof(attack_table));
  for (i = 0; i < 128; i++)
    assert(init_magic_square(i % 64, i > 63, magic_squares[i].magic,
        magic_squares[i].bits,
        attack_table + magic_squares[i].attack_table_offset));
  init_knight_attack_table();
  init_king_attack_table();
}

uint64_t
get_rook_attack_set(int rook_square, uint64_t blockers)
{
  struct magic_square *magic_square;
  uint64_t relevant_blockers, hash;
  magic_square = &magic_squares[rook_square + 64];
  relevant_blockers = blockers & rook_relevance_masks[rook_square];
  hash = (relevant_blockers * magic_square->magic) >> (64 - magic_square->bits);
  return attack_table[magic_square->attack_table_offset + hash];
}

uint64_t
get_bishop_attack_set(int bishop_square, uint64_t blockers)
{
  struct magic_square *magic_square;
  uint64_t relevant_blockers, hash;
  magic_square = &magic_squares[bishop_square];
  relevant_blockers = blockers & bishop_relevance_masks[bishop_square];
  hash = (relevant_blockers * magic_square->magic) >> (64 - magic_square->bits);
  return attack_table[magic_square->attack_table_offset + hash];
}
