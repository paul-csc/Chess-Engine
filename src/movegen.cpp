#include <iostream>
#include "types.h"
#include "board.h"
#include "precomputed_move_data.h"
#include "movegen.h"

namespace ChessCpp {
namespace {
    constexpr Direction DirectionOffsets[8] = {NORTH,      SOUTH,      WEST,       EAST,
                                               NORTH_WEST, SOUTH_EAST, NORTH_EAST, SOUTH_WEST};
    constexpr int PawnAttackDirs[2][2] = {
        {4, 6},
        {5, 7}
    };
}

void MoveGen::generate_sliding_moves(const Board& board, Square startSq, MoveList& list) {
    const Piece piece = board.pieces[startSq];
    const int startDirIdx = (type_of(piece) == BISHOP ? 4 : 0);
    const int endDirIdx = (type_of(piece) == ROOK ? 4 : 8);

    for (int dirIdx = startDirIdx; dirIdx < endDirIdx; ++dirIdx) {
        for (int n = 0; n < Precomputed::NumSquaresToEdge[startSq][dirIdx]; n++) {
            Square targetSq = startSq + (n + 1) * DirectionOffsets[dirIdx];
            Piece targetPiece = board.pieces[targetSq];
            ASSERT(is_ok(targetSq));

            if (targetPiece != NO_PIECE && color_of(targetPiece) == color_of(piece)) {
                break;
            }
            list.moves[list.count++] = Move(startSq, targetSq);
            if (targetPiece != NO_PIECE && color_of(targetPiece) == ~color_of(piece)) {
                break;
            }
        }
    }
}

void MoveGen::generate_king_moves(const Board& board, Square startSq, MoveList& list) {
    const Piece piece = board.pieces[startSq];
    const Color color = color_of(piece);

    for (int dirIdx = 0; dirIdx < 8; ++dirIdx) {
        if (Precomputed::NumSquaresToEdge[startSq][dirIdx] > 0) {
            Square targetSq = startSq + DirectionOffsets[dirIdx];
            Piece targetPiece = board.pieces[targetSq];
            ASSERT(is_ok(targetSq));

            if (targetPiece == NO_PIECE || color_of(targetPiece) != color) {
                list.moves[list.count++] = Move(startSq, targetSq);
            }
        }
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

void MoveGen::generate_knight_moves(const Board& board, Square startSq, MoveList& list) {
    for (int knightMoveIdx = 0; knightMoveIdx < 8; ++knightMoveIdx) {
        Square targetSq = Precomputed::KnightMoves[startSq][knightMoveIdx];

        if (targetSq == SQ_NONE)
            continue;

        Piece targetPiece = board.pieces[targetSq];
        if (targetPiece == NO_PIECE || color_of(targetPiece) != board.sideToMove) {
            list.moves[list.count++] = Move(startSq, targetSq);
        }
    }
}

void MoveGen::generate_pawn_moves(const Board& board, Square startSq, MoveList& list) {
    const Piece piece = board.pieces[startSq];
    const Color color = color_of(piece);
    const Rank rank = rank_of(startSq);
    const Rank startRank = relative_rank(color, RANK_2);
    const Rank promoRank = relative_rank(color, RANK_7);
    const Square oneForward = startSq + pawn_push(color);
    const Square twoForward = startSq + 2 * pawn_push(color);

    // pushes
    if (board.pieces[oneForward] == NO_PIECE) {
        if (rank == promoRank) {
            list.moves[list.count++] = Move::make<PROMOTION>(startSq, oneForward, KNIGHT);
            list.moves[list.count++] = Move::make<PROMOTION>(startSq, oneForward, BISHOP);
            list.moves[list.count++] = Move::make<PROMOTION>(startSq, oneForward, ROOK);
            list.moves[list.count++] = Move::make<PROMOTION>(startSq, oneForward, QUEEN);
        } else {
            list.moves[list.count++] = Move(startSq, oneForward);
            if (rank == startRank && board.pieces[twoForward] == NO_PIECE) {
                list.moves[list.count++] = Move(startSq, twoForward);
            }
        }
    }

    // captures
    for (int j = 0; j < 2; ++j) {
        if (Precomputed::NumSquaresToEdge[startSq][PawnAttackDirs[color][j]] > 0) {
            Square targetSq = startSq + DirectionOffsets[PawnAttackDirs[color][j]];
            Piece targetPiece = board.pieces[targetSq];

            if (targetPiece != NO_PIECE && color_of(targetPiece) == ~color) {
                if (rank == promoRank) {
                    list.moves[list.count++] = Move::make<PROMOTION>(startSq, targetSq, KNIGHT);
                    list.moves[list.count++] = Move::make<PROMOTION>(startSq, targetSq, BISHOP);
                    list.moves[list.count++] = Move::make<PROMOTION>(startSq, targetSq, ROOK);
                    list.moves[list.count++] = Move::make<PROMOTION>(startSq, targetSq, QUEEN);
                } else {
                    list.moves[list.count++] = Move(startSq, targetSq);
                }
            }

            // en passant
            if (targetSq == board.epSquare) {
                list.moves[list.count++] = Move::make<EN_PASSANT>(startSq, targetSq);
            }
        }
    }
}

bool MoveGen::is_square_attacked(const Board& board, Square sq, Color attacker) {
    // pawns
    for (int j = 0; j < 2; ++j) {
        // look from the opposite pawn attack direction
        Direction dir = Direction(PawnAttackDirs[~attacker][j]);
        if (Precomputed::NumSquaresToEdge[sq][dir] > 0) {
            if (board.pieces[sq + DirectionOffsets[dir]] == make_piece(attacker, PAWN)) {
                return true;
            }
        }
    }

    // kings
    for (int dirIdx = 0; dirIdx < 8; ++dirIdx) {
        if (Precomputed::NumSquaresToEdge[sq][dirIdx] > 0) {
            Square targetSq = sq + DirectionOffsets[dirIdx];
            ASSERT(is_ok(targetSq));
            Piece attackerPiece = board.pieces[targetSq];

            if (attackerPiece == make_piece(attacker, KING)) {
                return true;
            }
        }
    }

    // knights
    for (int knightMoveIdx = 0; knightMoveIdx < 8; ++knightMoveIdx) {
        Square targetSq = Precomputed::KnightMoves[sq][knightMoveIdx];

        if (targetSq != SQ_NONE && board.pieces[targetSq] == make_piece(attacker, KNIGHT)) {
            return true;
        }
    }

    // rooks, queens
    for (int dirIdx = 0; dirIdx < 4; dirIdx++) {
        for (int n = 0; n < Precomputed::NumSquaresToEdge[sq][dirIdx]; n++) {
            Square targetSq = sq + (n + 1) * DirectionOffsets[dirIdx];
            ASSERT(is_ok(targetSq));
            Piece targetPiece = board.pieces[targetSq];

            if (targetPiece == make_piece(attacker, ROOK)
                || targetPiece == make_piece(attacker, QUEEN)) {
                return true;
            } else if (targetPiece != NO_PIECE) {
                break;
            }
        }
    }

    // bishop, queens
    for (int dirIdx = 4; dirIdx < 8; dirIdx++) {
        for (int n = 0; n < Precomputed::NumSquaresToEdge[sq][dirIdx]; n++) {
            Square targetSq = sq + (n + 1) * DirectionOffsets[dirIdx];
            ASSERT(is_ok(targetSq));
            Piece targetPiece = board.pieces[targetSq];

            if (targetPiece == make_piece(attacker, BISHOP)
                || targetPiece == make_piece(attacker, QUEEN)) {
                return true;
            } else if (targetPiece != NO_PIECE) {
                break;
            }
        }
    }

    return false;
}

void MoveGen::generate_pseudo_moves(const Board& board, MoveList& list) {
    const Color color = board.sideToMove;
    for (PieceType i = BISHOP; i <= QUEEN; ++i) {
        for (int pieceIdx = 0; pieceIdx < board.pieceNb[make_piece(color, i)]; pieceIdx++) {
            generate_sliding_moves(board, board.pieceList[make_piece(color, i)][pieceIdx], list);
        }
    }

    for (int pieceIdx = 0; pieceIdx < board.pieceNb[make_piece(color, KNIGHT)]; pieceIdx++) {
        generate_knight_moves(board, board.pieceList[make_piece(color, KNIGHT)][pieceIdx], list);
    }

    for (int pieceIdx = 0; pieceIdx < board.pieceNb[make_piece(color, PAWN)]; pieceIdx++) {
        generate_pawn_moves(board, board.pieceList[make_piece(color, PAWN)][pieceIdx], list);
    }

    generate_king_moves(board, board.kingSquare[color], list);
}

void MoveGen::generate_legal_moves(const Board& board, MoveList& list) {
    generate_pseudo_moves(board, list);

    for (int i = 0; i < list.count; ++i) { }
}
}  // namespace ChessCpp
