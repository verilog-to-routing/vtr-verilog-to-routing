#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

// This initializes variables on the binary 'scores' heap also with
// smallest variable index first (thus picked first) and larger indices at
// the end.
//
void Internal::init_scores (int old_max_var, int new_max_var) {
  LOG ("initializing EVSIDS scores from %d to %d", old_max_var + 1,
       new_max_var);
  for (int i = old_max_var; i < new_max_var; i++)
    scores.push_back (i + 1);
}

// Shuffle the EVSIDS heap.

void Internal::shuffle_scores () {
  if (!opts.shuffle)
    return;
  if (!opts.shufflescores)
    return;
  CADICAL_assert (!level);
  stats.shuffled++;
  LOG ("shuffling scores");
  vector<int> shuffle;
  if (opts.shufflerandom) {
    scores.erase ();
    for (int idx = max_var; idx; idx--)
      shuffle.push_back (idx);
    Random random (opts.seed); // global seed
    random += stats.shuffled;  // different every time
    for (int i = 0; i <= max_var - 2; i++) {
      const int j = random.pick_int (i, max_var - 1);
      swap (shuffle[i], shuffle[j]);
    }
  } else {
    while (!scores.empty ()) {
      int idx = scores.front ();
      (void) scores.pop_front ();
      shuffle.push_back (idx);
    }
  }
  score_inc = 0;
  for (const auto &idx : shuffle) {
    stab[idx] = score_inc++;
    scores.push_back (idx);
  }
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
