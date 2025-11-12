#ifndef BOARD_H
#define BOARD_H

#include <string>
#include "types.h"

namespace ChessCpp {
namespace Zobrist {
    inline Key psq[PIECE_NB][SQUARE_NB];
    inline Key castling[CASTLING_RIGHT_NB];
    inline Key side;
}  // namespace Zobrist

struct StateInfo {
    Square epSquare;
    int rule50;
    int castlingRights;
    Piece captured;
    Key posKey;
};

class Board {
  private:
    unsigned long long perftLealNodes;

    void put_piece(Piece piece, Square sq);
    void remove_piece(Square sq);
    void move_piece(Square from, Square to);
    void perft(int depth);

  public:
    Piece pieces[SQUARE_NB];
    Square kingSquare[COLOR_NB];
    Color sideToMove;
    Square epSquare;
    int rule50;
    int gamePly;
    int castlingRights;
    Key posKey;

    StateInfo history[MAX_PLIES];

    static void init_zobrist();
    void generatePosKey();
    void reset();
    void set(const std::string& fenStr);
    void print() const;

    bool do_move(const Move& move);
    void undo_move(const Move& move);

    void perftTest(int depth);
};

inline void Board::put_piece(Piece piece, Square sq) {
    ASSERT(piece != NO_PIECE);
    pieces[sq] = piece;
    posKey ^= Zobrist::psq[piece][sq];
}

inline void Board::remove_piece(Square sq) {
    Piece piece = pieces[sq];
    ASSERT(piece != NO_PIECE);
    posKey ^= Zobrist::psq[piece][sq];
    pieces[sq] = NO_PIECE;
}

inline void Board::move_piece(Square from, Square to) {
    Piece piece = pieces[from];
    if (piece == NO_PIECE)
        ASSERT(piece != NO_PIECE);

    remove_piece(from);
    put_piece(piece, to);
}
}  // namespace ChessCpp

#endif  // BOARD_H
