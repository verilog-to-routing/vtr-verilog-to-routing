#ifndef _vivify_hpp_INCLUDED
#define _vivify_hpp_INCLUDED

#include "global.h"

#include "util.hpp"

#include <array>
#include <cstdint>
#include <vector>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

struct Clause;

enum class Vivify_Mode { TIER1, TIER2, TIER3, IRREDUNDANT };

#define COUNTREF_COUNTS 2

struct vivify_ref {
  bool vivify;
  std::size_t size;
  uint64_t count[COUNTREF_COUNTS];
  Clause *clause;
};

// In the vivifier structure, we put the schedules in an array in order to
// be able to iterate over them, but we provide the reference to them to
// make sure that you do need to remember the order.
struct Vivifier {
  std::array<std::vector<vivify_ref>, 4> refs_schedules;
  std::vector<vivify_ref> &refs_schedule_tier1, &refs_schedule_tier2,
      &refs_schedule_tier3, &refs_schedule_irred;
  std::array<std::vector<Clause *>, 4> schedules;
  std::vector<Clause *> &schedule_tier1, &schedule_tier2, &schedule_tier3,
      &schedule_irred;
  std::vector<int> sorted;
  Vivify_Mode tier;
  char tag;
  int tier1_limit;
  int tier2_limit;
  int64_t ticks;
  std::vector<std::tuple<int, Clause *, bool>> lrat_stack;
  Vivifier (Vivify_Mode mode_tier)
      : refs_schedule_tier1 (refs_schedules[0]),
        refs_schedule_tier2 (refs_schedules[1]),
        refs_schedule_tier3 (refs_schedules[2]),
        refs_schedule_irred (refs_schedules[3]),
        schedule_tier1 (schedules[0]), schedule_tier2 (schedules[1]),
        schedule_tier3 (schedules[2]), schedule_irred (schedules[3]),
        tier (mode_tier) {}

  void erase () {
    erase_vector (refs_schedule_tier1);
    erase_vector (refs_schedule_tier2);
    erase_vector (refs_schedule_tier3);
    erase_vector (refs_schedule_irred);
    erase_vector (sorted);
  }
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
