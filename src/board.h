#pragma once

#include "bitboard.h"
#include "types.h"

namespace Zugzwang {
namespace Zobrist {

inline Key psq[PIECE_NB][SQUARE_NB]; // psq[NO_PIECE] for en passant
inline Key castling[CASTLING_RIGHT_NB];
inline Key side;

} // namespace Zobrist

struct StateInfo {
    Square epSquare;
    int rule50;
    int castlingRights;
    Piece captured;
    Key posKey;
};

class Board {
  public:
    Board();

    void ParseFen(const std::string& fen);

    bool MakeMove(const Move& move);
    void UnmakeMove(const Move& move);

    void Print() const;

    uint64_t PerftTest(int depth);

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

  private:
    void InitZobrist();

    void PutPiece(Piece piece, Square sq);
    void RemovePiece(Square sq);
    void MovePiece(Square from, Square to);

    void GeneratePosKey();
    void Reset();
    void UpdateListsBitboards();
    void Perft(int depth);

    uint64_t perftLealNodes;

    StateInfo history[MAX_PLIES];
};

} // namespace Zugzwang
