/*
 * The following must be included before chess.h :
 * #include <stdint.h>
 * #include <stddef.h>
 * #include <assert.h>
 * #include <stdlib.h>
 */

/* TYPE DEFINITIONS AND CONSTANTS */

#define MAX_FEN_SIZE (8 * 8 + 7 + 1 + 4 + 2 + 6 + 6 + 5)

/* promote pieces fit in 2 bits */
#define PIECE_TYPE_KNIGHT 0
#define PIECE_TYPE_BISHOP 1
#define PIECE_TYPE_ROOK   2
#define PIECE_TYPE_QUEEN  3
/* non-promote pieces */
#define PIECE_TYPE_PAWN   4
#define PIECE_TYPE_KING   5

#define COLOR_BLACK 0
#define COLOR_WHITE 1

typedef uint16_t BoardFlags;
#define BOARD_FLAG_WHITE_TO_PLAY      0x0001
#define BOARD_FLAG_WHITE_CASTLE_KING  0x0002
#define BOARD_FLAG_WHITE_CASTLE_QUEEN 0x0004
#define BOARD_FLAG_BLACK_CASTLE_KING  0x0008
#define BOARD_FLAG_BLACK_CASTLE_QUEEN 0x0010
#define BOARD_FLAGS_WHITE_CASTLE (BOARD_FLAG_WHITE_CASTLE_KING | BOARD_FLAG_WHITE_CASTLE_QUEEN)
#define BOARD_FLAGS_BLACK_CASTLE (BOARD_FLAG_BLACK_CASTLE_KING | BOARD_FLAG_BLACK_CASTLE_QUEEN)
typedef uint64_t Bitboard;
struct board {
  Bitboard type_bitboards[6];
  Bitboard color_bitboards[2];
  uint8_t mailbox[32]; /* 4-bits per piece */

  BoardFlags flags;
  int16_t en_passant_square;
  uint16_t halfmove_clock;
  uint16_t fullmove_clock;
};

#define SPECIAL_MOVE_NONE       0
#define SPECIAL_MOVE_PROMOTE    1
#define SPECIAL_MOVE_EN_PASSANT 2
#define SPECIAL_MOVE_CASTLING   3

#define WHITE_KING_CASTLE_BLOCKERS (set_bit(5) | set_bit(6))
#define WHITE_QUEEN_CASTLE_BLOCKERS (set_bit(1) | set_bit(2) | set_bit(3))
#define BLACK_KING_CASTLE_BLOCKERS (set_bit(61) | set_bit(62))
#define BLACK_QUEEN_CASTLE_BLOCKERS (set_bit(57) | set_bit(58) | set_bit(59))

/*
 * 0-5   destination square
 * 6-11  origin square
 * 12-13 special move
 * 14-15 promote piece type
 */
typedef uint16_t Move;

/* FUNCTION DECLARATIONS */

/* fen.c */
int parse_fen(struct board *board, const char *fen);
/*
void write_fen(const struct board *board, char *buf);
*/

/* utils.c */
extern const char *square_names[64];
extern const char piece_chars[];
void *xmalloc(size_t len);
void *xrealloc(void *p, size_t len);
void print_bitmap(uint64_t bitmap);
void print_move(Move move);
void print_board(const struct board *board);
void read_buffer(char *buffer, int len);

/* bitboards.c */
extern uint64_t knight_attack_table[64];
extern uint64_t king_attack_table[64];
void print_best_magics(void);
void init_bitboards(void);
uint64_t get_rook_attack_set(int rook_square, uint64_t blockers);
uint64_t get_bishop_attack_set(int bishop_square, uint64_t blockers);

/* move_gen.c */
Move *generate_moves(const struct board *board, Move *moves);
long perft(const struct board *board, int depth, int print);

/* make_move.c */
void make_move(struct board *board, Move move);

/* INLINE FUNCTIONS */

static inline uint64_t
random_uint64(void)
{
  return random() | random() << 32;
}

static inline int
count_bits(Bitboard b)
{
  int count;
  for (count = 0; b; count++, b &= b - 1);
  return count;
}
static inline Bitboard
lsb(Bitboard b)
{
  assert(b);
  return b & -b;
}
static inline Bitboard
pop_lsb(Bitboard *b)
{
  Bitboard bit;
  assert(*b);
  bit = *b & -*b;
  *b &= *b - 1;
  return bit;
}
static inline int
lss(Bitboard b)
{
  assert(b);
  return __builtin_ctzl(b);
}
static inline int
pop_lss(Bitboard *b)
{
  int square;
  assert(*b);
  square = __builtin_ctzl(*b);
  *b &= *b - 1;
  return square;
}
static inline int
check_square(int square)
{
  return square >= 0 && square < 64;
}
static inline Bitboard
set_bit(int square)
{
  return (uint64_t)1 << square;
}
static inline int
get_piece_type(const uint8_t *mailbox, int square)
{
  if (square % 2 == 0)
    return mailbox[square / 2] & 0x0f;
  else
    return mailbox[square / 2] >> 4;
}
static inline void
set_piece_type(uint8_t *mailbox, int square, int piece_type)
{
  assert((piece_type & 0x0f) == piece_type);
  if (square % 2 == 0) {
    mailbox[square / 2] &= 0xf0;
    mailbox[square / 2] |= piece_type;
  } else {
    mailbox[square / 2] &= 0x0f;
    mailbox[square / 2] |= piece_type << 4;
  }
}

static inline Move
basic_move(int origin, int dest)
{
  assert(dest >= 0 && dest < 64);
  assert(origin >= 0 && origin < 64);
  return dest | (origin << 6);
}
static inline Move
promote_move(int origin, int dest, int promote_piece_type)
{
  assert((dest >= 0 && dest < 8) || (dest >= 56 && dest < 64));
  assert((origin >= 8 && origin < 16) || (origin >= 48 && origin < 56));
  assert((promote_piece_type & 0x03) == promote_piece_type);
  return dest | (origin << 6) | (SPECIAL_MOVE_PROMOTE << 12)
    | (promote_piece_type << 14);
}
static inline Move
en_passant_move(int origin, int dest)
{
  assert(dest >= 0 && dest < 64);
  assert(origin >= 0 && origin < 64);
  return dest | (origin << 6) | (SPECIAL_MOVE_EN_PASSANT << 12);
}
static inline Move
castle_move(int color, int king_side)
{
  int dest, origin;
  origin = color ? 4 : 60;
  dest = origin + (king_side ? 2 : -2);
  return dest | (origin << 6) | (SPECIAL_MOVE_CASTLING << 12);
}
static inline int
get_move_origin(Move move)
{
  return (move >> 6) & 0x3f;
}
static inline int
get_move_dest(Move move)
{
  return move & 0x3f;
}
static inline int
get_move_special_type(Move move)
{
  return (move >> 12) & 0x03;
}
static inline int
get_move_promote_piece(Move move)
{
  return (move >> 14) & 0x03;
}
