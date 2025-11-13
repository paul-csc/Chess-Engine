#include <iostream>
#include "types.h"
#include "board.h"
#include "bitboard.h"
#include "movegen.h"

namespace ChessCpp {
void MoveGen::generate_sliding_moves(const Board& board, MoveList& list) {
    const Color side = board.sideToMove;

    static constexpr Piece pieceIdx[2][3] = {
        {W_BISHOP, W_ROOK, W_QUEEN},
        {B_BISHOP, B_ROOK, B_QUEEN}
    };

    for (int i = 0; i < 3; i++) {
        for (int pieceNum = 0; pieceNum < board.pieceNb[pieceIdx[side][i]]; ++pieceNum) {
            Square startSq = board.pieceList[pieceIdx[side][i]][pieceNum];

            Bitboard attacks = 0ULL;
            Bitboard occupancy = board.byColorBB[WHITE] | board.byColorBB[BLACK];

            if (i == 0) {
                attacks = Magics::getBishopAttacks(startSq, occupancy);
            } else if (i == 1) {
                attacks = Magics::getRookAttacks(startSq, occupancy);
            } else if (i == 2) {
                attacks = Magics::getBishopAttacks(startSq, occupancy)
                        | Magics::getRookAttacks(startSq, occupancy);
            }

            attacks &= ~board.byColorBB[side];

            while (attacks) {
                Square targetSq = pop_lsb(attacks);
                list.moves[list.count++] = Move(startSq, targetSq);
            }
        }
    }
}

void MoveGen::generate_king_moves(const Board& board, MoveList& list) {
    const Color color = board.sideToMove;
    Square startSq = board.kingSquare[color];

    Bitboard attacks = Precomputed::KingAttacks[startSq] & ~board.byColorBB[color];
    while (attacks) {
        Square targetSq = pop_lsb(attacks);
        list.moves[list.count++] = Move(startSq, targetSq);
    }

    // castling
    if (color == WHITE) {
        if (board.castlingRights & WHITE_OO) {
            if (board.pieces[SQ_F1] == NO_PIECE && board.pieces[SQ_G1] == NO_PIECE) {
                if (!is_square_attacked(board, SQ_E1, BLACK)
                    && !is_square_attacked(board, SQ_F1, BLACK)) {
                    list.moves[list.count++] = Move::make<CASTLING>(SQ_E1, SQ_G1);
                }
            }
        }

        if (board.castlingRights & WHITE_OOO) {
            if (board.pieces[SQ_D1] == NO_PIECE && board.pieces[SQ_C1] == NO_PIECE
                && board.pieces[SQ_B1] == NO_PIECE) {
                if (!is_square_attacked(board, SQ_E1, BLACK)
                    && !is_square_attacked(board, SQ_D1, BLACK)) {
                    list.moves[list.count++] = Move::make<CASTLING>(SQ_E1, SQ_C1);
                }
            }
        }
    } else {
        if (board.castlingRights & BLACK_OO) {
            if (board.pieces[SQ_F8] == NO_PIECE && board.pieces[SQ_G8] == NO_PIECE) {
                if (!is_square_attacked(board, SQ_E8, WHITE)
                    && !is_square_attacked(board, SQ_F8, WHITE)) {
                    list.moves[list.count++] = Move::make<CASTLING>(SQ_E8, SQ_G8);
                }
            }
        }

        if (board.castlingRights & BLACK_OOO) {
            if (board.pieces[SQ_D8] == NO_PIECE && board.pieces[SQ_C8] == NO_PIECE
                && board.pieces[SQ_B8] == NO_PIECE) {
                if (!is_square_attacked(board, SQ_E8, WHITE)
                    && !is_square_attacked(board, SQ_D8, WHITE)) {
                    list.moves[list.count++] = Move::make<CASTLING>(SQ_E8, SQ_C8);
                }
            }
        }
    }
}

void MoveGen::generate_knight_moves(const Board& board, MoveList& list) {
    const Color color = board.sideToMove;

    for (int pieceNum = 0; pieceNum < board.pieceNb[make_piece(color, KNIGHT)]; ++pieceNum) {
        Square startSq = board.pieceList[make_piece(color, KNIGHT)][pieceNum];
        Bitboard attacks = Precomputed::KnightAttacks[startSq] & ~board.byColorBB[color];

        while (attacks) {
            Square targetSq = pop_lsb(attacks);
            list.moves[list.count++] = Move(startSq, targetSq);
        }
    }
}

void MoveGen::generate_pawn_moves(const Board& board, MoveList& list) {
    const Color color = board.sideToMove;
    const Rank startRank = relative_rank(color, RANK_2);
    const Rank promoRank = relative_rank(color, RANK_7);

    for (int pieceIdx = 0; pieceIdx < board.pieceNb[make_piece(color, PAWN)]; pieceIdx++) {
        const Square startSq = board.pieceList[make_piece(color, PAWN)][pieceIdx];
        const Rank rank = rank_of(startSq);
        const Square oneForward = startSq + pawn_push(color);
        ASSERT(is_ok(oneForward));
        // pushes
        if (board.pieces[oneForward] == NO_PIECE) {
            if (rank == promoRank) {
                list.moves[list.count++] = Move::make<PROMOTION>(startSq, oneForward, KNIGHT);
                list.moves[list.count++] = Move::make<PROMOTION>(startSq, oneForward, BISHOP);
                list.moves[list.count++] = Move::make<PROMOTION>(startSq, oneForward, ROOK);
                list.moves[list.count++] = Move::make<PROMOTION>(startSq, oneForward, QUEEN);
            } else {
                const Square twoForward = startSq + 2 * pawn_push(color);
                ASSERT(is_ok(twoForward));

                list.moves[list.count++] = Move(startSq, oneForward);
                if (rank == startRank && board.pieces[twoForward] == NO_PIECE) {
                    list.moves[list.count++] = Move(startSq, twoForward);
                }
            }
        }

        // captures
        Bitboard attacks = Precomputed::PawnAttacks[color][startSq] & board.byColorBB[~color];
        while (attacks) {
            Square to = pop_lsb(attacks);
            if (rank == promoRank) {
                list.moves[list.count++] = Move::make<PROMOTION>(startSq, to, KNIGHT);
                list.moves[list.count++] = Move::make<PROMOTION>(startSq, to, BISHOP);
                list.moves[list.count++] = Move::make<PROMOTION>(startSq, to, ROOK);
                list.moves[list.count++] = Move::make<PROMOTION>(startSq, to, QUEEN);
            } else {
                list.moves[list.count++] = Move(startSq, to);
            }
        }

        // en passant
        if (board.epSquare != SQ_NONE
            && (Precomputed::PawnAttacks[color][startSq] & (1ULL << board.epSquare))) {
            list.moves[list.count++] = Move::make<EN_PASSANT>(startSq, board.epSquare);
        }
    }
}

bool MoveGen::is_square_attacked(const Board& board, Square sq, Color attacker) {
    Bitboard sqBb = square_bb(sq);

    for (int i = 0; i < board.pieceNb[make_piece(attacker, PAWN)]; ++i) {
        if (Precomputed::PawnAttacks[attacker][board.pieceList[make_piece(attacker, PAWN)][i]]
            & sqBb)
            return true;
    }

    for (int i = 0; i < board.pieceNb[make_piece(attacker, KNIGHT)]; ++i) {
        if (Precomputed::KnightAttacks[board.pieceList[make_piece(attacker, KNIGHT)][i]] & sqBb)
            return true;
    }

    if (Precomputed::KingAttacks[board.kingSquare[attacker]] & sqBb) {
        return true;
    }

    Bitboard occupied = board.byColorBB[WHITE] | board.byColorBB[BLACK];
    for (int i = 0; i < board.pieceNb[BISHOP + attacker * 8]; ++i) {
        if (Magics::getBishopAttacks(board.pieceList[BISHOP + attacker * 8][i], occupied) & sqBb)
            return true;
    }

    for (int i = 0; i < board.pieceNb[ROOK + attacker * 8]; ++i) {
        if (Magics::getRookAttacks(board.pieceList[ROOK + attacker * 8][i], occupied) & sqBb)
            return true;
    }

    for (int i = 0; i < board.pieceNb[QUEEN + attacker * 8]; ++i) {
        if ((Magics::getRookAttacks(board.pieceList[QUEEN + attacker * 8][i], occupied)
             | Magics::getBishopAttacks(board.pieceList[QUEEN + attacker * 8][i], occupied))
            & sqBb)
            return true;
    }
    return false;
}

void MoveGen::generate_pseudo_moves(const Board& board, MoveList& list) {
    generate_pawn_moves(board, list);
    generate_sliding_moves(board, list);
    generate_knight_moves(board, list);
    generate_king_moves(board, list);
}

void MoveGen::generate_legal_moves(const Board& board, MoveList& list) {
    generate_pseudo_moves(board, list);

    for (int i = 0; i < list.count; ++i) { }
}
}  // namespace ChessCpp
