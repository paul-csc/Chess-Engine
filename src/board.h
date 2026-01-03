#pragma once

#include "bitboard.h"
#include "movegen.h"
#include "types.h"
#include <string>

namespace Zugzwang {

class MoveGen;

namespace Zobrist {

inline Key psq[PIECE_NB][SQUARE_NB];
inline Key castling[CASTLING_RIGHT_NB];
inline Key side;

} // namespace Zobrist

struct StateInfo {
    Move Move;

    Square EpSquare;
    int FiftyMoveCount;
    int CastlingRights;
    Piece Captured;
    Key PosKey;
};

class Board {
  public:
    Piece Pieces[SQUARE_NB];
    int PieceNumber[PIECE_NB];
    Square PieceList[PIECE_NB][10];
    Square KingSquare[COLOR_NB];
    Color SideToMove;
    Bitboard ByColorBB[COLOR_NB];

    int GamePly;

    Square EpSquare;
    int FiftyMoveCount;
    int CastlingRights;
    Key PosKey;

    static void InitZobrist();
    void Reset();
    void ParseFen(const char* fenStr);
    void Print() const;
    bool IsInCheck(Color color) const;
    bool IsRepetition() const;

    bool MakeMove(const Move& move);
    void UnmakeMove();

    void PerftTest(int depth);

  private:
    StateInfo m_History[MAX_PLIES];

    unsigned long long m_PerftLealNodes;

    void GeneratePosKey();
    void UpdateListsBitboards();
    void PutPiece(Piece piece, Square sq);
    void RemoveRiece(Square sq);
    void MovePiece(Square from, Square to);
    void Perft(int depth);
};

} // namespace Zugzwang