#include <iostream>
#include <algorithm>
#include <sstream>
#include <string>
#include <chrono>
#include <cctype>
#include "board.h"
#include "movegen.h"
#include "types.h"

namespace ChessCpp {
class MoveGen;

namespace {
    constexpr std::string_view PieceToChar(" PNBRQK  pnbrqk");
    constexpr int CastlePerm[64] = {
        13, 15, 15, 15, 12, 15, 15, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 7,  15, 15, 15, 3,  15, 15, 11,
    };

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

    for (int i = 0; i < 13; ++i) {
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

void Board::updateListsBitboards() {
    for (Square sq = SQ_A1; sq < SQUARE_NB; ++sq) {
        Piece piece = pieces[sq];

        if (piece != NO_PIECE) {
            Color color = color_of(piece);
            
            set_bit(byColorBB[color], sq);
            pieceList[piece][pieceNb[piece]++] = sq;
        }
    }
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
    updateListsBitboards();
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

bool Board::do_move(const Move& move) {
    const Square from = move.from_sq();
    const Square to = move.to_sq();

    // save state
    history[gamePly].posKey = posKey;
    history[gamePly].rule50 = rule50;
    history[gamePly].epSquare = epSquare;
    history[gamePly].castlingRights = castlingRights;
    history[gamePly].captured = pieces[to];  // normal captures only; en-passant handled separately

    // remove old EP & castling from hash
    if (epSquare != SQ_NONE)
        posKey ^= Zobrist::psq[NO_PIECE][epSquare];
    posKey ^= Zobrist::castling[castlingRights];

    // special move handling
    if (move.type_of() == EN_PASSANT) {
        // remove the captured pawn (behind 'to')
        remove_piece(to + (sideToMove == WHITE ? SOUTH : NORTH));
        rule50 = 0;  // reset 50-move on capture
    } else if (move.type_of() == CASTLING) {
        switch (to) {
        case SQ_C1: move_piece(SQ_A1, SQ_D1); break;
        case SQ_C8: move_piece(SQ_A8, SQ_D8); break;
        case SQ_G1: move_piece(SQ_H1, SQ_F1); break;
        case SQ_G8: move_piece(SQ_H8, SQ_F8); break;
        default: ASSERT(false);
        }
    }

    // normal capture handling (if a piece sits on 'to')
    if (pieces[to] != NO_PIECE) {
        remove_piece(to);
        rule50 = 0;
    } else if (move.type_of() != EN_PASSANT) {
        // only increment rule50 if it wasn't a capture (en-passant already set to 0)
        rule50++;
    }

    // move the piece
    move_piece(from, to);

    // promotion handling
    if (move.type_of() == PROMOTION) {
        remove_piece(to);
        put_piece(make_piece(sideToMove, move.promotion_type()), to);
    }

    // update king square (if moved)
    if (type_of(pieces[to]) == KING)
        kingSquare[sideToMove] = to;

    // new en-passant target (from a double pawn push)
    epSquare = SQ_NONE;
    if (type_of(pieces[to]) == PAWN && std::abs(rank_of(from) - rank_of(to)) == 2) {
        epSquare = from + (sideToMove == WHITE ? NORTH : SOUTH);
    }
    if (epSquare != SQ_NONE)
        posKey ^= Zobrist::psq[NO_PIECE][epSquare];  // add new EP key

    // update castling rights and re-add castling key
    castlingRights &= CastlePerm[from];
    castlingRights &= CastlePerm[to];
    posKey ^= Zobrist::castling[castlingRights];

    // flip side
    sideToMove = ~sideToMove;
    posKey ^= Zobrist::side;

    gamePly++;

    if (MoveGen::is_square_attacked(*this, kingSquare[~sideToMove], sideToMove)) {
        undo_move(move);
        return false;
    }
    return true;
}

void Board::undo_move(const Move& move) {
    gamePly--;

    const Square from = move.from_sq();
    const Square to = move.to_sq();

    sideToMove = ~sideToMove;

    // Undo special moves first where needed
    if (move.type_of() == EN_PASSANT) {
        // the captured pawn belongs to the opponent of the mover
        put_piece(make_piece(~sideToMove, PAWN), to + (sideToMove == WHITE ? SOUTH : NORTH));
    } else if (move.type_of() == CASTLING) {
        switch (to) {
        case SQ_C1: move_piece(SQ_D1, SQ_A1); break;
        case SQ_C8: move_piece(SQ_D8, SQ_A8); break;
        case SQ_G1: move_piece(SQ_F1, SQ_H1); break;
        case SQ_G8: move_piece(SQ_F8, SQ_H8); break;
        default: ASSERT(false);
        }
    }

    move_piece(to, from);

    if (move.type_of() == PROMOTION) {
        remove_piece(from);
        put_piece(make_piece(sideToMove, PAWN), from);
    }

    if (history[gamePly].captured != NO_PIECE) {
        put_piece(history[gamePly].captured, to);
    }

    if (type_of(pieces[from]) == KING)
        kingSquare[sideToMove] = from;

    epSquare = history[gamePly].epSquare;
    rule50 = history[gamePly].rule50;
    castlingRights = history[gamePly].castlingRights;
    posKey = history[gamePly].posKey;
}

void Board::perft(int depth) {
    if (depth == 0) {
        perftLealNodes++;
        return;
    }
    MoveList list;
    MoveGen::generate_pseudo_moves(*this, list);
    for (int i = 0; i < list.count; ++i) {
        if (!do_move(list.moves[i])) {
            continue;
        }
        perft(depth - 1);
        undo_move(list.moves[i]);
    }
}

void Board::perftTest(int depth) {
    print();
    std::cout << "Starting perft test to depth " << depth << "\n";

    perftLealNodes = 0;
    MoveList list;

    using namespace std::chrono;
    auto start = high_resolution_clock::now();

    MoveGen::generate_pseudo_moves(*this, list);
    for (int i = 0; i < list.count; ++i) {
        Move move = list.moves[i];
        if (!do_move(move)) {
            continue;
        }
        unsigned long long before = perftLealNodes;
        perft(depth - 1);
        undo_move(move);
        std::cout << move << ": " << perftLealNodes - before << std::endl;
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start).count();

    std::cout << "Total: " << perftLealNodes << " nodes in " << duration << " ms" << std::endl;
}
}  // namespace ChessCpp
