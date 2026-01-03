#pragma once

#include "types.h"

namespace Zugzwang {
class Board;

class MoveGen {
  public:
    static bool IsSquareAttacked(const Board& board, Square sq, Color attacker);
    static bool ISMoveLegal(Board& board, const Move move);
    static void GeneratePseudoMoves(const Board& board, MoveList& list);

  private:
    MoveGen() {}

    static void GenerateSlidingMoves(const Board& board, MoveList& list);
    static void GenerateKingMoves(const Board& board, MoveList& list);
    static void GenerateKnightMoves(const Board& board, MoveList& list);
    static void GeneratePawnMoves(const Board& board, MoveList& list);
};
} // namespace Zugzwang
