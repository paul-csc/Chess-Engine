#pragma once
#include "board.h"
#include <sstream>

namespace Zugzwang {

class UCIEngine {
  public:
    UCIEngine(int argc, char** argv);
    void Loop();

  private:
    void go(std::istringstream& is);
    void position(std::istringstream& is);

    static bool isMoveStr(std::string_view str);
    Move parseMove(std::string_view str) const;

    Board board;
};

} // namespace Zugzwang