#pragma once

#include "types.h"
#include <bit>

namespace Zugzwang {

class Bitboards {
  public:
    Bitboards() = delete;

    static void Init();

    template <PieceType P>
    static Bitboard GetAttacks(Square square, Bitboard occupancy = 0, Color color = WHITE) {
        if constexpr (P == ROOK) {
            return RookAttackTable[square][((occupancy & RookMasks[square]) * RookMagics[square]) >>
                RookShifts[square]];
        } else if constexpr (P == BISHOP) {
            return BishopAttackTable[square][((occupancy & BishopMasks[square]) * BishopMagics[square]) >>
                BishopShifts[square]];
        } else if constexpr (P == QUEEN) {
            return GetAttacks<ROOK>(square, occupancy) | GetAttacks<BISHOP>(square, occupancy);
        } else if constexpr (P == PieceType::KNIGHT) {
            return KnightAttacks[square];
        } else if constexpr (P == PieceType::KING) {
            return KingAttacks[square];
        } else if constexpr (P == PieceType::PAWN) {
            return PawnAttacks[color][square];
        }
    }

  private:
    static const int RookShifts[SQUARE_NB];
    static const int BishopShifts[SQUARE_NB];
    static const Bitboard RookMagics[SQUARE_NB];
    static const Bitboard BishopMagics[SQUARE_NB];
    static const Bitboard RookMasks[SQUARE_NB];
    static const Bitboard BishopMasks[SQUARE_NB];

    static Bitboard RookAttackTable[SQUARE_NB][4096];
    static Bitboard BishopAttackTable[SQUARE_NB][1024];

    static const Bitboard KnightAttacks[64];
    static const Bitboard KingAttacks[64];
    static const Bitboard PawnAttacks[2][64];

    static int CreateBlockerBitboards(Bitboard movementMask, Bitboard blockers[]);
    static Bitboard CreateRookBitboard(Square square, Bitboard blockBitboard);
    static Bitboard CreateBishopBitboard(Square square, Bitboard blockBitboard);
};

inline Bitboard SquareBb(Square s) {
    ASSERT(IsOk(s));
    return (1ULL << s);
}

inline Square PopLsb(Bitboard& b) {
    ASSERT(b);
    Square index = Square(std::countr_zero(b));
    b &= (b - 1);
    return index;
}

constexpr Square PopCount(Bitboard b) { return Square(std::popcount(b)); }

constexpr void SetBit(Bitboard& b, Square sq) { b |= (1ULL << sq); }

constexpr void ClearBit(Bitboard& b, Square sq) { b &= ~(1ULL << sq); }

} // namespace Zugzwang