#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "types.h"

namespace ChessCpp {
class Board;

// static class
class MoveGen {
  private:
    static void generate_sliding_moves(const Board& board, Square startSq, MoveList& list);
    static void generate_king_moves(const Board& board, Square startSq, MoveList& list);
    static void generate_knight_moves(const Board& board, Square startSq, MoveList& list);
    static void generate_pawn_moves(const Board& board, Square startSq, MoveList& list);

  public:
    MoveGen() = delete;
    static bool is_square_attacked(const Board& board, Square sq, Color attacker);
    static void generate_pseudo_moves(const Board& board, MoveList& list);
    static void generate_legal_moves(const Board& board, MoveList& list);
};
}  // namespace ChessCpp

#endif  // MOVEGEN_H
