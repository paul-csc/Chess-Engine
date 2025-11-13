#include <iostream>
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "bitboard.h"

using namespace ChessCpp;

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    Magics::init();
    Board::init_zobrist();

    Board board;
    MoveList list;

    board.set(START_FEN);
    board.perftTest(6);
    // board.do_move(Move(SQ_B1, SQ_C3));
    // board.print();
    // MoveGen::generate_pseudo_moves(board, list);
    // std::cout << list;

    std::cin.get();
    return 0;
}
