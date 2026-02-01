#include "movegen.h"
#include "bitboard.h"
#include "board.h"
#include "types.h"

namespace Zugzwang {
namespace {

void GenerateSlidingMoves(const Board& board, MoveList& list) {
    const Color side = board.sideToMove;
    const Bitboard occupancy = board.byColorBB[WHITE] | board.byColorBB[BLACK];

    static constexpr Piece pieceIdx[2][3] = { { W_BISHOP, W_ROOK, W_QUEEN }, { B_BISHOP, B_ROOK, B_QUEEN } };

    for (int i = 0; i < 3; i++) {
        const Piece piece = pieceIdx[side][i];
        const int pieceNb = board.pieceNb[piece];

        for (int pieceIdx = 0; pieceIdx < pieceNb; ++pieceIdx) {
            Square startSq = board.pieceList[piece][pieceIdx];

            Bitboard attacks = 0ULL;

            if (i == 0) {
                attacks = Bitboards::GetAttacks<BISHOP>(startSq, occupancy);
            } else if (i == 1) {
                attacks = Bitboards::GetAttacks<ROOK>(startSq, occupancy);
            } else if (i == 2) {
                attacks = Bitboards::GetAttacks<QUEEN>(startSq, occupancy);
            }

            attacks &= ~board.byColorBB[side];

            while (attacks) {
                Square targetSq = PopLsb(attacks);
                list.Insert(Move(startSq, targetSq));
            }
        }
    }
}

void GenerateKnightMoves(const Board& board, MoveList& list) {
    const Color color = board.sideToMove;
    const Piece piece = MakePiece(color, KNIGHT);
    const int pieceNb = board.pieceNb[piece];

    for (int pieceIdx = 0; pieceIdx < pieceNb; ++pieceIdx) {
        Square startSq = board.pieceList[piece][pieceIdx];
        Bitboard attacks = Bitboards::GetAttacks<KNIGHT>(startSq) & ~board.byColorBB[color];

        while (attacks) {
            Square targetSq = PopLsb(attacks);
            list.Insert(Move(startSq, targetSq));
        }
    }
}

void GeneratePawnMoves(const Board& board, MoveList& list) {
    const Color color = board.sideToMove;
    const Piece pawn = MakePiece(color, PAWN);
    const int pieceNb = board.pieceNb[pawn];

    const Rank startRank = RelativeRank(color, RANK_2);
    const Rank promoRank = RelativeRank(color, RANK_7);

    auto add_promotions = [&](Square startSq, Square toSq) {
        list.Insert(Move::Make<PROMOTION>(startSq, toSq, QUEEN));
        list.Insert(Move::Make<PROMOTION>(startSq, toSq, ROOK));
        list.Insert(Move::Make<PROMOTION>(startSq, toSq, BISHOP));
        list.Insert(Move::Make<PROMOTION>(startSq, toSq, KNIGHT));
    };

    for (int pieceIdx = 0; pieceIdx < pieceNb; pieceIdx++) {
        const Square startSq = board.pieceList[pawn][pieceIdx];
        const Rank rank = RankOf(startSq);
        const Square oneForward = startSq + PawnPush(color);

        ASSERT(IsOk(oneForward));

        // pushes
        if (board.pieces[oneForward] == NO_PIECE) {
            if (rank == promoRank) {
                add_promotions(startSq, oneForward);
            } else {
                const Square twoForward = startSq + 2 * PawnPush(color);
                ASSERT(IsOk(twoForward));

                list.Insert(Move(startSq, oneForward));
                if (rank == startRank && board.pieces[twoForward] == NO_PIECE) {
                    list.Insert(Move(startSq, twoForward));
                }
            }
        }

        // captures
        const Bitboard pawnAtt = Bitboards::GetAttacks<PAWN>(startSq, 0, color);
        Bitboard captures = pawnAtt & board.byColorBB[~color];

        while (captures) {
            Square to = PopLsb(captures);
            if (rank == promoRank) {
                add_promotions(startSq, to);
            } else {
                list.Insert(Move(startSq, to));
            }
        }

        // en passant
        if (board.epSquare != SQ_NONE && (pawnAtt & (1ULL << board.epSquare))) {
            list.Insert(Move::Make<EN_PASSANT>(startSq, board.epSquare));
        }
    }
}

void GenerateKingMoves(const Board& board, MoveList& list) {
    const Color color = board.sideToMove;
    Square startSq = board.kingSquare[color];

    Bitboard attacks = Bitboards::GetAttacks<KING>(startSq) & ~board.byColorBB[color];
    while (attacks) {
        Square targetSq = PopLsb(attacks);
        list.Insert(Move(startSq, targetSq));
    }

    // castling
    if (color == WHITE) {
        if (board.castlingRights & WHITE_OO) {
            if (board.pieces[SQ_F1] == NO_PIECE && board.pieces[SQ_G1] == NO_PIECE) {
                if (!MoveGen::IsSquareAttacked(board, SQ_E1, BLACK) &&
                    !MoveGen::IsSquareAttacked(board, SQ_F1, BLACK)) {
                    list.Insert(Move::Make<CASTLING>(SQ_E1, SQ_G1));
                }
            }
        }

        if (board.castlingRights & WHITE_OOO) {
            if (board.pieces[SQ_D1] == NO_PIECE && board.pieces[SQ_C1] == NO_PIECE &&
                board.pieces[SQ_B1] == NO_PIECE) {
                if (!MoveGen::IsSquareAttacked(board, SQ_E1, BLACK) &&
                    !MoveGen::IsSquareAttacked(board, SQ_D1, BLACK)) {
                    list.Insert(Move::Make<CASTLING>(SQ_E1, SQ_C1));
                }
            }
        }
    } else {
        if (board.castlingRights & BLACK_OO) {
            if (board.pieces[SQ_F8] == NO_PIECE && board.pieces[SQ_G8] == NO_PIECE) {
                if (!MoveGen::IsSquareAttacked(board, SQ_E8, WHITE) &&
                    !MoveGen::IsSquareAttacked(board, SQ_F8, WHITE)) {
                    list.Insert(Move::Make<CASTLING>(SQ_E8, SQ_G8));
                }
            }
        }

        if (board.castlingRights & BLACK_OOO) {
            if (board.pieces[SQ_D8] == NO_PIECE && board.pieces[SQ_C8] == NO_PIECE &&
                board.pieces[SQ_B8] == NO_PIECE) {
                if (!MoveGen::IsSquareAttacked(board, SQ_E8, WHITE) &&
                    !MoveGen::IsSquareAttacked(board, SQ_D8, WHITE)) {
                    list.Insert(Move::Make<CASTLING>(SQ_E8, SQ_C8));
                }
            }
        }
    }
}

} // namespace

namespace MoveGen {

bool IsSquareAttacked(const Board& board, Square sq, Color attacker) {
    const Bitboard sqBb = SquareBb(sq);

    Piece piece = MakePiece(attacker, PAWN);
    for (int i = 0; i < board.pieceNb[piece]; ++i) {
        if (Bitboards::GetAttacks<PAWN>(board.pieceList[piece][i], 0, attacker) & sqBb) {
            return true;
        }
    }

    piece = MakePiece(attacker, KNIGHT);
    for (int i = 0; i < board.pieceNb[piece]; ++i) {
        if (Bitboards::GetAttacks<KNIGHT>(board.pieceList[piece][i]) & sqBb) {
            return true;
        }
    }

    if (Bitboards::GetAttacks<KING>(board.kingSquare[attacker]) & sqBb) {
        return true;
    }

    const Bitboard occupied = board.byColorBB[WHITE] | board.byColorBB[BLACK];

    piece = MakePiece(attacker, BISHOP);
    for (int i = 0; i < board.pieceNb[piece]; ++i) {
        if (Bitboards::GetAttacks<BISHOP>(board.pieceList[piece][i], occupied) & sqBb) {
            return true;
        }
    }

    piece = MakePiece(attacker, ROOK);
    for (int i = 0; i < board.pieceNb[piece]; ++i) {
        if (Bitboards::GetAttacks<ROOK>(board.pieceList[piece][i], occupied) & sqBb) {
            return true;
        }
    }

    piece = MakePiece(attacker, QUEEN);
    for (int i = 0; i < board.pieceNb[piece]; ++i) {
        if (Bitboards::GetAttacks<QUEEN>(board.pieceList[piece][i], occupied) & sqBb) {
            return true;
        }
    }

    return false;
}

void GeneratePseudoMoves(const Board& board, MoveList& list) {
    GenerateSlidingMoves(board, list);
    GenerateKnightMoves(board, list);
    GeneratePawnMoves(board, list);
    GenerateKingMoves(board, list);
}

} // namespace MoveGen
} // namespace Zugzwang