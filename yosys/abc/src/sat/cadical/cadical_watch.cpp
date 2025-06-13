#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

void Internal::init_watches () {
  CADICAL_assert (wtab.empty ());
  if (wtab.size () < 2 * vsize)
    wtab.resize (2 * vsize, Watches ());
  LOG ("initialized watcher tables");
}

void Internal::clear_watches () {
  for (auto lit : lits)
    watches (lit).clear ();
}

void Internal::reset_watches () {
  CADICAL_assert (!wtab.empty ());
  erase_vector (wtab);
  LOG ("reset watcher tables");
}

// This can be quite costly since lots of memory is accessed in a rather
// random fashion, and thus we optionally profile it.

void Internal::connect_watches (bool irredundant_only) {
  START (connect);
  CADICAL_assert (watching ());

  LOG ("watching all %sclauses", irredundant_only ? "irredundant " : "");

  // First connect binary clauses.
  //
  for (const auto &c : clauses) {
    if (irredundant_only && c->redundant)
      continue;
    if (c->garbage || c->size > 2)
      continue;
    watch_clause (c);
  }

  // Then connect non-binary clauses.
  //
  for (const auto &c : clauses) {
    if (irredundant_only && c->redundant)
      continue;
    if (c->garbage || c->size == 2)
      continue;
    watch_clause (c);
    if (!level) {
      const int lit0 = c->literals[0];
      const int lit1 = c->literals[1];
      const signed char tmp0 = val (lit0);
      const signed char tmp1 = val (lit1);
      if (tmp0 > 0)
        continue;
      if (tmp1 > 0)
        continue;
      if (tmp0 < 0) {
        const size_t pos0 = var (lit0).trail;
        if (pos0 < propagated) {
          propagated = pos0;
          LOG ("literal %d resets propagated to %zd", lit0, pos0);
        }
      }
      if (tmp1 < 0) {
        const size_t pos1 = var (lit1).trail;
        if (pos1 < propagated) {
          propagated = pos1;
          LOG ("literal %d resets propagated to %zd", lit1, pos1);
        }
      }
    }
  }

  STOP (connect);
}

// This can be quite costly since lots of memory is accessed in a rather
// random fashion, and thus we optionally profile it.

void Internal::connect_binary_watches () {
  START (connect);
  CADICAL_assert (watching ());

  LOG ("watching binary clauses");

  // First connect binary clauses.
  //
  for (const auto &c : clauses) {
    if (c->garbage || c->size > 2)
      continue;
    watch_clause (c);
  }

  STOP (connect);
}

void Internal::sort_watches () {
  CADICAL_assert (watching ());
  LOG ("sorting watches");
  Watches saved;
  for (auto lit : lits) {
    Watches &ws = watches (lit);

    const const_watch_iterator end = ws.end ();
    watch_iterator j = ws.begin ();
    const_watch_iterator i;

    CADICAL_assert (saved.empty ());

    for (i = j; i != end; i++) {
      const Watch w = *i;
      if (w.binary ())
        *j++ = w;
      else
        saved.push_back (w);
    }

    std::copy (saved.cbegin (), saved.cend (), j);

    saved.clear ();
  }
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
