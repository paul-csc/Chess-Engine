#pragma once

#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>

namespace Zugzwang {

using Bitboard = uint64_t;
using Key = uint64_t;

constexpr const char* START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
constexpr int MAX_MOVES = 256;
constexpr int MAX_PLIES = 2048;

// clang-format off
enum PieceType : int8_t {
    NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    ALL_PIECES = 0,
    PIECE_TYPE_NB = 8
};

enum Piece : int8_t {
    NO_PIECE,
    W_PAWN = PAWN,     W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = PAWN + 8, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NB = 16
};

enum Color : int8_t { WHITE, BLACK, COLOR_NB = 2 };

enum CastlingRights : int8_t {
    NO_CASTLING,
    WHITE_OO,
    WHITE_OOO = WHITE_OO << 1,
    BLACK_OO  = WHITE_OO << 2,
    BLACK_OOO = WHITE_OO << 3,

    KING_SIDE      = WHITE_OO | BLACK_OO,
    QUEEN_SIDE     = WHITE_OOO | BLACK_OOO,
    WHITE_CASTLING = WHITE_OO | WHITE_OOO,
    BLACK_CASTLING = BLACK_OO | BLACK_OOO,
    ANY_CASTLING   = WHITE_CASTLING | BLACK_CASTLING,

    CASTLING_RIGHT_NB = 16
};

enum Square : int8_t {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_NONE, SQUARE_NB = 64
};
// clang-format on

enum Direction : int {
    NORTH = 8,
    EAST = 1,
    SOUTH = -NORTH,
    WEST = -EAST,

    NORTH_EAST = NORTH + EAST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST,
    NORTH_WEST = NORTH + WEST
};

enum File : int8_t { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB };

enum Rank : int8_t { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB };

#define ENABLE_INCR_OPERATORS_ON(T) \
    inline T& operator++(T& d) {    \
        return d = T(int(d) + 1);   \
    }                               \
    inline T& operator--(T& d) {    \
        return d = T(int(d) - 1);   \
    }

ENABLE_INCR_OPERATORS_ON(PieceType)
ENABLE_INCR_OPERATORS_ON(Square)
ENABLE_INCR_OPERATORS_ON(File)
ENABLE_INCR_OPERATORS_ON(Rank)

#undef ENABLE_INCR_OPERATORS_ON

#ifdef DEBUG
#define ASSERT(n)                                                                      \
    do {                                                                               \
        if (!(n)) {                                                                    \
            std::cout << "Assertion failed: " << #n << "\n";                           \
            std::cout << "In file: " << __FILE__ << ", at line: " << __LINE__ << "\n"; \
            std::cin.get();                                                            \
            std::exit(1);                                                              \
        }                                                                              \
    } while (0)
#else
#define ASSERT(n) ((void)0)
#endif

constexpr Direction operator+(Direction d1, Direction d2) {
    return Direction(int(d1) + int(d2));
}
constexpr Direction operator*(int i, Direction d) {
    return Direction(i * int(d));
}

// Additional operators to add a Direction to a Square
constexpr Square operator+(Square s, Direction d) {
    return Square(int(s) + int(d));
}
constexpr Square operator-(Square s, Direction d) {
    return Square(int(s) - int(d));
}
inline Square& operator+=(Square& s, Direction d) {
    return s = s + d;
}
inline Square& operator-=(Square& s, Direction d) {
    return s = s - d;
}

// Toggle color
constexpr Color operator~(Color c) {
    return Color(c ^ BLACK);
}

constexpr Square MakeSquare(File f, Rank r) {
    return Square((r << 3) + f);
}

constexpr Piece MakePiece(Color c, PieceType pt) {
    return Piece((c << 3) + pt);
}

constexpr PieceType TypeOf(Piece pc) {
    return PieceType(pc & 7);
}

inline Color ColorOf(Piece pc) {
    ASSERT(pc != NO_PIECE);
    return Color(pc >> 3);
}

constexpr bool IsOk(Square s) {
    return s >= SQ_A1 && s <= SQ_H8;
}

constexpr File FileOf(Square s) {
    return File(s & 7);
}

constexpr Rank RankOf(Square s) {
    return Rank(s >> 3);
}

constexpr Square RelativeSquare(Color c, Square s) {
    return Square(s ^ (c * 56));
}

constexpr Rank RelativeRank(Color c, Rank r) {
    return Rank(r ^ (c * 7));
}

constexpr Rank RelativeRank(Color c, Square s) {
    return RelativeRank(c, RankOf(s));
}

constexpr Direction PawnPush(Color c) {
    return c == WHITE ? NORTH : SOUTH;
}

inline std::ostream& operator<<(std::ostream& os, const Square& sq) {
    os << char('a' + FileOf(sq)) << char('1' + RankOf(sq));
    return os;
}

enum MoveType { NORMAL, PROMOTION = 1 << 14, EN_PASSANT = 2 << 14, CASTLING = 3 << 14 };

// 16 bits:
// bit  0- 5: destination square (from 0 to 63)
// bit  6-11: origin square (from 0 to 63)
// bit 12-13: promotion piece type - 2 (from KNIGHT-2 to QUEEN-2)
// bit 14-15: special move flag: promotion (1), en passant (2), castling (3)
class Move {
    uint16_t data;

  public:
    Move() = default;
    constexpr explicit Move(uint16_t d) : data(d) {}

    constexpr Move(Square from, Square to) : data((from << 6) + to) {}

    template <MoveType T>
    static constexpr Move Make(Square from, Square to, PieceType pt = KNIGHT) {
        return Move(T + ((pt - KNIGHT) << 12) + (from << 6) + to);
    }

    static constexpr Move None() { return Move(0); }

    constexpr Square FromSq() const { return Square((data >> 6) & 0x3F); }

    constexpr Square ToSq() const { return Square(data & 0x3F); }

    constexpr MoveType TypeOf() const { return MoveType(data & (3 << 14)); }

    constexpr PieceType PromotionType() const { return PieceType(((data >> 12) & 3) + KNIGHT); }

    constexpr bool operator==(const Move& m) const { return data == m.data; }
    constexpr bool operator!=(const Move& m) const { return data != m.data; }

    friend std::ostream& operator<<(std::ostream& os, const Move& move) {
        os << move.FromSq() << move.ToSq();

        if (move.TypeOf() == PROMOTION) {
            char promoCh;
            switch (move.PromotionType()) {
                case KNIGHT: promoCh = 'n'; break;
                case ROOK: promoCh = 'r'; break;
                case BISHOP: promoCh = 'b'; break;
                default: promoCh = 'q'; break;
            }
            os << promoCh;
        }

        return os;
    }
};

struct MoveList {
    Move Moves[MAX_MOVES];
    int Count = 0;

    friend std::ostream& operator<<(std::ostream& os, const MoveList& list) {
        for (int i = 0; i < list.Count; ++i) {
            os << std::setw(2) << i + 1 << ": " << list.Moves[i] << "\n";
        }
        os << "Total: " << list.Count << " moves.\n";
        return os;
    }
};

} // namespace Zugzwang