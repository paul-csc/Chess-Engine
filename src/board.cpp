#include <iostream>
#include <algorithm>
#include <sstream>
#include <string>
#include <cctype>
#include "board.h"
#include "types.h"

namespace ChessCpp {
namespace {
    constexpr std::string_view PieceToChar(" PNBRQK  pnbrqk");

    uint64_t seed = 1804289383ULL;
    inline uint64_t rand64() {
        uint64_t x = seed;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        seed = x;
        return x * 2685821657736338717ULL;
    }
}

namespace Zobrist {
    Key psq[PIECE_NB][SQUARE_NB];
    Key castling[CASTLING_RIGHT_NB];
    Key side;
}  // namespace Zobrist

void Board::init_zobrist() {
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

void Board::generatePosKey() {
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

void Board::reset() {
    for (Square i = SQ_A1; i < SQUARE_NB; ++i) {
        pieces[i] = NO_PIECE;
    }

    kingSquare[WHITE] = kingSquare[BLACK] = SQ_NONE;
    sideToMove = WHITE;
    epSquare = SQ_NONE;
    rule50 = 0;
    gamePly = 0;
    castlingRights = NO_CASTLING;
    posKey = 0ULL;
}

void Board::set(const std::string& fenStr) {
    reset();

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
            pieces[sq] = Piece(idx);
            if (Piece(idx) == W_KING)
                kingSquare[WHITE] = sq;
            if (Piece(idx) == B_KING)
                kingSquare[BLACK] = sq;
            ++sq;
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
    if (((ss >> col) && (col >= 'a' && col <= 'h'))
        && ((ss >> row) && (row == (sideToMove == WHITE ? '6' : '3')))) {
        epSquare = make_square(File(col - 'a'), Rank(row - '1'));
    }

    // 5. Halfmove clock (rule50)
    ss >> std::skipws >> rule50 >> gamePly;

    // Convert from fullmove starting from 1 to internal ply count
    gamePly = std::max(2 * (gamePly - 1), 0) + (sideToMove == BLACK);

    generatePosKey();
}

void Board::print() const {
    using std::cout;

    cout << "+---+---+---+---+---+---+---+---+\n";
    for (Rank rank = RANK_8; rank >= RANK_1; --rank) {
        for (File file = FILE_A; file <= FILE_H; ++file) {  // <= not <
            Square sq = make_square(file, rank);
            Piece p = pieces[sq];
            char c = (p != NO_PIECE ? PieceToChar[p] : ' ');
            cout << "| " << c << " ";
        }
        cout << "| " << (int(rank) + 1) << "\n"
             << "+---+---+---+---+---+---+---+---+\n";

        if (rank == RANK_1)
            break;  // prevent underflow since rank is unsigned
    }

    cout << "  a   b   c   d   e   f   g   h\n";
    cout << "Side to move: " << (sideToMove == WHITE ? "w" : "b") << "\n";
    cout << "En passant square: ";
    if (is_ok(epSquare))
        cout << epSquare;
    else
        cout << "none";
    cout << "\n";
    cout << "Castle permissions: " << (castlingRights & WHITE_OO ? "K" : "-")
         << (castlingRights & WHITE_OOO ? "Q" : "-") << (castlingRights & BLACK_OO ? "k" : "-")
         << (castlingRights & BLACK_OOO ? "q" : "-") << "\n";
    cout << "Position key: " << posKey << "\n";
}
}  // namespace ChessCpp
