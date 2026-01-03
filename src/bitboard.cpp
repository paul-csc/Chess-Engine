#include "bitboard.h"
#include "types.h"

namespace Zugzwang {
void Magics::createAllBlockerBitboards(
    Bitboard movementMask, Bitboard blockers[], int& numPatterns) {
    int numSquares = __builtin_popcountll(movementMask);
    numPatterns = 1 << numSquares;

    int idx = 0;
    Bitboard subset = 0;
    do {
        blockers[idx++] = subset;
        subset = (subset - 1) & movementMask;
    } while (subset);
}

Bitboard Magics::createRookLegalMovesBitboard(Square square, Bitboard blockBitboard) {
    Bitboard moves = 0ULL;

    Square s = square + NORTH; // Up
    while (s < SQUARE_NB) {
        moves |= 1ULL << s;
        if (blockBitboard & (1ULL << s)) {
            break;
        }
        s += NORTH;
    }
    s = square + SOUTH; // Down
    while (s >= 0) {
        moves |= 1ULL << s;
        if (blockBitboard & (1ULL << s)) {
            break;
        }
        s += SOUTH;
    }
    s = square + WEST;
    while (s >= 0 && FileOf(s) < FileOf(square)) {
        moves |= 1ULL << s;
        if (blockBitboard & (1ULL << s)) {
            break;
        }
        s += WEST;
    }
    s = square + EAST;
    while (s < SQUARE_NB && FileOf(s) > FileOf(square)) {
        moves |= 1ULL << s;
        if (blockBitboard & (1ULL << s)) {
            break;
        }
        s += EAST;
    }
    return moves;
}

Bitboard Magics::createBishopLegalMovesBitboard(Square square, Bitboard blockBitboard) {
    Bitboard moves = 0ULL;
    int file = FileOf(square);
    Square s = square + NORTH_EAST;
    while (s < SQUARE_NB && FileOf(s) > file) {
        moves |= 1ULL << s;
        if (blockBitboard & (1ULL << s)) {
            break;
        }
        s += NORTH_EAST;
    }
    s = square + NORTH_WEST;
    while (s < SQUARE_NB && FileOf(s) < file) {
        moves |= 1ULL << s;
        if (blockBitboard & (1ULL << s)) {
            break;
        }
        s += NORTH_WEST;
    }
    s = square + SOUTH_EAST;
    while (s >= 0 && FileOf(s) > file) {
        moves |= 1ULL << s;
        if (blockBitboard & (1ULL << s)) {
            break;
        }
        s += SOUTH_EAST;
    }
    s = square + SOUTH_WEST;
    while (s >= 0 && FileOf(s) < file) {
        moves |= 1ULL << s;
        if (blockBitboard & (1ULL << s)) {
            break;
        }
        s += SOUTH_WEST;
    }
    return moves;
}

void Magics::Init() {
    Bitboard blockers[4096]; // 2 ^ 12
    int numPatterns;

    for (Square startSquare = SQ_A1; startSquare < SQUARE_NB; ++startSquare) {
        Bitboard movementMask = Magics::RookMasks[startSquare];
        createAllBlockerBitboards(movementMask, blockers, numPatterns);

        for (int i = 0; i < numPatterns; ++i) {
            Bitboard blockerBitboard = blockers[i];
            Bitboard index = ((blockerBitboard & movementMask) * Magics::RookMagics[startSquare]) >>
                Magics::RookShifts[startSquare];
            RookAttackTable[startSquare][index] = createRookLegalMovesBitboard(startSquare, blockerBitboard);
        }

        movementMask = Magics::BishopMasks[startSquare];
        createAllBlockerBitboards(movementMask, blockers, numPatterns);

        for (int i = 0; i < numPatterns; ++i) {
            Bitboard blockerBitboard = blockers[i];
            Bitboard index = ((blockerBitboard & movementMask) * Magics::BishopMagics[startSquare]) >>
                Magics::BishopShifts[startSquare];
            BishopAttackTable[startSquare][index] =
                createBishopLegalMovesBitboard(startSquare, blockerBitboard);
        }
    }
}
} // namespace Zugzwang