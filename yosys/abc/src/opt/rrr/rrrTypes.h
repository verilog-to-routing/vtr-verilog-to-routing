#pragma once

#include <vector>
#include <chrono>

ABC_NAMESPACE_CXX_HEADER_START

namespace rrr {

  enum NodeType {
    PI,
    PO,
    AND,
    XOR,
    LUT
  };

  enum SatResult {
    SAT,
    UNSAT,
    UNDET
  };

  enum VarValue: char {
    UNDEF,
    TRUE,
    FALSE,
    TEMP_TRUE,
    TEMP_FALSE
  };

  enum ActionType {
    NONE,
    REMOVE_FANIN,
    REMOVE_UNUSED,
    REMOVE_BUFFER,
    REMOVE_CONST,
    ADD_FANIN,
    TRIVIAL_COLLAPSE,
    TRIVIAL_DECOMPOSE,
    SORT_FANINS,
    SAVE,
    LOAD,
    POP_BACK,
    INSERT
  };

  struct Action {
    ActionType type = NONE;
    int id = -1;
    int idx = -1;
    int fi = -1;
    bool c = false;
    std::vector<int> vFanins;
    std::vector<int> vIndices;
    std::vector<int> vFanouts;
  };

  using seconds = int64_t;
  using clock_type = std::chrono::steady_clock;
  using time_point = std::chrono::time_point<clock_type>;

}

ABC_NAMESPACE_CXX_HEADER_END
