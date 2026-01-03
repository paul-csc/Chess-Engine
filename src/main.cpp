#include "board.h"
#include "types.h"
#include <iostream>

namespace Zugzwang {

static bool IsMoveStr(const std::string& str) {
    auto IsFileValid = [](char ch) { return ch >= 'a' && ch <= 'h'; };
    auto IsRankValid = [](char ch) { return ch >= '1' && ch <= '8'; };
    auto IsPromoValid = [](char ch) { return ch == 'q' || ch == 'r' || ch == 'b' || ch == 'n'; };

    if (str.size() != 4 && str.size() != 5) {
        return false;
    }
    if (!IsFileValid(str[0]) || !IsRankValid(str[1]) || !IsFileValid(str[2]) || !IsRankValid(str[3])) {
        return false;
    }
    if (str[0] == str[2] && str[1] == str[3]) { // same from and to square
        return false;
    }
    if (str.size() == 5 && !IsPromoValid(str[4])) {
        return false;
    }
    return true;
}

static Move ParseMove(std::string& str, Board& board) {
    Square from = MakeSquare(File(str[0] - 'a'), Rank(str[1] - '1'));
    Square to = MakeSquare(File(str[2] - 'a'), Rank(str[3] - '1'));

    MoveList list;
    MoveGen::GeneratePseudoMoves(board, list);

    for (int i = 0; i < list.Count; ++i) {
        Move& move = list.Moves[i];
        if (move.FromSq() == from && move.ToSq() == to) {
            if (move.TypeOf() == PROMOTION) {
                PieceType type = move.PromotionType();
                if ((type == KNIGHT && str[4] == 'n') || (type == ROOK && str[4] == 'r') ||
                    (type == BISHOP && str[4] == 'b') || (type == QUEEN && str[4] == 'q')) {
                    return move;
                }
                continue;
            }
            return move;
        }
    }
    return Move::None();
}

} // namespace Zugzwang

int main() {
    using namespace Zugzwang;

    Magics::Init();
    Board::InitZobrist();

    Board board;
    MoveList list;

    constexpr auto TEST_FEN = "r1bq2r1/b4pk1/p1pp1p2/1p2pP2/1P2P1PB/3P4/1PPQ2P1/R3K2R w 0 1";
    board.ParseFen(START_FEN);

    std::string input;

    board.Print();
    while (true) {
        do {
            std::cout << "> ";
            std::getline(std::cin, input);
        } while (input.empty());

        if (input[0] == 'q') {
            break;
        } else if (input[0] == 't') {
            board.UnmakeMove();
            board.Print();
        } else if (input[0] == 'p') {
            board.PerftTest(5);
        } else if (IsMoveStr(input)) {
            Move move = ParseMove(input, board);
            if (move == Move::None()) {
                std::cout << "Illegal move\n";
                continue;
            }
            board.MakeMove(move);
            board.Print();
        } else {
            std::cout << "This is not a move\n";
        }
    };

    return 0;
}
