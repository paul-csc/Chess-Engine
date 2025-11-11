#ifndef BOARD_H
#define BOARD_H

#include <string>
#include "types.h"

namespace ChessCpp {
class Board {
  public:
    Piece pieces[SQUARE_NB];
    Square kingSquare[COLOR_NB];
    Color sideToMove;
    Square epSquare;
    int rule50;
    int gamePly;
    int castlingRights;
    Key posKey;

    static void init_zobrist();
    void generatePosKey();
    void reset();
    void set(const std::string& fenStr);
    void print() const;
    void do_move(Move& move);
    void undo_move();
};
}  // namespace ChessCpp

#endif  // BOARD_H
