#pragma once
#include "board.h"
#include <sstream>

namespace Zugzwang {

class UCIEngine {
  public:
    UCIEngine(int argc, char** argv);
    void Loop();

  private:
    void Go(std::istringstream& is);
    void Position(std::istringstream& is);

    bool IsMoveStr(std::string_view str);
    Move ParseMove(std::string_view str) const;

    Board board;
};

} // namespace Zugzwang