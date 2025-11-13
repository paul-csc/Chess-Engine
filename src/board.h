#ifndef BOARD_H
#define BOARD_H

#include <string>
#include "types.h"
#include "bitboard.h"

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
    int pieceNb[PIECE_NB];
    Square pieceList[PIECE_NB][10];
    Square kingSquare[COLOR_NB];
    Color sideToMove;
    Bitboard byColorBB[COLOR_NB];

    Square epSquare;
    int rule50;
    int gamePly;
    int castlingRights;
    Key posKey;

    StateInfo history[MAX_PLIES];

    static void init_zobrist();
    void generatePosKey();
    void reset();
    void updateListsBitboards();
    void set(const std::string& fenStr);
    void print() const;

    bool do_move(const Move& move);
    void undo_move(const Move& move);

    void perftTest(int depth);
};

inline void Board::put_piece(Piece piece, Square sq) {
    ASSERT(piece != NO_PIECE);
    Color color = color_of(piece);

    pieces[sq] = piece;
    posKey ^= Zobrist::psq[piece][sq];
    pieceList[piece][pieceNb[piece]++] = sq;
    set_bit(byColorBB[color], sq);
}

inline void Board::remove_piece(Square sq) {
    Piece piece = pieces[sq];

    ASSERT(piece != NO_PIECE);

    Color color = color_of(piece);
    posKey ^= Zobrist::psq[piece][sq];
    pieces[sq] = NO_PIECE;

    clear_bit(byColorBB[color], sq);

    for (int i = 0; i < pieceNb[piece]; ++i) {
        if (pieceList[piece][i] == sq) {
            pieceList[piece][i] = pieceList[piece][--pieceNb[piece]];
            return;
        }
    }
    ASSERT(false);
}

inline void Board::move_piece(Square from, Square to) {
    Piece piece = pieces[from];
    ASSERT(piece != NO_PIECE);
    Color color = color_of(piece);

    posKey ^= Zobrist::psq[piece][from];
    posKey ^= Zobrist::psq[piece][to];
    pieces[from] = NO_PIECE;
    pieces[to] = piece;
    clear_bit(byColorBB[color], from);
    set_bit(byColorBB[color], to);

    for (int i = 0; i < pieceNb[piece]; ++i) {
        if (pieceList[piece][i] == from) {
            pieceList[piece][i] = to;
            return;
        }
    }
    ASSERT(false);
}
}  // namespace ChessCpp

#endif  // BOARD_H
