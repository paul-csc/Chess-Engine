#include "bitboard.h"
#include "uci.h"

int main(int argc, char** argv) {
    using namespace Zugzwang;

    Bitboards::init();
    UCIEngine uci(argc, argv);
    uci.Loop();
    return 0;
}
