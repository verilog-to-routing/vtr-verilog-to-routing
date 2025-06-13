#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

Limit::Limit () { memset (this, 0, sizeof *this); }

/*------------------------------------------------------------------------*/

double Internal::scale (double v) const {
  const double ratio = clause_variable_ratio ();
  const double factor = (ratio <= 2) ? 1.0 : log (ratio) / log (2);
  double res = factor * v;
  if (res < 1)
    res = 1;
  return res;
}

/*------------------------------------------------------------------------*/

Last::Last () { memset (this, 0, sizeof *this); }

/*------------------------------------------------------------------------*/

Inc::Inc () {
  memset (this, 0, sizeof *this);
  decisions = conflicts = -1; // unlimited
}

void Internal::limit_terminate (int l) {
  if (l <= 0 && !lim.terminate.forced) {
    LOG ("keeping unbounded terminate limit");
  } else if (l <= 0) {
    LOG ("reset terminate limit to be unbounded");
    lim.terminate.forced = 0;
  } else {
    lim.terminate.forced = l;
    LOG ("new terminate limit of %d calls", l);
  }
}

void Internal::limit_conflicts (int l) {
  if (l < 0 && inc.conflicts < 0) {
    LOG ("keeping unbounded conflict limit");
  } else if (l < 0) {
    LOG ("reset conflict limit to be unbounded");
    inc.conflicts = -1;
  } else {
    inc.conflicts = l;
    LOG ("new conflict limit of %d conflicts", l);
  }
}

void Internal::limit_decisions (int l) {
  if (l < 0 && inc.decisions < 0) {
    LOG ("keeping unbounded decision limit");
  } else if (l < 0) {
    LOG ("reset decision limit to be unbounded");
    inc.decisions = -1;
  } else {
    inc.decisions = l;
    LOG ("new decision limit of %d decisions", l);
  }
}

void Internal::limit_preprocessing (int l) {
  if (l < 0) {
    LOG ("ignoring invalid preprocessing limit %d", l);
  } else if (!l) {
    LOG ("reset preprocessing limit to no preprocessing");
    inc.preprocessing = 0;
  } else {
    inc.preprocessing = l;
    LOG ("new preprocessing limit of %d preprocessing rounds", l);
  }
}

void Internal::limit_local_search (int l) {
  if (l < 0) {
    LOG ("ignoring invalid local search limit %d", l);
  } else if (!l) {
    LOG ("reset local search limit to no local search");
    inc.localsearch = 0;
  } else {
    inc.localsearch = l;
    LOG ("new local search limit of %d local search rounds", l);
  }
}

bool Internal::is_valid_limit (const char *name) {
  if (!strcmp (name, "terminate"))
    return true;
  if (!strcmp (name, "conflicts"))
    return true;
  if (!strcmp (name, "decisions"))
    return true;
  if (!strcmp (name, "preprocessing"))
    return true;
  if (!strcmp (name, "localsearch"))
    return true;
  return false;
}

bool Internal::limit (const char *name, int l) {
  bool res = true;
  if (!strcmp (name, "terminate"))
    limit_terminate (l);
  else if (!strcmp (name, "conflicts"))
    limit_conflicts (l);
  else if (!strcmp (name, "decisions"))
    limit_decisions (l);
  else if (!strcmp (name, "preprocessing"))
    limit_preprocessing (l);
  else if (!strcmp (name, "localsearch"))
    limit_local_search (l);
  else
    res = false;
  return res;
}

void Internal::reset_limits () {
  LOG ("reset limits");
  limit_terminate (0);
  limit_conflicts (-1);
  limit_decisions (-1);
  limit_preprocessing (0);
  limit_local_search (0);
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
