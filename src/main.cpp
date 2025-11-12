#include <iostream>
#include "types.h"
#include "board.h"
#include "movegen.h"
#include "precomputed_move_data.h"

using namespace ChessCpp;

int main() {
    Precomputed::init();
    Board::init_zobrist();

    Board board;
    board.set(START_FEN);
    board.perftTest(5);
    
    std::cin.get();
    return 0;
}
