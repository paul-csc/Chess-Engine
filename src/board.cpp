#include "board.h"
#include "movegen.h"
#include "types.h"
#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>

namespace Zugzwang {
namespace {

constexpr auto PieceToChar = " PNBRQK  pnbrqk";

// clang-format off
constexpr int CastlePerm[64] = {
    13, 15, 15, 15, 12, 15, 15, 14,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
     7, 15, 15, 15,  3, 15, 15, 11,
};
// clang-format on

uint64_t seed = 1804289383ULL;
uint64_t rand64() {
    uint64_t x = seed;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    seed = x;
    return x * 2685821657736338717ULL;
}

} // namespace

Board::Board() { InitZobrist(); }

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

void Board::PutPiece(Piece piece, Square sq) {
    ASSERT(piece != NO_PIECE);
    Color color = ColorOf(piece);

    pieces[sq] = piece;
    posKey ^= Zobrist::psq[piece][sq];
    pieceList[piece][pieceNb[piece]++] = sq;
    SetBit(byColorBB[color], sq);
}

void Board::RemovePiece(Square sq) {
    Piece piece = pieces[sq];
    ASSERT(piece != NO_PIECE);

    Color color = ColorOf(piece);

    posKey ^= Zobrist::psq[piece][sq];
    pieces[sq] = NO_PIECE;

    ClearBit(byColorBB[color], sq);

    for (int i = 0; i < pieceNb[piece]; ++i) {
        if (pieceList[piece][i] == sq) {
            pieceList[piece][i] = pieceList[piece][--pieceNb[piece]];
            return;
        }
    }
    ASSERT(false);
}

void Board::MovePiece(Square from, Square to) {
    Piece piece = pieces[from];
    ASSERT(piece != NO_PIECE);

    Color color = ColorOf(piece);

    posKey ^= Zobrist::psq[piece][from];
    posKey ^= Zobrist::psq[piece][to];
    pieces[from] = NO_PIECE;
    pieces[to] = piece;

    ClearBit(byColorBB[color], from);
    SetBit(byColorBB[color], to);

    for (int i = 0; i < pieceNb[piece]; ++i) {
        if (pieceList[piece][i] == from) {
            pieceList[piece][i] = to;
            return;
        }
    }
    ASSERT(false);
}

void Board::GeneratePosKey() {
    for (Square i = SQ_A1; i < SQUARE_NB; ++i) {
        Piece piece = pieces[i];
        if (piece != NO_PIECE) {
            posKey ^= Zobrist::psq[piece][i];
        }
    }

    if (sideToMove == WHITE) {
        posKey ^= Zobrist::side;
    }

    if (epSquare != SQ_NONE) {
        posKey ^= Zobrist::psq[NO_PIECE][epSquare];
    }

    posKey ^= Zobrist::castling[castlingRights];
}

void Board::Reset() {
    for (Square i = SQ_A1; i < SQUARE_NB; ++i) {
        pieces[i] = NO_PIECE;
    }

    for (int i = 0; i < PIECE_NB; ++i) {
        pieceNb[i] = 0;
    }

    byColorBB[WHITE] = byColorBB[BLACK] = 0ULL;
    kingSquare[WHITE] = kingSquare[BLACK] = SQ_NONE;
    sideToMove = WHITE;
    epSquare = SQ_NONE;
    rule50 = 0;
    gamePly = 0;
    castlingRights = NO_CASTLING;
    posKey = 0ULL;
}

void Board::UpdateListsBitboards() {
    for (Square sq = SQ_A1; sq < SQUARE_NB; ++sq) {
        const Piece piece = pieces[sq];

        if (piece != NO_PIECE) {
            Color color = ColorOf(piece);

            SetBit(byColorBB[color], sq);
            pieceList[piece][pieceNb[piece]++] = sq;
        }
    }
}

void Board::ParseFen(const std::string& fen) {
    Reset();

    unsigned char col, row, token;
    size_t idx;
    Square sq = SQ_A8;
    std::istringstream ss(fen);

    ss >> std::noskipws;

    // 1. Piece placement
    while ((ss >> token) && !isspace(token)) {
        if (isdigit(token)) {
            sq += (token - '0') * EAST;
        } else if (token == '/') {
            sq += 2 * SOUTH;
        } else {
            const char* p = std::strchr(PieceToChar, token);
            if (p) {
                idx = p - PieceToChar;
                pieces[sq] = Piece(idx);
                if (Piece(idx) == W_KING) {
                    kingSquare[WHITE] = sq;
                }
                if (Piece(idx) == B_KING) {
                    kingSquare[BLACK] = sq;
                }
                ++sq;
            }
        }
    }

    // 2. Active color
    ss >> token;
    sideToMove = (token == 'w' ? WHITE : BLACK);
    ss >> token;

    // 3. Castling availability
    while ((ss >> token) && !isspace(token)) {
        switch (token) {
            case 'K': castlingRights |= WHITE_OO; break;
            case 'k': castlingRights |= BLACK_OO; break;
            case 'Q': castlingRights |= WHITE_OOO; break;
            case 'q': castlingRights |= BLACK_OOO; break;
        }
    }

    // 4. En passant square
    if (((ss >> col) && (col >= 'a' && col <= 'h')) &&
        ((ss >> row) && (row == (sideToMove == WHITE ? '6' : '3')))) {
        epSquare = MakeSquare(File(col - 'a'), Rank(row - '1'));
    }

    // 5. Halfmove clock (rule50)
    ss >> std::skipws >> rule50 >> gamePly;

    // Convert from fullmove starting from 1 to internal ply count
    gamePly = std::max(2 * (gamePly - 1), 0) + (sideToMove == BLACK);

    GeneratePosKey();
    UpdateListsBitboards();
}

bool Board::MakeMove(const Move& move) {
    const Square from = move.FromSq();
    const Square to = move.ToSq();

    // save state
    history[gamePly].posKey = posKey;
    history[gamePly].rule50 = rule50;
    history[gamePly].epSquare = epSquare;
    history[gamePly].castlingRights = castlingRights;
    history[gamePly].captured = pieces[to]; // normal captures only; en-passant handled separately

    // remove old EP & castling from hash
    if (epSquare != SQ_NONE) {
        posKey ^= Zobrist::psq[NO_PIECE][epSquare];
    }
    posKey ^= Zobrist::castling[castlingRights];

    // special move handling
    if (move.TypeOf() == EN_PASSANT) {
        // remove the captured pawn (behind 'to')
        RemovePiece(to + (sideToMove == WHITE ? SOUTH : NORTH));
        rule50 = 0; // reset 50-move on capture
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
    if (pieces[to] != NO_PIECE) {
        RemovePiece(to);
        rule50 = 0;
    } else if (move.TypeOf() != EN_PASSANT) {
        // only increment rule50 if it wasn't a capture (en-passant already set to 0)
        rule50++;
    }

    // move the piece
    MovePiece(from, to);

    // promotion handling
    if (move.TypeOf() == PROMOTION) {
        RemovePiece(to);
        PutPiece(MakePiece(sideToMove, move.PromotionType()), to);
    }

    // update king square (if moved)
    if (TypeOf(pieces[to]) == KING) {
        kingSquare[sideToMove] = to;
    }

    // new en-passant target (from a double pawn push)
    epSquare = SQ_NONE;
    if (TypeOf(pieces[to]) == PAWN && std::abs(RankOf(from) - RankOf(to)) == 2) {
        epSquare = from + (sideToMove == WHITE ? NORTH : SOUTH);
    }
    if (epSquare != SQ_NONE) {
        posKey ^= Zobrist::psq[NO_PIECE][epSquare]; // add new EP key
    }

    // update castling rights and re-add castling key
    castlingRights &= CastlePerm[from];
    castlingRights &= CastlePerm[to];
    posKey ^= Zobrist::castling[castlingRights];

    // flip side
    sideToMove = ~sideToMove;
    posKey ^= Zobrist::side;

    gamePly++;

    if (MoveGen::IsSquareAttacked(*this, kingSquare[~sideToMove], sideToMove)) {
        UnmakeMove(move);
        return false;
    }
    return true;
}

void Board::UnmakeMove(const Move& move) {
    gamePly--;

    const Square from = move.FromSq();
    const Square to = move.ToSq();

    if (move.TypeOf() == EN_PASSANT) {
        PutPiece(MakePiece(sideToMove, PAWN), to + PawnPush(sideToMove));
    } else if (move.TypeOf() == CASTLING) {
        switch (to) {
            case SQ_C1: MovePiece(SQ_D1, SQ_A1); break;
            case SQ_C8: MovePiece(SQ_D8, SQ_A8); break;
            case SQ_G1: MovePiece(SQ_F1, SQ_H1); break;
            case SQ_G8: MovePiece(SQ_F8, SQ_H8); break;
            default: ASSERT(false);
        }
    }

    sideToMove = ~sideToMove;

    MovePiece(to, from);

    if (move.TypeOf() == PROMOTION) {
        RemovePiece(from);
        PutPiece(MakePiece(sideToMove, PAWN), from);
    }

    if (history[gamePly].captured != NO_PIECE) {
        PutPiece(history[gamePly].captured, to);
    }

    if (TypeOf(pieces[from]) == KING) {
        kingSquare[sideToMove] = from;
    }

    epSquare = history[gamePly].epSquare;
    rule50 = history[gamePly].rule50;
    castlingRights = history[gamePly].castlingRights;
    posKey = history[gamePly].posKey;
}

void Board::Print() const {
    using std::cout;

    cout << "+---+---+---+---+---+---+---+---+\n";
    for (Rank rank = RANK_8; rank >= RANK_1; --rank) {
        for (File file = FILE_A; file <= FILE_H; ++file) { // <= not <
            Square sq = MakeSquare(file, rank);
            Piece p = pieces[sq];
            char c = (p != NO_PIECE ? PieceToChar[p] : ' ');
            cout << "| " << c << " ";
        }
        cout << "| " << (int(rank) + 1) << "\n"
             << "+---+---+---+---+---+---+---+---+\n";
    }

    cout << "  a   b   c   d   e   f   g   h\n";
    cout << "Side to move: " << (sideToMove == WHITE ? "w" : "b") << "\n";
    cout << "En passant square: ";
    if (IsOk(epSquare)) {
        cout << epSquare;
    } else {
        cout << "none";
    }
    cout << "\n";
    cout << "Castle permissions: " << (castlingRights & WHITE_OO ? "K" : "-")
         << (castlingRights & WHITE_OOO ? "Q" : "-") << (castlingRights & BLACK_OO ? "k" : "-")
         << (castlingRights & BLACK_OOO ? "q" : "-") << "\n";
    cout << "Position key: " << posKey << "\n";
}

void Board::Perft(int depth) {
    if (depth == 0) {
        perftLealNodes++;
        return;
    }
    MoveList list;
    MoveGen::GeneratePseudoMoves(*this, list);

    for (const auto& move : list) {
        if (!MakeMove(move)) {
            continue;
        }
        Perft(depth - 1);
        UnmakeMove(move);
    }
}

uint64_t Board::PerftTest(int depth) {
    using namespace std::chrono;

    std::cout << "Starting perft test to depth " << depth << "\n";

    perftLealNodes = 0;
    MoveList list;

    const auto start = high_resolution_clock::now();

    MoveGen::GeneratePseudoMoves(*this, list);
    for (const auto& move : list) {
        if (!MakeMove(move)) {
            continue;
        }

        uint64_t before = perftLealNodes;

        Perft(depth - 1);
        UnmakeMove(move);
        std::cout << move << ": " << perftLealNodes - before << "\n";
    }

    const auto stop = high_resolution_clock::now();
    const auto duration = duration_cast<milliseconds>(stop - start).count();

    std::cout << "Total: " << perftLealNodes << " nodes in " << duration << " ms\n\n";

    return duration;
}

} // namespace Zugzwang
