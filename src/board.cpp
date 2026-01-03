#include "board.h"
#include "movegen.h"
#include "types.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

namespace Zugzwang {

namespace {
constexpr std::string_view PieceToChar(" PNBRQK  pnbrqk");

// clang-format off
constexpr int CastlePerm[64] = {
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 11, 15
};
// clang-format on

uint64_t seed = 1804289383ULL;
inline uint64_t rand64() {
    uint64_t x = seed;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    seed = x;
    return x * 2685821657736338717ULL;
}
} // namespace

void Board::InitZobrist() {
    for (int i = 0; i < PIECE_NB; ++i) {
        for (Square j = SQ_A1; j < SQUARE_NB; ++j) {
            Zobrist::psq[i][j] = rand64();
        }
    }
    Zobrist::side = rand64();
    for (int i = 0; i < CASTLING_RIGHT_NB; ++i) {
        Zobrist::castling[i] = rand64();
    }
}

void Board::GeneratePosKey() {
    for (Square i = SQ_A1; i < SQUARE_NB; ++i) {
        Piece piece = Pieces[i];
        if (piece != NO_PIECE) {
            PosKey ^= Zobrist::psq[piece][i];
        }
    }

    if (SideToMove == WHITE) {
        PosKey ^= Zobrist::side;
    }

    if (EpSquare != SQ_NONE) {
        PosKey ^= Zobrist::psq[NO_PIECE][EpSquare];
    }

    PosKey ^= Zobrist::castling[CastlingRights];
}

void Board::Reset() {
    for (Square i = SQ_A1; i < SQUARE_NB; ++i) {
        Pieces[i] = NO_PIECE;
    }

    for (int i = 0; i < 13; ++i) {
        PieceNumber[i] = 0;
    }

    ByColorBB[WHITE] = ByColorBB[BLACK] = 0ULL;
    KingSquare[WHITE] = KingSquare[BLACK] = SQ_NONE;
    SideToMove = WHITE;
    EpSquare = SQ_NONE;
    FiftyMoveCount = 0;
    GamePly = 0;
    CastlingRights = NO_CASTLING;
    PosKey = 0ULL;
}

void Board::UpdateListsBitboards() {
    for (Square sq = SQ_A1; sq < SQUARE_NB; ++sq) {
        Piece piece = Pieces[sq];

        if (piece != NO_PIECE) {
            Color color = ColorOf(piece);
            SetBit(ByColorBB[color], sq);
            PieceList[piece][PieceNumber[piece]++] = sq;
        }
    }
}

void Board::PutPiece(Piece piece, Square sq) {
    ASSERT(piece != NO_PIECE);
    Color color = ColorOf(piece);

    Pieces[sq] = piece;
    PosKey ^= Zobrist::psq[piece][sq];
    PieceList[piece][PieceNumber[piece]++] = sq;
    SetBit(ByColorBB[color], sq);
}

void Board::RemoveRiece(Square sq) {
    Piece piece = Pieces[sq];

    ASSERT(piece != NO_PIECE);

    Color color = ColorOf(piece);
    PosKey ^= Zobrist::psq[piece][sq];
    Pieces[sq] = NO_PIECE;

    ClearBit(ByColorBB[color], sq);

    for (int i = 0; i < PieceNumber[piece]; ++i) {
        if (PieceList[piece][i] == sq) {
            PieceList[piece][i] = PieceList[piece][--PieceNumber[piece]];
            return;
        }
    }
    ASSERT(false);
}

void Board::MovePiece(Square from, Square to) {
    Piece piece = Pieces[from];
    ASSERT(piece != NO_PIECE);
    Color color = ColorOf(piece);

    PosKey ^= Zobrist::psq[piece][from];
    PosKey ^= Zobrist::psq[piece][to];
    Pieces[from] = NO_PIECE;
    Pieces[to] = piece;
    ClearBit(ByColorBB[color], from);
    SetBit(ByColorBB[color], to);

    for (int i = 0; i < PieceNumber[piece]; ++i) {
        if (PieceList[piece][i] == from) {
            PieceList[piece][i] = to;
            return;
        }
    }
    ASSERT(false);
}

void Board::ParseFen(const char* fenStr) {
    Reset();

    unsigned char col, row, token;
    size_t idx;
    Square sq = SQ_A8;
    std::istringstream ss(fenStr);

    ss >> std::noskipws;

    // 1. Piece placement
    while ((ss >> token) && !isspace(token)) {
        if (isdigit(token)) {
            sq += (token - '0') * EAST;
        } else if (token == '/') {
            sq += 2 * SOUTH;
        } else if ((idx = PieceToChar.find(token)) != std::string::npos) {
            Pieces[sq] = Piece(idx);
            if (Piece(idx) == W_KING) {
                KingSquare[WHITE] = sq;
            }
            if (Piece(idx) == B_KING) {
                KingSquare[BLACK] = sq;
            }
            ++sq;
        }
    }

    // 2. Active color
    ss >> token;
    SideToMove = (token == 'w' ? WHITE : BLACK);
    ss >> token;

    // 3. Castling availability
    while ((ss >> token) && !isspace(token)) {
        switch (token) {
            case 'K': CastlingRights |= WHITE_OO; break;
            case 'k': CastlingRights |= BLACK_OO; break;
            case 'Q': CastlingRights |= WHITE_OOO; break;
            case 'q': CastlingRights |= BLACK_OOO; break;
        }
    }

    // 4. En passant square
    if (((ss >> col) && (col >= 'a' && col <= 'h')) &&
        ((ss >> row) && (row == (SideToMove == WHITE ? '6' : '3')))) {
        EpSquare = MakeSquare(File(col - 'a'), Rank(row - '1'));
    }

    // 5. Halfmove clock
    ss >> std::skipws >> FiftyMoveCount >> GamePly;

    // Convert from fullmove starting from 1 to internal ply count
    GamePly = std::max(2 * (GamePly - 1), 0) + (SideToMove == BLACK);

    GeneratePosKey();
    UpdateListsBitboards();
}

void Board::Print() const {
    using std::cout;

    cout << "+---+---+---+---+---+---+---+---+\n";
    for (Rank rank = RANK_8; rank >= RANK_1; --rank) {
        for (File file = FILE_A; file <= FILE_H; ++file) { // <= not <
            Square sq = MakeSquare(file, rank);
            Piece p = Pieces[sq];
            char c = (p != NO_PIECE ? PieceToChar[p] : ' ');
            cout << "| " << c << " ";
        }
        cout << "| " << (int(rank) + 1) << "\n"
             << "+---+---+---+---+---+---+---+---+\n";
    }

    cout << "  a   b   c   d   e   f   g   h\n";
    cout << "Side to move: " << (SideToMove == WHITE ? "w" : "b") << "\n";
    cout << "En passant square: ";
    if (IsOk(EpSquare)) {
        cout << EpSquare;
    } else {
        cout << "none";
    }
    cout << "\n";
    cout << "Castle permissions: " << (CastlingRights & WHITE_OO ? "K" : "-")
         << (CastlingRights & WHITE_OOO ? "Q" : "-") << (CastlingRights & BLACK_OO ? "k" : "-")
         << (CastlingRights & BLACK_OOO ? "q" : "-") << "\n";
    cout << "Position key: " << PosKey << "\n";
}

bool Board::IsInCheck(Color color) const {
    return MoveGen::IsSquareAttacked(*this, KingSquare[color], ~color);
}

bool Board::IsRepetition() const {
    for (int i = GamePly - FiftyMoveCount; i < GamePly - 1; ++i) {
        ASSERT(i >= 0 && i < MAX_PLIES);
        if (PosKey == m_History[i].PosKey) {
            return true;
        }
    }
    return false;
}

bool Board::MakeMove(const Move& move) {
    const Square from = move.FromSq();
    const Square to = move.ToSq();

    ASSERT(IsOk(from));
    ASSERT(IsOk(to));

    // save state
    m_History[GamePly].Move = move;
    m_History[GamePly].PosKey = PosKey;
    m_History[GamePly].FiftyMoveCount = FiftyMoveCount;
    m_History[GamePly].EpSquare = EpSquare;
    m_History[GamePly].CastlingRights = CastlingRights;
    m_History[GamePly].Captured = Pieces[to]; // normal captures only; en-passant handled separately

    // remove old EP & castling from hash
    if (EpSquare != SQ_NONE) {
        PosKey ^= Zobrist::psq[NO_PIECE][EpSquare];
    }
    PosKey ^= Zobrist::castling[CastlingRights];

    // special move handling
    if (move.TypeOf() == EN_PASSANT) {
        // remove the captured pawn (behind 'to')
        RemoveRiece(to + (SideToMove == WHITE ? SOUTH : NORTH));
        FiftyMoveCount = 0; // reset 50-move on capture
    } else if (move.TypeOf() == CASTLING) {
        switch (to) {
            case SQ_C1: MovePiece(SQ_A1, SQ_D1); break;
            case SQ_C8: MovePiece(SQ_A8, SQ_D8); break;
            case SQ_G1: MovePiece(SQ_H1, SQ_F1); break;
            case SQ_G8: MovePiece(SQ_H8, SQ_F8); break;
            default: ASSERT(false);
        }
    }

    // normal capture handling (if a piece sits on 'to')
    if (Pieces[to] != NO_PIECE) {
        RemoveRiece(to);
        FiftyMoveCount = 0;
    } else if (move.TypeOf() != EN_PASSANT) {
        // only increment rule50 if it wasn't a capture (en-passant already set to 0)
        FiftyMoveCount++;
    }

    // move the piece
    MovePiece(from, to);

    // promotion handling
    if (move.TypeOf() == PROMOTION) {
        RemoveRiece(to);
        PutPiece(MakePiece(SideToMove, move.PromotionType()), to);
    }

    // update king square (if moved)
    if (TypeOf(Pieces[to]) == KING) {
        KingSquare[SideToMove] = to;
    }

    // new en-passant target (from a double pawn push)
    EpSquare = SQ_NONE;
    if (TypeOf(Pieces[to]) == PAWN && std::abs(RankOf(from) - RankOf(to)) == 2) {
        EpSquare = from + (SideToMove == WHITE ? NORTH : SOUTH);
    }
    if (EpSquare != SQ_NONE) {
        PosKey ^= Zobrist::psq[NO_PIECE][EpSquare]; // add new EP key
    }

    // update castling rights and re-add castling key
    CastlingRights &= CastlePerm[from];
    CastlingRights &= CastlePerm[to];
    PosKey ^= Zobrist::castling[CastlingRights];

    // flip side
    SideToMove = ~SideToMove;
    PosKey ^= Zobrist::side;

    GamePly++;

    if (IsInCheck(~SideToMove)) {
        UnmakeMove();
        return false;
    }
    return true;
}

void Board::UnmakeMove() {
    GamePly--;

    ASSERT(GamePly >= 0 && GamePly < MAX_MOVES);

    const Move move = m_History[GamePly].Move;
    const Square from = move.FromSq();
    const Square to = move.ToSq();

    ASSERT(IsOk(from));
    ASSERT(IsOk(to));

    SideToMove = ~SideToMove;

    // Undo special moves first where needed
    if (move.TypeOf() == EN_PASSANT) {
        // the captured pawn belongs to the opponent of the mover
        PutPiece(MakePiece(~SideToMove, PAWN), to + (SideToMove == WHITE ? SOUTH : NORTH));
    } else if (move.TypeOf() == CASTLING) {
        switch (to) {
            case SQ_C1: MovePiece(SQ_D1, SQ_A1); break;
            case SQ_C8: MovePiece(SQ_D8, SQ_A8); break;
            case SQ_G1: MovePiece(SQ_F1, SQ_H1); break;
            case SQ_G8: MovePiece(SQ_F8, SQ_H8); break;
            default: ASSERT(false);
        }
    }

    MovePiece(to, from);

    if (move.TypeOf() == PROMOTION) {
        RemoveRiece(from);
        PutPiece(MakePiece(SideToMove, PAWN), from);
    }

    if (m_History[GamePly].Captured != NO_PIECE) {
        PutPiece(m_History[GamePly].Captured, to);
    }

    if (TypeOf(Pieces[from]) == KING) {
        KingSquare[SideToMove] = from;
    }

    EpSquare = m_History[GamePly].EpSquare;
    FiftyMoveCount = m_History[GamePly].FiftyMoveCount;
    CastlingRights = m_History[GamePly].CastlingRights;
    PosKey = m_History[GamePly].PosKey;
}

void Board::Perft(int depth) {
    if (depth == 0) {
        m_PerftLealNodes++;
        return;
    }
    MoveList list;
    MoveGen::GeneratePseudoMoves(*this, list);
    for (int i = 0; i < list.Count; ++i) {
        if (!MakeMove(list.Moves[i])) {
            continue;
        }
        Perft(depth - 1);
        UnmakeMove();
    }
}

void Board::PerftTest(int depth) {
    Print();
    std::cout << "Starting perft test to depth " << depth << "\n";

    m_PerftLealNodes = 0;
    MoveList list;

    using namespace std::chrono;
    auto start = high_resolution_clock::now();

    MoveGen::GeneratePseudoMoves(*this, list);
    for (int i = 0; i < list.Count; ++i) {
        Move move = list.Moves[i];
        if (!MakeMove(move)) {
            continue;
        }
        unsigned long long before = m_PerftLealNodes;
        Perft(depth - 1);
        UnmakeMove();
        std::cout << move << ": " << m_PerftLealNodes - before << "\n";
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start).count();

    std::cout << "Total: " << m_PerftLealNodes << " nodes in " << duration << " ms\n";
}
} // namespace Zugzwang
