#include <iostream>
#include <string>
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "precomputed_move_data.h"

using namespace ChessCpp;

int main() {
    Precomputed::init();
    Board::init_zobrist();

    const std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    const std::string TEST_FEN = "r3kq1r/8/8/8/8/8/8/R3K2R w KQkq - 0 1";

    Board board;
    MoveList list;
    board.set(START_FEN);
    board.print();

    MoveGen::generate_pseudo_moves(board, list);
    std::cout << "\n" << list << "\n";

    for (Square sq = SQ_A1; sq <= SQ_H8; ++sq) {
        std::cout << MoveGen::is_square_attacked(board, sq, WHITE) << " ";
        if (file_of(sq) == FILE_H) {
            std::cout << "\n";
        }
    }
    
    std::cin.get();
    return 0;
}
