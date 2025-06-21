#include "global.h"

#include "congruence.hpp"
#include "internal.hpp"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <utility>
#include <vector>

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

Closure::Closure (Internal *i)
    : internal (i), table (128, Hash (nonces))
#ifdef LOGGING
      ,
      fresh_id (internal->clause_id)
#endif
{
}

char &Closure::lazy_propagated (int lit) {
  return lazy_propagated_idx[internal->vidx (lit)];
}

void update_ite_flags (Gate *g) {
  int8_t f = g->degenerated_ite;
  const int lhs = g->lhs;
  const int cond = g->rhs [0];
  const int then_lit = g->rhs[1];
  const int else_lit = g->rhs[2];

  if (lhs == cond) {
    f |= Special_ITE_GATE::NO_NEG_THEN;
    f |= Special_ITE_GATE::NO_PLUS_ELSE;
  }
  if (lhs == -cond) {
    f |= Special_ITE_GATE::NO_PLUS_THEN;
    f |= Special_ITE_GATE::NO_NEG_ELSE;
  }
  if (lhs == then_lit) {
    f |= Special_ITE_GATE::NO_PLUS_THEN;
    f |= Special_ITE_GATE::NO_NEG_THEN;
  }
  if (lhs == else_lit) {
    f |= Special_ITE_GATE::NO_PLUS_ELSE;
    f |= Special_ITE_GATE::NO_NEG_ELSE;
  }
  g->degenerated_ite = f;
  CADICAL_assert (lhs != -then_lit);
  CADICAL_assert (lhs != -else_lit);
  CADICAL_assert (cond != then_lit);
  CADICAL_assert (cond != else_lit);
  CADICAL_assert (cond != -then_lit);
  CADICAL_assert (cond != -else_lit);
}

void check_correct_ite_flags (const Gate *const g) {
#ifndef CADICAL_NDEBUG
  const int8_t f = g->degenerated_ite;
  const int lhs = g->lhs;
  const int cond = g->rhs [0];
  const int then_lit = g->rhs[1];
  const int else_lit = g->rhs[2];
  CADICAL_assert (g->pos_lhs_ids.size () == 4);
  if (g->pos_lhs_ids[0].clause == nullptr)
    CADICAL_assert ((f & Special_ITE_GATE::NO_PLUS_THEN));
  if (g->pos_lhs_ids[1].clause == nullptr)
    CADICAL_assert (f & Special_ITE_GATE::NO_NEG_THEN);
  if (g->pos_lhs_ids[2].clause == nullptr)
    CADICAL_assert (f & Special_ITE_GATE::NO_PLUS_ELSE);
  if (g->pos_lhs_ids[3].clause == nullptr)
    CADICAL_assert (f & Special_ITE_GATE::NO_NEG_ELSE);
  if (lhs == cond) {
    CADICAL_assert (f & Special_ITE_GATE::NO_NEG_THEN);
    CADICAL_assert (f & Special_ITE_GATE::NO_PLUS_ELSE);
  }
  if (lhs == -cond) {
    CADICAL_assert (f & Special_ITE_GATE::NO_PLUS_THEN);
    CADICAL_assert (f & Special_ITE_GATE::NO_NEG_ELSE);
  }
  if (lhs == then_lit) {
    CADICAL_assert (f & Special_ITE_GATE::NO_PLUS_THEN);
    CADICAL_assert (f & Special_ITE_GATE::NO_NEG_THEN);
  }
  if (lhs == else_lit) {
    CADICAL_assert (f & Special_ITE_GATE::NO_PLUS_ELSE);
    CADICAL_assert (f & Special_ITE_GATE::NO_NEG_ELSE);
  }
  CADICAL_assert (lhs != -then_lit);
  CADICAL_assert (lhs != -else_lit);
  CADICAL_assert (cond != then_lit);
  CADICAL_assert (cond != else_lit);
  CADICAL_assert (cond != -then_lit);
  CADICAL_assert (cond != -else_lit);
#else
  (void)g;
#endif
}

/*------------------------------------------------------------------------*/

static size_t hash_lits (std::array<int, 16> &nonces,
                         const vector<int> &lits) {
  size_t hash = 0;
  const auto end_nonces = end (nonces);
  const auto begin_nonces = begin (nonces);
  auto n = begin_nonces;
  for (auto lit : lits) {
    hash += lit;
    hash *= *n++;
    hash = (hash << 4) | (hash >> 60);
    if (n == end_nonces)
      n = begin_nonces;
  }
  hash ^= hash >> 32;
  return hash;
}

size_t Hash::operator() (const Gate *const g) const {
  CADICAL_assert (hash_lits (nonces, g->rhs) == g->hash);
  return g->hash;
}

bool gate_contains (Gate *g, int lit) {
  return find (begin (g->rhs), end (g->rhs), lit) != end (g->rhs);
}
/*------------------------------------------------------------------------*/
struct compact_binary_rank {
  typedef uint64_t Type;
  uint64_t operator() (const CompactBinary &a) {
    return ((uint64_t) a.lit1 << 32) + a.lit2;
  };
};

struct compact_binary_order {
  bool operator() (const CompactBinary &a, const CompactBinary &b) {
    return compact_binary_rank () (a) < compact_binary_rank () (b);
  };
};

bool Closure::find_binary (int lit, int other) const {
  const auto offsize =
      offsetsize[internal->vlit (lit)]; // in C++17: [offset, size] =
  const auto offset = offsize.first;
  const auto size = offsize.second;
  const auto begin = std::begin (binaries) + offset;
  const auto end = std::begin (binaries) + size;
  CADICAL_assert (end <= std::end (binaries));
  const CompactBinary target = CompactBinary (nullptr, 0, lit, other);
  auto it = std::lower_bound (begin, end, target, compact_binary_order ());
  // search_binary only returns a bool
  bool found = (it != end && it->lit1 == lit && it->lit2 == other);
  if (found) {
    LOG ("found binary [%zd] %d %d", it->id, lit, other);
    if (internal->lrat)
      lrat_chain.push_back (it->id);
  }
  return found;
}

void Closure::extract_binaries () {
  if (!internal->opts.congruencebinaries)
    return;
  START (extractbinaries);
  offsetsize.resize (internal->max_var * 2 + 3, make_pair (0, 0));

  // in kissat this is done during watch clearing. TODO: consider doing this
  // too.
  for (Clause *c : internal->clauses) {
    if (c->garbage)
      continue;
    if (c->redundant && c->size != 2)
      continue;
    if (c->size > 2)
      continue;
    CADICAL_assert (c->size == 2);
    const int lit = c->literals[0];
    const int other = c->literals[1];
    const bool already_sorted =
        internal->vlit (lit) < internal->vlit (other);
    binaries.push_back (CompactBinary (c, c->id,
                                       already_sorted ? lit : other,
                                       already_sorted ? other : lit));
  }

  MSORT (internal->opts.radixsortlim, begin (binaries), end (binaries),
         compact_binary_rank (), compact_binary_order ());

  {
    const size_t size = binaries.size ();
    size_t i = 0;
    while (i < size) {
      CompactBinary bin = binaries[i];
      const int lit = bin.lit1;
      size_t j = i;
      while (j < size && binaries[j].lit1 == lit) {
        ++j;
      }
      CADICAL_assert (j >= i);
      CADICAL_assert (j <= size);
      offsetsize[internal->vlit (lit)] = make_pair (i, j);
      i = j;
    }
  }

  size_t extracted = 0, already_present = 0, duplicated = 0;

  const size_t size = internal->clauses.size ();
  for (size_t i = 0; i < size; ++i) {
    Clause *d = internal->clauses[i]; // binary clauses are appended, so
                                      // reallocation possible
    if (d->garbage)
      continue;
    if (d->redundant)
      continue;
    if (d->size != 3)
      continue;
    const int *lits = d->literals;
    const int a = lits[0];
    const int b = lits[1];
    const int c = lits[2]; // obfuscating d->literals[2] which triggers an error in pedandic mode
    if (internal->val (a))
      continue;
    if (internal->val (b))
      continue;
    if (internal->val (c))
      continue;
    int l = 0, k = 0;
    if (find_binary (-a, b) || find_binary (-a, c)) {
      l = b, k = c;
    } else if (find_binary (-b, a) || find_binary (-b, c)) {
      l = a, k = c;
    } else if (find_binary (-c, a) || find_binary (-c, b)) {
      l = a, k = b;
    } else
      continue;
    LOG (d, "strengthening");
    if (!find_binary (l, k)) {
      if (internal->lrat)
        lrat_chain.push_back (d->id);
      add_binary_clause (l, k);
      ++extracted;
    } else {
      ++already_present;
      if (internal->lrat)
        lrat_chain.clear ();
    }
  }
  lrat_chain.clear ();

  offsetsize.clear ();

  // kissat has code to remove duplicates, which we have already removed
  // before starting congruence
  MSORT (internal->opts.radixsortlim, begin (binaries), end (binaries),
         compact_binary_rank (), compact_binary_order ());
  const size_t new_size = binaries.size ();
  {
    size_t i = 0;
    for (size_t j = 1; j < new_size; ++j) {
      CADICAL_assert (i < j);
      if (binaries[i].lit1 == binaries[j].lit1 &&
          binaries[i].lit2 == binaries[j].lit2) {
        // subsuming later clause
        subsume_clause (binaries[i].clause,
                        binaries[j].clause); // the local one is specialized
        ++duplicated;
      } else {
        binaries[++i] = binaries[j];
      }
    }
    CADICAL_assert (i <= new_size);
    binaries.resize (i);
  }
  binaries.clear ();
  STOP (extractbinaries);
  LOG ("extracted %zu binaries (plus %zu already present and %zu "
       "duplicates)",
       extracted, already_present, duplicated);
}

/*------------------------------------------------------------------------*/
// marking structure for congruence closure, by reference
signed char &Closure::marked (int lit) {
  CADICAL_assert (internal->vlit (lit) < marks.size ());
  return marks[internal->vlit (lit)];
}

void Closure::unmark_all () {
  for (auto lit : internal->analyzed) {
    CADICAL_assert (marked (lit));
    marked (lit) = 0;
  }
  internal->analyzed.clear ();
}

void Closure::set_mu1_reason (int lit, Clause *c) {
  CADICAL_assert (marked (lit) & 1);
  LOG (c, "mu1 %d -> %zd", lit, c->id);
  mu1_ids[internal->vlit (lit)] = LitClausePair (lit, c);
}

void Closure::set_mu2_reason (int lit, Clause *c) {
  CADICAL_assert (marked (lit) & 2);
  if (!internal->lrat)
    return;
  LOG (c, "mu2 %d -> %zd", lit, c->id);
  mu2_ids[internal->vlit (lit)] = LitClausePair (lit, c);
}

void Closure::set_mu4_reason (int lit, Clause *c) {
  CADICAL_assert (marked (lit) & 4);
  if (!internal->lrat)
    return;
  LOG (c, "mu4 %d -> %zd", lit, c->id);
  mu4_ids[internal->vlit (lit)] = LitClausePair (lit, c);
}

LitClausePair Closure::marked_mu1 (int lit) {
  return mu1_ids[internal->vlit (lit)];
}

LitClausePair Closure::marked_mu2 (int lit) {
  return mu2_ids[internal->vlit (lit)];
}

LitClausePair Closure::marked_mu4 (int lit) {
  return mu4_ids[internal->vlit (lit)];
}

struct sort_literals_by_var_rank {
  CaDiCaL::Internal *internal;
  sort_literals_by_var_rank (Internal *i) : internal (i) {}

  typedef uint64_t Type;

  Type operator() (const int &a) const { return internal->vlit (a); }
};
struct sort_literals_by_var_rank_except {
  CaDiCaL::Internal *internal;
  int lhs;
  int except;
  sort_literals_by_var_rank_except (Internal *i, int my_lhs, int except2)
      : internal (i), lhs (my_lhs), except (except2) {}
  sort_literals_by_var_rank_except (Internal *i, int my_lhs)
      : internal (i), lhs (my_lhs), except (0) {}
  typedef uint64_t Type;
  Type operator() (const int &a) const {
    Type res = 0;
    if (abs (a) == abs (except))
      res = 1 - (a > 0);
    else if (abs (a) == abs (lhs))
      res = 3 - (a > 0);
    else
      res = internal->vlit (a) + 2; // probably +2 enough
    return ~res;
  }
};

struct sort_literals_by_var_smaller_except {
  CaDiCaL::Internal *internal;
  int lhs;
  int except;
  sort_literals_by_var_smaller_except (Internal *i, int my_lhs, int except2)
      : internal (i), lhs (my_lhs), except (except2) {}
  sort_literals_by_var_smaller_except (Internal *i, int my_lhs)
      : internal (i), lhs (my_lhs), except (0) {}
  bool operator() (const int &a, const int &b) const {
    return sort_literals_by_var_rank_except (internal, lhs, except) (a) <
           sort_literals_by_var_rank_except (internal, lhs, except) (b);
    if (abs (a) == abs (except) && abs (b) != abs (except))
      return false;
    if (abs (a) != abs (except) && abs (b) == abs (except))
      return true;
    if (abs (a) == abs (lhs) && abs (b) != abs (lhs))
      return false;
    if (abs (a) != abs (lhs) && abs (b) == abs (lhs))
      return true;
    return sort_literals_by_var_rank (internal) (a) >
           sort_literals_by_var_rank (internal) (b);
  }
};
struct sort_literals_by_var_smaller {
  CaDiCaL::Internal *internal;
  sort_literals_by_var_smaller (Internal *i) : internal (i) {}
  bool operator() (const int &a, const int &b) const {
    return sort_literals_by_var_rank (internal) (a) <
           sort_literals_by_var_rank (internal) (b);
  }
};

void Closure::sort_literals_by_var_except (vector<int> &rhs, int lhs,
                                           int except2) {
  MSORT (internal->opts.radixsortlim, begin (rhs), end (rhs),
         sort_literals_by_var_rank_except (internal, lhs, except2),
         sort_literals_by_var_smaller_except (internal, lhs, except2));
}
void Closure::sort_literals_by_var (vector<int> &rhs) {
  MSORT (internal->opts.radixsortlim, begin (rhs), end (rhs),
         sort_literals_by_var_rank (internal),
         sort_literals_by_var_smaller (internal));
}
/*------------------------------------------------------------------------*/
int &Closure::representative (int lit) {
  CADICAL_assert (internal->vlit (lit) < representant.size ());
  return representant[internal->vlit (lit)];
}
int Closure::representative (int lit) const {
  CADICAL_assert (internal->vlit (lit) < representant.size ());
  return representant[internal->vlit (lit)];
}

int &Closure::eager_representative (int lit) {
  CADICAL_assert (internal->vlit (lit) < eager_representant.size ());
  return eager_representant[internal->vlit (lit)];
}

int Closure::eager_representative (int lit) const {
  CADICAL_assert (internal->vlit (lit) < eager_representant.size ());
  return eager_representant[internal->vlit (lit)];
}

int Closure::find_lrat_representative_with_marks (int lit) {
  int res = lit;
  int nxt = lit;
  do {
    res = nxt;
    nxt = representative (nxt);
    if (nxt != res) {
      LOG ("%d has reason %" PRIu64, res, representative_id (res));
      lrat_chain.push_back (representative_id (res));
    }
  } while (nxt != res || marked (nxt) || marked (-nxt));

  return nxt;
}
int Closure::find_representative (int lit) {
  int res = lit;
  int nxt = lit;
  do {
    res = nxt;
    nxt = representative (nxt);
  } while (nxt != res);

  return res;
}

int Closure::find_representative_and_compress (int lit, bool update_eager) {
  LOG ("finding representative of %d", lit);
  int res = lit;
  int nxt = lit;
  int path_length = 0;
  do {
    res = nxt;
    nxt = representative (nxt);
    ++path_length;
    LOG ("updating %d -> %d", res, nxt);
  } while (nxt != res);

  if (path_length > 2) {
    LOG ("learning new rewriting from %d to %d (current path length: %d)",
         lit, res, path_length);
    if (update_eager)
      eager_representative (lit) = res;
    if (internal->lrat) {
      produce_representative_lrat (lit);
      Clause *equiv = add_tmp_binary_clause (-lit, res);

      if (equiv) {
        representative_id (lit) = equiv->id;
        if (update_eager)
          eager_representative_id (lit) = equiv->id;
      }
    }
    if (internal->lrat)
      lrat_chain.clear ();
  } else if (path_length == 2) {
    if (update_eager) {
      LOG ("updating information %d -> %d in eager", lit, res);
      eager_representative (lit) = res;
      if (internal->lrat)
        eager_representative_id (lit) = representative_id (lit);
      CADICAL_assert (!internal->lrat || eager_representative_id (lit));
    }
  }

  if (lit != res) {
    representative (lit) = res;
  }
  LOG ("representative of %d is %d", lit, res);
  return res;
}

void Closure::push_lrat_unit (int lit) {
  if (!internal->lrat)
    return;
  CADICAL_assert (internal->val (lit) > 0);
  LRAT_ID id = internal->unit_id (lit);
  lrat_chain.push_back (id);
}

int Closure::find_eager_representative (int lit) {
  int res = lit;
  int nxt = lit;
  do {
    res = nxt;
    nxt = eager_representative (nxt);
  } while (nxt != res);

  return res;
}

int Closure::find_eager_representative_and_compress (int lit) {
  if (!internal->lrat)
    return find_representative (lit);
  int res = lit;
  int nxt = lit;
  int path_length = 0;
  do {
    res = nxt;
    nxt = eager_representative (nxt);
    ++path_length;
  } while (nxt != res);

  //  CADICAL_assert (res == find_representative (lit));
  // we have to do path compression to support LRAT proofs
  if (path_length > 2) {
    LOG ("learning new rewriting from %d to %d (current path length: %d)",
         lit, res, path_length);
    std::vector<LRAT_ID> tmp_lrat_chain;
    if (internal->lrat) {
      tmp_lrat_chain = std::move (lrat_chain);
      lrat_chain.clear ();
      produce_eager_representative_lrat (lit);
    }
    eager_representative (lit) = res;
    Clause *equiv = add_tmp_binary_clause (-lit, res);
    equiv->hyper = true;

    if (internal->lrat && equiv) {
      eager_representative_id (lit) = equiv->id;
    }
    if (internal->lrat) {
      lrat_chain = std::move (tmp_lrat_chain);
    }
  } else if (path_length == 2) {
    LOG ("duplicated information %d -> %d to eager with clause %" PRIu64,
         lit, res, eager_representative_id (lit));
    CADICAL_assert (eager_representative (lit) == res);
    CADICAL_assert (!internal->lrat || eager_representative_id (lit));
  }
  CADICAL_assert (internal->clause.empty ());
  return res;
}

void Closure::find_representative_and_compress_both (int lit) {
  find_representative_and_compress (lit, false);
  find_representative_and_compress (-lit, false);
}

void Closure::import_lazy_and_find_eager_representative_and_compress_both (
    int lit) {
  find_representative_and_compress (lit);
  find_eager_representative_and_compress (lit);
  find_representative_and_compress (-lit);
  find_eager_representative_and_compress (-lit);
}

void Closure::produce_representative_lrat (int lit) {
  CADICAL_assert (internal->lrat);
  LOG ("production of LRAT chain for %d with representative %" PRIu64, lit,
       representative_id (lit));
  CADICAL_assert (internal->lrat);
  CADICAL_assert (lrat_chain.empty ());
  int res = lit;
  int nxt = lit;
  CADICAL_assert (nxt != representative (nxt));
  do {
    res = nxt;
    nxt = representative (nxt);
    if (nxt != res) {
      LOG ("%d has reason %" PRIu64, res, representative_id (res));
      lrat_chain.push_back (representative_id (res));
    }
  } while (nxt != res);
}

void Closure::produce_eager_representative_lrat (int lit) {
  CADICAL_assert (internal->lrat);
  LOG ("production of LRAT chain for %d with representative %" PRIu64, lit,
       eager_representative_id (lit));
  CADICAL_assert (internal->lrat);
  CADICAL_assert (lrat_chain.empty ());
  int res = lit;
  int nxt = lit;
  CADICAL_assert (nxt != eager_representative (nxt));
  do {
    res = nxt;
    nxt = eager_representative (nxt);
    if (nxt != res) {
      LOG ("%d has reason %" PRIu64, res, eager_representative_id (res));
      lrat_chain.push_back (eager_representative_id (res));
    }
  } while (nxt != res);
}

LRAT_ID Closure::find_representative_lrat (int lit) {
  if (!internal->lrat)
    return 0;
  int res = lit;
#ifndef CADICAL_NDEBUG
  int nxt = representative (res);
  CADICAL_assert (nxt == representative (res));
#endif
  LOG ("checking for existing LRAT chain for %d with clause %" PRIu64, lit,
       eager_representative_id (res));
  CADICAL_assert (representative_id (res));
  return representative_id (res);
}

LRAT_ID Closure::find_eager_representative_lrat (int lit) {
  if (!internal->lrat)
    return 0;
  int res = lit;
#ifndef CADICAL_NDEBUG
  int nxt = eager_representative (res);
  CADICAL_assert (nxt == eager_representative (res));
#endif
  LOG ("checking for existing LRAT chain for %d with clause %" PRIu64, lit,
       eager_representative_id (res));
  CADICAL_assert (eager_representative_id (res));
  return eager_representative_id (res);
}

LRAT_ID &Closure::eager_representative_id (int lit) {
  CADICAL_assert (internal->vlit (lit) < eager_representant_id.size ());
  return eager_representant_id[internal->vlit (lit)];
}
LRAT_ID Closure::eager_representative_id (int lit) const {
  CADICAL_assert (internal->vlit (lit) < eager_representant_id.size ());
  return eager_representant_id[internal->vlit (lit)];
}

LRAT_ID &Closure::representative_id (int lit) {
  CADICAL_assert (internal->vlit (lit) < representant_id.size ());
  return representant_id[internal->vlit (lit)];
}
LRAT_ID Closure::representative_id (int lit) const {
  CADICAL_assert (internal->vlit (lit) < representant_id.size ());
  return representant_id[internal->vlit (lit)];
}

void Closure::mark_garbage (Gate *g) {
  LOG (g, "marking as garbage");
  CADICAL_assert (!g->garbage);
  g->garbage = true;
  garbage.push_back (g);
}

bool Closure::remove_gate (GatesTable::iterator git) {
  CADICAL_assert (git != end (table));
  CADICAL_assert (!internal->unsat);
  (*git)->indexed = false;
  LOG ((*git), "removing from hash table");
  table.erase (git);
  return true;
}

bool Closure::remove_gate (Gate *g) {
  if (!g->indexed)
    return false;
  CADICAL_assert (!internal->unsat);
  CADICAL_assert (table.find (g) != end (table));
  table.erase (table.find (g));
  g->indexed = false;
  LOG (g, "removing from hash table");
  return true;
}

void Closure::index_gate (Gate *g) {
  CADICAL_assert (!g->indexed);
  CADICAL_assert (!internal->unsat);
  CADICAL_assert (g->arity () > 1);
  CADICAL_assert (g->hash == hash_lits (nonces, g->rhs));
  LOG (g, "adding to hash table");
  table.insert (g);
  g->indexed = true;
}

void Closure::produce_rewritten_clause_lrat_and_clean (
    std::vector<LitClausePair> &litIds, int except_lhs, bool remove_units) {
  CADICAL_assert (internal->lrat_chain.empty ());
  for (auto &litId : litIds) {
    CADICAL_assert (litId.clause);
    litId.clause = produce_rewritten_clause_lrat (litId.clause, except_lhs,
                                                  remove_units);
    litId.current_lit = find_eager_representative (litId.current_lit);
  }
  litIds.erase (
      std::remove_if (begin (litIds), end (litIds),
                      [] (const LitClausePair &p) { return !p.clause; }),
      end (litIds));
}

void Closure::produce_rewritten_clause_lrat_and_clean (
    std::vector<LitClausePair> &litIds, int except_lhs,
    size_t &old_position1, size_t &old_position2, bool remove_units) {
  CADICAL_assert (internal->lrat_chain.empty ());
  CADICAL_assert (old_position1 != old_position2);
  size_t j = 0;
  for (size_t i = 0; i < litIds.size (); ++i) {
    CADICAL_assert (j <= i);
    litIds[j].clause = produce_rewritten_clause_lrat (
        litIds[i].clause, except_lhs, remove_units);
    litIds[j].current_lit =
        find_eager_representative (litIds[i].current_lit);
    if (i == old_position1) {
      if (litIds[j].clause)
        old_position1 = j;
      else
        old_position1 = -1;
    }
    if (i == old_position2) {
      if (litIds[j].clause)
        old_position2 = j;
      else
        old_position2 = -1;
    }
    if (litIds[j].clause)
      ++j;
  }
  litIds.resize (j);
}

void Closure::compute_rewritten_clause_lrat_simple (Clause *c, int except) {
  CADICAL_assert (internal->clause.empty ());
  CADICAL_assert (internal->lrat_chain.empty ());
  CADICAL_assert (clause.empty ());
  CADICAL_assert (lrat_chain.empty ());
  bool changed = false;
  bool tautology = false;
  for (auto lit : *c) {
    LOG ("checking if %d is required", lit);
    if (internal->marked2 (lit)) {
      continue;
    }
    if (internal->marked2 (-lit)) {
      tautology = true;
      break;
    }
    if (lit == except || lit == -except) {
      internal->mark2 (lit);
      clause.push_back (lit);
      continue;
    }
    if (internal->val (lit) < 0) {
#if 1
      LOG ("found unit %d, removing it", -lit);
      LRAT_ID id = internal->unit_id (-lit);
      lrat_chain.push_back (id);
      changed = true;
      continue;
#else
      LOG ("found unit %d, but ignoring it", -lit);
#endif
    }
    if (internal->val (lit) > 0) {
      LOG ("found positive unit, so clause is subsumed by unit");
      tautology = true;
    }
    const int other = find_eager_representative_and_compress (lit);
    const bool marked = internal->marked2 (other);
    const bool neg_marked = internal->marked2 (-other);
    if (!marked)
      internal->mark2 (other);
    if (neg_marked) {
      tautology = true;
      LOG ("tautology due to %d -> %d", lit, other);
    } else if (lit == other && marked) {
      changed = true;
      LOG ("%d -> %d already in", lit, other);
    } else if (lit != other) {
      if (!marked)
        clause.push_back (other);
      changed = true;
      lrat_chain.push_back (eager_representative_id (lit));
    } else if (!marked)
      clause.push_back (lit);
  }

  for (auto lit : *c) {
    internal->unmark (lit);
  }

  for (auto lit : clause) {
    internal->unmark (lit);
  }

  lrat_chain.push_back (c->id);
  if (tautology) {
    LOG ("generated clause is a tautology");
    lrat_chain.clear ();
    clause.clear ();
  } else if (changed && clause.size () == 1) {
    LOG (lrat_chain, "LRAT chain");
  } else {
    LOG (c, "oops this should not happen");
    CADICAL_assert (false);
  }
}

Clause *Closure::new_tmp_clause (std::vector<int> &clause) {
  CADICAL_assert (internal->lrat);
  CADICAL_assert (!clause.empty ());
  CADICAL_assert (!lrat_chain.empty ());
  bool clear = false;

  LOG (clause, "learn new tmp clause");
  CADICAL_assert (clause.size () >= 2);
  internal->external->check_learned_clause ();

  CADICAL_assert (internal->clause.size () <= (size_t) INT_MAX);
  const int size = (int) clause.size ();
  CADICAL_assert (size >= 2);

  size_t bytes = Clause::bytes (size);
  Clause *c = (Clause *) new char[bytes];
  DeferDeleteArray<char> clause_delete ((char *) c);

  c->id = ++internal->clause_id;

  c->conditioned = false;
  c->covered = false;
  c->enqueued = false;
  c->frozen = false;
  c->garbage = false;
  c->gate = false;
  c->hyper = false;
  c->instantiated = false;
  c->moved = false;
  c->reason = false;
  c->redundant = false;
  c->transred = false;
  c->subsume = false;
  c->swept = false;
  c->flushed = false;
  c->vivified = false;
  c->vivify = false;
  c->used = 0;

  c->glue = size;
  c->size = size;
  c->pos = 2;

  for (int i = 0; i < size; i++)
    c->literals[i] = clause[i];

  // Just checking that we did not mess up our sophisticated memory layout.
  // This might be compiler dependent though. Crucial for correctness.
  //
  CADICAL_assert (c->bytes () == bytes);

  clause_delete.release ();
  LOG (c, "new pointer %p", (void *) c);

  if (clear)
    clause.clear ();

  if (internal->proof) {
    internal->proof->add_derived_clause (c, lrat_chain);
  }
  extra_clauses.push_back (c);
  CADICAL_assert (internal->lrat_chain.empty ());
  return c;
}

Clause *Closure::new_clause () {
  CADICAL_assert (internal->clause.empty () || clause.empty ());
  bool clear = false;
  if (internal->clause.empty ()) {
    swap (internal->clause, clause);
    clear = true;
  }

  Clause *c = internal->new_clause (false);

  if (clear)
    internal->clause.clear ();

  if (internal->proof) {
    internal->proof->add_derived_clause (c, lrat_chain);
  }

  return c;
}

// TODO we here duplicate the arguments of push_id_and_rewriting_lrat but we
// probably do not need that.
void Closure::produce_rewritten_clause_lrat (
    std::vector<LitClausePair> &litIds, int except_lhs, bool remove_units) {
  CADICAL_assert (internal->lrat_chain.empty ());
  for (auto &litId : litIds) {
    if (!litId.clause)
      continue;
    litId.clause = produce_rewritten_clause_lrat (litId.clause, except_lhs,
                                                  remove_units);
    litId.current_lit = find_eager_representative (litId.current_lit);
  }
}

Clause *Closure::produce_rewritten_clause_lrat (Clause *c, int except_lhs,
                                                bool remove_units, bool fail_on_unit) {
  CADICAL_assert (internal->clause.empty ());
  CADICAL_assert (internal->lrat_chain.empty ());
  auto tmp_lrat (std::move (lrat_chain));
  lrat_chain.clear ();
  LOG (c, "rewriting clause for LRAT proof, except for rewriting %d",
       except_lhs);
  CADICAL_assert (internal->clause.empty ());
  CADICAL_assert (lrat_chain.empty ());
  bool changed = false;
  bool tautology = false;
  for (auto lit : *c) {
    LOG ("checking if %d is required", lit);
    if (internal->marked2 (lit)) {
      continue;
    }
    if (internal->marked2 (-lit)) {
      tautology = true;
      break;
    }
    if (lit == except_lhs || lit == -except_lhs) {
      internal->mark2 (lit);
      clause.push_back (lit);
      continue;
    }
    if (internal->val (lit) < 0) {
      if (remove_units || lazy_propagated (lit)) {
        LOG ("found unit %d, removing it", -lit);
        LRAT_ID id = internal->unit_id (-lit);
        lrat_chain.push_back (id);
        changed = true;
        continue;
      } else
        LOG ("found unit %d, but ignoring it", -lit);
    }
    if (internal->val (lit) > 0) {
      LOG ("found positive unit %d, so clause is subsumed by unit", lit);
      if (remove_units || lazy_propagated (lit))
        tautology = true;
    }
    const int other = find_eager_representative_and_compress (lit);
    const bool marked = internal->marked2 (other);
    const bool neg_marked = internal->marked2 (-other);
    if (!marked)
      internal->mark2 (other);
    if (neg_marked) {
      tautology = true;
      LOG ("tautology due to %d -> %d", lit, other);
    } else if (lit == other && marked) {
      changed = true;
      LOG ("%d -> %d already in", lit, other);
    } else if (lit != other) {
      if (!marked)
        clause.push_back (other);
      changed = true;
      lrat_chain.push_back (eager_representative_id (lit));
    } else if (!marked)
      clause.push_back (lit);
  }

  for (auto lit : *c) {
    internal->unmark (lit);
  }

  for (auto lit : clause) {
    internal->unmark (lit);
  }

  lrat_chain.push_back (c->id);
  Clause *d;
  CADICAL_assert (internal->clause.empty ());
  if (tautology) {
    LOG ("generated clause is a tautology");
    d = nullptr;
    lrat_chain.clear ();
  } else if (changed && clause.size () == 1) {
    LOG (lrat_chain, "LRAT chain");
    if (fail_on_unit) {
      d = nullptr;
      CADICAL_assert (false && "rewriting produced a unit clause");
    } else {
      d = c;
    }
  } else if (changed) {
    LOG (lrat_chain, "LRAT Chain");
    d = new_tmp_clause (clause);
    LOG (d, "rewritten clause to");
  } else {
    LOG (c, "clause is unchanged, so giving up");
    lrat_chain.clear ();
    d = c;
  }
  clause.clear ();
  lrat_chain = std::move (tmp_lrat);
  CADICAL_assert (internal->clause.empty ());
  return d;
}

void Closure::push_id_on_chain (std::vector<LRAT_ID> &chain,
                                Rewrite rewrite, int lit) {
  LOG ("adding reason %zd for rewriting %d marked",
       lit == rewrite.src ? rewrite.id1 : rewrite.id2, lit);
  chain.push_back (lit == rewrite.src ? rewrite.id1 : rewrite.id2);
}

void Closure::push_id_and_rewriting_lrat_unit (Clause *c, Rewrite rewrite1,
                                               std::vector<LRAT_ID> &chain,
                                               bool insert_id_after,
                                               Rewrite rewrite2,
                                               int except_lhs,
                                               int except_lhs2) {
  LOG (c,
       "computing normalized LRAT chain for clause to produce unit, "
       "rewriting except for %d (%" PRIu64 ", %" PRIu64 ") and %d (%" PRIu64
       ", %" PRIu64 ") and skipping %d and %d",
       rewrite1.src, rewrite1.id1, rewrite1.id2, rewrite2.src, rewrite2.id1,
       rewrite2.id2, except_lhs, except_lhs2);
  CADICAL_assert (c);
  std::vector<LRAT_ID> units, rewriting;
  for (auto other : *c) {
    // unclear how to achieve this in the simplify context where other ==
    // g->lhs might be set CADICAL_assert (internal->val (other) <= 0 || other ==
    // except);
    if (other == except_lhs || other == -except_lhs) {
      // do nothing;
    } else if (other == except_lhs2 || other == -except_lhs2) {
      // do nothing;
    } else if (internal->val (other) < 0) {
      LOG ("found unit %d", -other);
      LRAT_ID id = internal->unit_id (-other);
      units.push_back (id);
    } else if (other == rewrite1.src && rewrite1.id1) {
      push_id_on_chain (rewriting, rewrite1, other);
    } else if (other == -rewrite1.src && rewrite1.id2) {
      push_id_on_chain (rewriting, rewrite1, other);
    } else if (other == rewrite2.src && rewrite2.id1) {
      push_id_on_chain (rewriting, rewrite2, other);
    } else if (other == -rewrite2.src && rewrite2.id2) {
      push_id_on_chain (rewriting, rewrite2, other);
    } else if (other != find_eager_representative_and_compress (other)) {
#if defined(LOGGING) || !defined(CADICAL_NDEBUG)
      const int rewritten_other = eager_representative (other);
      CADICAL_assert (other != rewritten_other);
      LOG ("reason for representative of %d %d is %" PRIu64 "", other,
           rewritten_other, find_eager_representative_lrat (other));
#endif
      rewriting.push_back (find_eager_representative_lrat (other));
    } else {
      LOG ("no rewriting needed for %d", other);
    }
  }
  for (auto id : units)
    chain.push_back (id);

  if (!insert_id_after)
    chain.push_back (c->id);
  for (auto id : rewriting)
    chain.push_back (id);

  if (insert_id_after)
    chain.push_back (c->id);
}

// Note: it is important that the Rewrite takes over the normal rewriting,
// because we can force rewriting that way that have not been done eagerly
// yet.
void Closure::push_id_and_rewriting_lrat_full (Clause *c, Rewrite rewrite1,
                                               std::vector<LRAT_ID> &chain,
                                               bool insert_id_after,
                                               Rewrite rewrite2,
                                               int except_lhs,
                                               int except_lhs2) {
  LOG (c,
       "computing normalized LRAT chain for clause, rewriting except for "
       "%d (%" PRIu64 ", %" PRIu64 ") and %d (%" PRIu64 ", %" PRIu64
       ") and skipping %d and %d",
       rewrite1.src, rewrite1.id1, rewrite1.id2, rewrite2.src, rewrite2.id1,
       rewrite2.id2, except_lhs, except_lhs2);
  CADICAL_assert (c);
  if (!insert_id_after)
    chain.push_back (c->id);
  for (auto other : *c) {
    // unclear how to achieve this in the simplify context where other ==
    // g->lhs might be set CADICAL_assert (internal->val (other) <= 0 || other ==
    // except);
    if (other == except_lhs) {
      // do nothing;
    } else if (other == except_lhs2) {
      // do nothing;
    } else if (internal->val (other) < 0) {
      LOG ("found unit %d", -other);
      LRAT_ID id = internal->unit_id (-other);
      CADICAL_assert (id);
      chain.push_back (id);
    } else if (other == rewrite1.src && rewrite1.id1) {
      push_id_on_chain (chain, rewrite1, other);
    } else if (other == -rewrite1.src && rewrite1.id2) {
      push_id_on_chain (chain, rewrite1, other);
    } else if (other == rewrite2.src && rewrite2.id1) {
      push_id_on_chain (chain, rewrite2, other);
    } else if (other == -rewrite2.src && rewrite2.id2) {
      push_id_on_chain (chain, rewrite2, other);
    } else {
      CADICAL_assert (other == find_eager_representative (other));
      LOG ("no rewriting needed for %d", other);
    }
  }
  if (insert_id_after)
    chain.push_back (c->id);
}

// Note: it is important that the Rewrite takes over the normal rewriting,
// because we can force rewriting that way that have not been done eagerly
// yet.
void Closure::push_id_on_chain (std::vector<LRAT_ID> &chain, Clause *c) {
  CADICAL_assert (c);
  chain.push_back (c->id);
  LOG (lrat_chain, "chain");
}

void Closure::push_id_on_chain (std::vector<LRAT_ID> &chain,
                                const std::vector<LitClausePair> &reasons) {
  for (auto litId : reasons) {
    LOG (litId.clause, "found lrat in gate %d from %zd", litId.current_lit,
         litId.clause->id);
    push_id_on_chain (chain, litId.clause);
  }
  LOG (lrat_chain, "chain from %zd reasons", reasons.size ());
}

void Closure::learn_congruence_unit_when_lhs_set (Gate *g, int src,
                                                  LRAT_ID id1, LRAT_ID id2,
                                                  int dst) {
  if (!internal->lrat)
    return;
  LOG ("calculating LRAT chain learn_congruence_unit_when_lhs_set");
  CADICAL_assert (!g->pos_lhs_ids.empty ());
  CADICAL_assert (internal->analyzed.empty ());
  CADICAL_assert (internal->val (g->lhs) < 0);
  switch (g->tag) {
  case Gate_Type::And_Gate:
    LOG (lrat_chain, "lrat");
    LOG (lrat_chain, "lrat");
    for (auto litId : g->neg_lhs_ids)
      push_id_and_rewriting_lrat_unit (
          litId.clause, Rewrite (src, dst, id1, id2), lrat_chain);
    LOG (lrat_chain, "lrat");
    break;
  default:
    CADICAL_assert (false);
  }
}

// Something very important here: as we are producing a unit, we cannot
// simplify or rewrite the clauses as this will produce units.
void Closure::learn_congruence_unit_falsifies_lrat_chain (
    Gate *g, int src, int dst, int clashing, int falsified, int unit) {
  if (!internal->lrat)
    return;
  CADICAL_assert (!g->pos_lhs_ids.empty ());
  CADICAL_assert (internal->analyzed.empty ());
  CADICAL_assert (lrat_chain.empty ());
  std::vector<LRAT_ID> proof_chain;
  switch (g->tag) {
  case Gate_Type::And_Gate:
    if (clashing) {
      LOG ("clashing %d where -lhs=%d", clashing, -g->lhs);
      // Example: -2 = 1&3 and 3=2
      // The proof consists in taking the binary clause of the clashing
      // literal
      if (clashing == -g->lhs) {
        for (auto litId : g->pos_lhs_ids) {
          LOG (litId.clause,
               "found lrat in gate %d from %zd (looking for %d)",
               litId.current_lit, litId.clause->id, falsified);
          if (litId.current_lit == clashing) {
            push_id_and_rewriting_lrat_unit (
                litId.clause, Rewrite (), proof_chain, true, Rewrite (),
                g->degenerated_and_neg || g->degenerated_and_pos ? 0 : -g->lhs);
          }
        }
      } else {
        // Example: 3 = (-1&2) and 2=1
        // The proof consists in taking the binary clause with the rewrites
        // Example where the rewrite must be before:
        // 2: 3v2
        // 9: -2v1
        // 6: 3v1
        // The chain cannot start by 9
        if (g->degenerated_and_neg || g->degenerated_and_pos) {
          LOG ("%d %d %d", src, dst, g->lhs);
          if (src == g->lhs || dst == g->lhs) {
            LOG ("degenerated AND gate with dst=lhs");
            for (const auto &litId : g->pos_lhs_ids) {
              LOG (litId.clause, "definition clause %d ->",
                   litId.current_lit);
              if (litId.current_lit == clashing) {
                push_id_and_rewriting_lrat_unit (litId.clause, Rewrite (),
                                                 proof_chain, true,
                                                 Rewrite (), 0);
                LOG (proof_chain, "produced lrat chain so far");
              }
            }
	    CADICAL_assert (!proof_chain.empty ());
          } else {
            LOG ("degenerated AND gate with conflict without LHS");
            for (const auto &litId : g->pos_lhs_ids) {
              LOG (litId.clause, "definition clause %d ->",
                   litId.current_lit);
              push_id_and_rewriting_lrat_unit (litId.clause, Rewrite (),
                                               proof_chain, false,
                                               Rewrite (), -g->lhs);
              LOG (proof_chain, "produced lrat chain so far");
            }
          }
        } else {
          LOG ("normal AND gate");
          for (const auto &litId : g->pos_lhs_ids) {
            push_id_and_rewriting_lrat_unit (litId.clause, Rewrite (),
                                             proof_chain, false, Rewrite (),
                                             g->degenerated_and_neg || g->degenerated_and_pos ? 0 : -g->lhs);
            LOG (proof_chain, "produced lrat chain so far");
          }
        }
      }
      LOG (proof_chain, "produced lrat chain");
    } else if (falsified) {
      LOG ("falsifies %d", falsified);
      // Example is 3=(1&2) with 2=false or 3=(1&4) with 4=2 and 2=false
      // (can happen when the unit was derived in the middle of the
      // rewriting)
      for (auto litId : g->pos_lhs_ids) {
        LOG (litId.clause,
             "found lrat in gate %d from %zd (looking for %d)",
             litId.current_lit, litId.clause->id, falsified);
        if (litId.current_lit == falsified ||
            (litId.current_lit == src && dst == falsified)) {
          push_id_and_rewriting_lrat_unit (litId.clause, Rewrite (),
                                           proof_chain, true, Rewrite (),
                                           -dst, -g->lhs);
        }
      }
    } else {
      CADICAL_assert (unit);
      // Example is 1 = 2&3 where 2 and 3 are false
      for (auto litId : g->neg_lhs_ids) {
        push_id_and_rewriting_lrat_unit (litId.clause, Rewrite (),
                                         proof_chain);
      }
      LOG (proof_chain, "produced lrat chain");
      break;
    }
    lrat_chain = std::move (proof_chain);
    break;
  default:
    CADICAL_assert (false);
  }
  (void) unit;
}

bool Closure::fully_propagate () {
  if (internal->unsat)
    return false;
  LOG ("fully propagating");
  CADICAL_assert (internal->watching ());
  CADICAL_assert (full_watching);
  bool no_conflict = internal->propagate ();

  if (no_conflict)
    return true;
  internal->learn_empty_clause ();
  if (internal->lrat)
    lrat_chain.clear ();

  return false;
}
bool Closure::learn_congruence_unit (int lit, bool delay_propagation, bool force_propagation) {
  if (internal->unsat)
    return false;
  const signed char val_lit = internal->val (lit);
  if (val_lit > 0) {
    LOG ("already set lit %d", lit);
    if (internal->lrat)
      lrat_chain.clear ();
    if (force_propagation)
      return fully_propagate();
    return true;
  }
  LOG ("adding unit %s", LOGLIT (lit));
  ++internal->stats.congruence.units;
  CADICAL_assert (!internal->lrat || !lrat_chain.empty ());
  if (val_lit < 0) {
    if (internal->lrat) {
      CADICAL_assert (internal->lrat_chain.empty ());
      LRAT_ID id = internal->unit_id (-lit);
      internal->lrat_chain.push_back (id);
      for (auto id : lrat_chain)
        internal->lrat_chain.push_back (id);
      lrat_chain.clear ();
    }
    internal->learn_empty_clause ();
    return false;
  }

  LOG (lrat_chain, "assigning due to LRAT chain");
  swap (lrat_chain, internal->lrat_chain);
  internal->assign_unit (lit);
  CADICAL_assert (lrat_chain.empty ());
  CADICAL_assert (internal->lrat_chain.empty ());
  if (delay_propagation)
    return false;
  else return fully_propagate ();
}

// for merging the literals there are many cases
// TODO: LRAT does not work if the LHS is not in normal form and if the
// representative is also in the gate.
bool Closure::merge_literals_lrat (
    Gate *g, Gate *h, int lit, int other,
    const std::vector<LRAT_ID> &extra_reasons_lit,
    const std::vector<LRAT_ID> &extra_reasons_ulit) {
  CADICAL_assert (!internal->unsat);
  CADICAL_assert (g->lhs == lit);
  CADICAL_assert (g == h || h->lhs == other);
  (void) g, (void) h;
  LOG ("merging literals %s and %s", LOGLIT(lit), LOGLIT(other));
  // TODO: this should not update_eager but still calculate the LRAT chain
  // below!
  const int repr_lit = find_representative_and_compress (lit, false);
  const int repr_other = find_representative_and_compress (other, false);
  find_representative_and_compress (-lit, false);
  find_representative_and_compress (-other, false);
  LOG ("merging literals %d [=%d] and %d [=%d]", lit, repr_lit, other,
       repr_other);

  if (repr_lit == repr_other) {
    LOG ("already merged %d and %d", lit, other);
    if (internal->lrat)
      lrat_chain.clear ();
    return false;
  }

  const int val_lit = internal->val (lit);
  const int val_other = internal->val (other);
  if (val_lit) {
    if (val_lit == val_other) {
      LOG ("not merging lits %d and %d assigned to same value", lit, other);
      if (internal->lrat)
        lrat_chain.clear ();
      return false;
    }
  }

  // For LRAT we need to distinguish more cases for a more regular
  // reconstruction.
  //
  // 1. if lit = -other, then we learn lit and -lit to derive false
  //
  // 2. otherwise, we learn the new clauses lit = -other (which are two real
  // clauses).
  //
  //    2a. if repr_lit = -repr_other, we learn the units repr_lit and
  //    -repr_lit to derive false
  //
  //    2b. otherwise, we learn the equivalences repr_lit = -repr_other
  //    (which are two real clauses)
  //
  // Without LRAT this is easier, as we directly learn the conclusion
  // (either false or the equivalence). The steps can also not be merged
  // because repr_lit can appear in the gate and hence in the resolution
  // chain.
  int smaller_repr = repr_lit;
  int larger_repr = repr_other;
  int smaller = lit;
  int larger = other;
  const std::vector<LRAT_ID> *smaller_chain = &extra_reasons_ulit;
  const std::vector<LRAT_ID> *larger_chain = &extra_reasons_lit;

  if (abs (smaller_repr) > abs (larger_repr)) {
    swap (smaller_repr, larger_repr);
    swap (smaller, larger);
    swap (smaller_chain, larger_chain);
  }

  CADICAL_assert (find_representative (smaller_repr) == smaller_repr);
  CADICAL_assert (find_representative (larger_repr) == larger_repr);
  if (lit == -other) {
    CADICAL_assert (chain.empty ());
    LOG ("merging clashing %d and %d", lit, other);
    if (internal->proof) {
      if (internal->lrat) {
        for (auto id : *smaller_chain)
          lrat_chain.push_back (id);
      }
      unsimplified.push_back (smaller);
      LRAT_ID id = simplify_and_add_to_proof_chain (unsimplified);

      if (internal->lrat) {
        internal->lrat_chain.push_back (id);
        for (auto id : *larger_chain)
          internal->lrat_chain.push_back (id);
        LOG (internal->lrat_chain, "lrat chain");
      }
    }
    internal->learn_empty_clause ();
    delete_proof_chain ();
    return false;
  }

  LOG ("merging %d and %d first and then the equivalences of %d and %d "
       "with LRAT",
       lit, other, repr_lit, repr_other);
  Clause *eq1_tmp = nullptr;
  if (internal->lrat) {
    lrat_chain = *smaller_chain;
    eq1_tmp = add_tmp_binary_clause (-larger, smaller);
  }
  CADICAL_assert (!internal->lrat || eq1_tmp);

  Clause *eq2_tmp = nullptr;
  if (internal->lrat) {
    lrat_chain = *larger_chain;
    LOG (lrat_chain, "lrat chain");

    eq2_tmp = add_tmp_binary_clause (larger, -smaller);
    // the order in the clause is important for the
    // repr_lit == -repr_other to get the right chain
  }
  CADICAL_assert (!internal->lrat || eq2_tmp);
  if (internal->lrat)
    lrat_chain.clear ();

  if (repr_lit == -repr_other) {
    // now derive empty clause
    Rewrite rew1, rew2;
    if (internal->lrat) {
      // no need to calculate push_id_and_rewriting_lrat here because all
      // the job is done by the arguments already
      rew1 = Rewrite (lit == repr_lit ? 0 : lit, repr_lit,
                      lit == repr_lit ? 0 : representative_id (lit),
                      lit == repr_lit ? 0 : representative_id (-lit));
      rew2 = Rewrite (other == repr_other ? 0 : other, repr_other,
                      other == repr_other ? 0 : representative_id (other),
                      other == repr_other ? 0 : representative_id (-other));
      push_id_and_rewriting_lrat_unit (eq1_tmp, rew1, lrat_chain, true,
                                       rew2);
      swap (lrat_chain, internal->lrat_chain);
    }
    internal->assign_unit (-larger_repr);
    if (internal->lrat) {
      internal->lrat_chain.clear ();

      if (larger != larger_repr)
        push_lrat_unit (-larger_repr);
      push_id_and_rewriting_lrat_unit (
          eq2_tmp, rew1, lrat_chain, true, rew2,
          larger != larger_repr ? larger_repr : 0);
      LOG (lrat_chain, "lrat chain");
      swap (lrat_chain, internal->lrat_chain);
    }
    internal->learn_empty_clause ();
    if (internal->lrat)
      internal->lrat_chain.clear ();
    return false;
  }

  if (val_lit) {
    if (val_lit == val_other) {
      LOG ("not merging lits %d and %d assigned to same value", lit, other);
      if (internal->lrat)
        lrat_chain.clear ();
      return false;
    }
    if (val_lit == -val_other) {
      LOG ("merging lits %d and %d assigned to inconsistent value", lit,
           other);
      CADICAL_assert (lrat_chain.empty ());
      if (internal->lrat) {
        Clause *c = val_lit ? eq2_tmp : eq1_tmp;
        int pos = val_lit ? other : lit;
        int neg = val_lit ? -lit : -other;
        push_lrat_unit (pos);
        push_lrat_unit (neg);
        push_id_on_chain (lrat_chain, c);
      }
      internal->learn_empty_clause ();
      if (internal->lrat)
        lrat_chain.clear ();
      return false;
    }

    CADICAL_assert (!val_other);
    LOG ("merging assigned %d and unassigned %d", lit, other);
    CADICAL_assert (lrat_chain.empty ());
    const int unit = (val_lit < 0) ? -other : other;
    if (internal->lrat) {
      Clause *c;
      if (lit == smaller) {
        if (val_lit < 0)
          c = eq1_tmp;
        else
          c = eq2_tmp;
      } else {
        if (val_lit < 0)
          c = eq2_tmp;
        else
          c = eq1_tmp;
      }
      push_id_and_rewriting_lrat_unit (c, Rewrite (), lrat_chain, true,
                                       Rewrite (), unit);
    }
    learn_congruence_unit (unit);
    if (internal->lrat)
      lrat_chain.clear ();
    return false;
  }

  if (!val_lit && val_other) {
    LOG ("merging assigned %d and unassigned %d", lit, other);
    CADICAL_assert (lrat_chain.empty ());
    const int unit = (val_other < 0) ? -lit : lit;
    if (internal->lrat) {
      Clause *c;
      if (lit == smaller) {
        if (val_lit < 0)
          c = eq1_tmp;
        else
          c = eq2_tmp;
      } else {
        if (val_lit < 0)
          c = eq2_tmp;
        else
          c = eq1_tmp;
      }
      push_id_and_rewriting_lrat_unit (c, Rewrite (), lrat_chain, true,
                                       Rewrite (), lit, unit);
    }
    learn_congruence_unit (unit);
    if (internal->lrat)
      lrat_chain.clear ();
    return false;
  }

  Clause *eq1_repr, *eq2_repr;
  if (smaller_repr != smaller || larger != larger_repr) {
    if (internal->lrat) {
      lrat_chain.clear ();
      Rewrite rew1 = Rewrite (
          smaller_repr != smaller ? smaller : 0,
          smaller_repr != smaller ? smaller_repr : 0,
          smaller_repr != smaller ? representative_id (smaller) : 0,
          smaller_repr != smaller ? representative_id (-smaller) : 0);
      Rewrite rew2 =
          Rewrite (larger_repr != larger ? larger : 0,
                   larger_repr != larger ? larger_repr : 0,
                   larger_repr != larger ? representative_id (larger) : 0,
                   larger_repr != larger ? representative_id (-larger) : 0);
      push_id_and_rewriting_lrat_full (eq1_tmp, rew1, lrat_chain, true,
                                       rew2);
    }
    eq1_repr = learn_binary_tmp_or_full_clause (-larger_repr, smaller_repr);
  } else {
    if (internal->lrat)
      eq1_repr = maybe_promote_tmp_binary_clause (eq1_tmp);
    else
      eq1_repr = maybe_add_binary_clause (-larger_repr, smaller_repr);
  }

  if (internal->lrat) {
    lrat_chain.clear ();
  }

  if (smaller_repr != smaller || larger != larger_repr) {

    if (internal->lrat) {
      lrat_chain.clear ();
      // eq2 = larger, -smaller
      Rewrite rew1 = Rewrite (
          smaller_repr != smaller ? smaller : 0,
          smaller_repr != smaller ? smaller_repr : 0,
          smaller_repr != smaller ? representative_id (smaller) : 0,
          smaller_repr != smaller ? representative_id (-smaller) : 0);
      Rewrite rew2 =
          Rewrite (larger_repr != larger ? larger : 0,
                   larger_repr != larger ? larger_repr : 0,
                   larger_repr != larger ? representative_id (larger) : 0,
                   larger_repr != larger ? representative_id (-larger) : 0);
      push_id_and_rewriting_lrat_full (eq2_tmp, rew1, lrat_chain, true,
                                       rew2);
    }

    eq2_repr = learn_binary_tmp_or_full_clause (larger_repr, -smaller_repr);

  } else {
    if (internal->lrat)
      eq2_repr = maybe_promote_tmp_binary_clause (eq2_tmp);
    else
      eq2_repr = maybe_add_binary_clause (larger_repr, -smaller_repr);
  }
  lrat_chain.clear ();

  if (internal->lrat) {
    representative_id (larger_repr) = eq1_repr->id;
    CADICAL_assert (std::find (begin (*eq1_repr), end (*eq1_repr), -larger_repr) !=
            end (*eq1_repr));
    representative_id (-larger_repr) = eq2_repr->id;
    CADICAL_assert (std::find (begin (*eq2_repr), end (*eq2_repr), larger_repr) !=
            end (*eq2_repr));
  }
  LOG ("updating %d -> %d", larger_repr, smaller_repr);
  representative (larger_repr) = smaller_repr;
  representative (-larger_repr) = -smaller_repr;
  schedule_literal (larger_repr);
  ++internal->stats.congruence.congruent;
  CADICAL_assert (lrat_chain.empty ());
  return true;
}

// Variant when there are no gates
// TODO: this is exactly the same as the other function without the gates.
// Kill the arguments!
bool Closure::merge_literals_lrat (
    int lit, int other, const std::vector<LRAT_ID> &extra_reasons_lit,
    const std::vector<LRAT_ID> &extra_reasons_ulit) {
  CADICAL_assert (!internal->unsat);
  LOG ("merging literals %s and %s", LOGLIT (lit), LOGLIT (other));
  // TODO: this should not update_eager but still calculate the LRAT chain
  // below!
  const int repr_lit = find_representative_and_compress (lit, false);
  const int repr_other = find_representative_and_compress (other, false);
  find_representative_and_compress (-lit, false);
  find_representative_and_compress (-other, false);
  LOG ("merging literals %s [=%d] and %s [=%d]", LOGLIT (lit), repr_lit,
       LOGLIT (other), repr_other);
  LOG (lrat_chain, "lrat chain beginning of merge");

  if (repr_lit == repr_other) {
    LOG ("already merged %s and %s", LOGLIT (lit), LOGLIT (other));
    if (internal->lrat)
      lrat_chain.clear ();
    return false;
  }

  const int val_lit = internal->val (lit);
  const int val_other = internal->val (other);

  // For LRAT we need to distinguish more cases for a more regular
  // reconstruction.
  //
  // 1. if lit = -other, then we learn lit and -lit to derive false
  //
  // 2. otherwise, we learn the new clauses lit = -other (which are two real
  // clauses).
  //
  //    2a. if repr_lit = -repr_other, we learn the units repr_lit and
  //    -repr_lit to derive false
  //
  //    2b. otherwise, we learn the equivalences repr_lit = -repr_other
  //    (which are two real clauses)
  //
  // Without LRAT this is easier, as we directly learn the conclusion
  // (either false or the equivalence). The steps can also not be merged
  // because repr_lit can appear in the gate and hence in the resolution
  // chain.
  int smaller_repr = repr_lit;
  int larger_repr = repr_other;
  int val_smaller = val_lit;
  int val_larger = val_other;
  int smaller = lit;
  int larger = other;
  const std::vector<LRAT_ID> *smaller_chain = &extra_reasons_ulit;
  const std::vector<LRAT_ID> *larger_chain = &extra_reasons_lit;

  if (abs (smaller_repr) > abs (larger_repr)) {
    swap (smaller_repr, larger_repr);
    swap (smaller, larger);
    swap (smaller_chain, larger_chain);
    swap (val_smaller, val_larger);
  }

  CADICAL_assert (find_representative (smaller_repr) == smaller_repr);
  CADICAL_assert (find_representative (larger_repr) == larger_repr);
  if (lit == -other) {
    LOG ("merging clashing %d and %d", lit, other);
    if (!val_smaller) {
      if (internal->lrat)
        internal->lrat_chain = *smaller_chain;
      internal->assign_unit (smaller);
      if (internal->lrat)
        internal->lrat_chain.clear ();

      push_lrat_unit (smaller);
      if (internal->lrat) {
        CADICAL_assert (internal->lrat_chain.empty ());
        swap (internal->lrat_chain, lrat_chain);
        for (auto id : *larger_chain)
          internal->lrat_chain.push_back (id);
        LOG (internal->lrat_chain, "lrat chain");
      }
      internal->learn_empty_clause ();
      return false;
    } else {
      if (internal->lrat)
        internal->lrat_chain =
            (val_smaller < 0 ? *smaller_chain : *larger_chain);
      if (internal->lrat)
        internal->lrat_chain.push_back (
            internal->unit_id (val_smaller > 0 ? smaller : -smaller));
      internal->learn_empty_clause ();
      return false;
    }
  }

  if (val_lit && val_lit == val_other) {
    LOG ("not merging lits %d and %d assigned to same value", lit, other);
    return false;
  }

  if (val_lit && val_lit == -val_other) {
    if (internal->lrat) {
      internal->lrat_chain.push_back (
          internal->unit_id (val_smaller < 0 ? -smaller : smaller));
      internal->lrat_chain.push_back (
				      internal->unit_id (val_larger < 0 ? -larger : larger));
      for (auto id : (val_smaller < 0 ? *smaller_chain : *larger_chain)) {
	internal->lrat_chain.push_back(id);
      }
    }
    internal->learn_empty_clause ();
    return false;
  }

  LOG ("merging %d and %d first and then the equivalences of %d and %d "
       "with LRAT",
       lit, other, repr_lit, repr_other);
  Clause *eq1_tmp = nullptr;
  if (internal->lrat) {
    lrat_chain = *smaller_chain;
    eq1_tmp = add_tmp_binary_clause (-larger, smaller);
    CADICAL_assert (eq1_tmp);
  }
  CADICAL_assert (!internal->lrat || eq1_tmp);

  Clause *eq2_tmp = nullptr;
  if (internal->lrat) {
    lrat_chain = *larger_chain;
    LOG (lrat_chain, "lrat chain");
    // the order in the clause is important for the
    // repr_lit == -repr_other to get the right chain
    eq2_tmp = add_tmp_binary_clause (larger, -smaller);
    CADICAL_assert (eq2_tmp);
  }

  CADICAL_assert (!internal->lrat || eq2_tmp);
  if (internal->lrat)
    lrat_chain.clear ();

  if (repr_lit == -repr_other) {
    // now derive empty clause
    Rewrite rew1, rew2;
    if (internal->lrat) {
      // no need to calculate push_id_and_rewriting_lrat here because all
      // the job is done by the arguments already
      rew1 = Rewrite (lit == repr_lit ? 0 : lit, repr_lit,
                      lit == repr_lit ? 0 : representative_id (lit),
                      lit == repr_lit ? 0 : representative_id (-lit));
      rew2 = Rewrite (other == repr_other ? 0 : other, repr_other,
                      other == repr_other ? 0 : representative_id (other),
                      other == repr_other ? 0 : representative_id (-other));
      push_id_and_rewriting_lrat_unit (eq1_tmp, rew1, lrat_chain, true,
                                       rew2);
      swap (lrat_chain, internal->lrat_chain);
    }
    CADICAL_assert (val_larger == internal->val (larger_repr));
    if (!val_larger) {
      // not assigned, first assign one
      internal->assign_unit (-larger_repr);
      if (internal->lrat) {
        internal->lrat_chain.clear ();

        if (larger != larger_repr)
          push_lrat_unit (-larger_repr);
        // no need to calculate push_id_and_rewriting_lrat here because all
        // the job is done by the arguments already
        push_id_and_rewriting_lrat_unit (
            eq2_tmp, rew1, lrat_chain, true, rew2,
            larger != larger_repr ? larger_repr : 0);
        LOG (lrat_chain, "lrat chain");
        swap (lrat_chain, internal->lrat_chain);
      }
    } else {
      // otherwise, no need to
      if (internal->lrat)
        lrat_chain.push_back (internal->unit_id (larger_repr));
    }
    internal->learn_empty_clause ();
    if (internal->lrat)
      internal->lrat_chain.clear ();
    return false;
  }

  if (val_lit) {
    if (val_lit == val_other) {
      LOG ("not merging lits %d and %d assigned to same value", lit, other);
      if (internal->lrat)
        lrat_chain.clear ();
      return false;
    }
    if (val_lit == -val_other) {
      LOG ("merging lits %d and %d assigned to inconsistent value", lit,
           other);
      CADICAL_assert (lrat_chain.empty ());
      if (internal->lrat) {
        Clause *c = val_lit ? eq2_tmp : eq1_tmp;
        int pos = val_lit ? other : lit;
        int neg = val_lit ? -lit : -other;
        push_lrat_unit (pos);
        push_lrat_unit (neg);
        push_id_on_chain (lrat_chain, c);
      }
      internal->learn_empty_clause ();
      if (internal->lrat)
        lrat_chain.clear ();
      return false;
    }

    CADICAL_assert (!val_other);
    LOG ("merging assigned %d and unassigned %d", lit, other);
    CADICAL_assert (lrat_chain.empty ());
    const int unit = (val_lit < 0) ? -other : other;
    if (internal->lrat) {
      Clause *c;
      if (lit == smaller) {
        if (val_lit < 0)
          c = eq1_tmp;
        else
          c = eq2_tmp;
      } else {
        if (val_lit < 0)
          c = eq2_tmp;
        else
          c = eq1_tmp;
      }
      push_id_and_rewriting_lrat_unit (c, Rewrite (), lrat_chain, true,
                                       Rewrite (), unit);
    }
    learn_congruence_unit (unit);
    if (internal->lrat)
      lrat_chain.clear ();
    return false;
  }

  if (!val_lit && val_other) {
    LOG ("merging assigned %d and unassigned %d", lit, other);
    CADICAL_assert (lrat_chain.empty ());
    const int unit = (val_other < 0) ? -lit : lit;
    if (internal->lrat) {
      Clause *c;
      if (lit == smaller) {
        CADICAL_assert (other == larger);
        if (val_other > 0)
          c = eq1_tmp;
        else
          c = eq2_tmp;
      } else {
        if (val_other > 0)
          c = eq2_tmp;
        else
          c = eq1_tmp;
      }
      push_id_and_rewriting_lrat_unit (c, Rewrite (), lrat_chain, true,
                                       Rewrite (), lit, unit);
    }
    learn_congruence_unit (unit);
    if (internal->lrat)
      lrat_chain.clear ();
    return false;
  }

  Clause *eq1_repr, *eq2_repr;
  if (smaller_repr != smaller || larger != larger_repr) {
    if (internal->lrat) {
      lrat_chain.clear ();
      Rewrite rew1 = Rewrite (
          smaller_repr != smaller ? smaller : 0,
          smaller_repr != smaller ? smaller_repr : 0,
          smaller_repr != smaller ? representative_id (smaller) : 0,
          smaller_repr != smaller ? representative_id (-smaller) : 0);
      Rewrite rew2 =
          Rewrite (larger_repr != larger ? larger : 0,
                   larger_repr != larger ? larger_repr : 0,
                   larger_repr != larger ? representative_id (larger) : 0,
                   larger_repr != larger ? representative_id (-larger) : 0);
      push_id_and_rewriting_lrat_full (eq1_tmp, rew1, lrat_chain, true,
                                       rew2);
    }
    eq1_repr = learn_binary_tmp_or_full_clause (-larger_repr, smaller_repr);
  } else {
    if (internal->lrat)
      eq1_repr = maybe_promote_tmp_binary_clause (eq1_tmp);
    else
      eq1_repr = maybe_add_binary_clause (-larger_repr, smaller_repr);
  }

  if (internal->lrat) {
    lrat_chain.clear ();
  }

  if (smaller_repr != smaller || larger != larger_repr) {

    if (internal->lrat) {
      lrat_chain.clear ();
      // eq2 = larger, -smaller
      Rewrite rew1 = Rewrite (
          smaller_repr != smaller ? smaller : 0,
          smaller_repr != smaller ? smaller_repr : 0,
          smaller_repr != smaller ? representative_id (smaller) : 0,
          smaller_repr != smaller ? representative_id (-smaller) : 0);
      Rewrite rew2 =
          Rewrite (larger_repr != larger ? larger : 0,
                   larger_repr != larger ? larger_repr : 0,
                   larger_repr != larger ? representative_id (larger) : 0,
                   larger_repr != larger ? representative_id (-larger) : 0);
      push_id_and_rewriting_lrat_full (eq2_tmp, rew1, lrat_chain, true,
                                       rew2);
    }
    eq2_repr = learn_binary_tmp_or_full_clause (larger_repr, -smaller_repr);
  } else {
    if (internal->lrat)
      eq2_repr = maybe_promote_tmp_binary_clause (eq2_tmp);
    else
      eq2_repr = maybe_add_binary_clause (larger_repr, -smaller_repr);
  }
  lrat_chain.clear ();

  if (internal->lrat) {
    representative_id (larger_repr) = eq1_repr->id;
    CADICAL_assert (std::find (begin (*eq1_repr), end (*eq1_repr), -larger_repr) !=
            end (*eq1_repr));
    representative_id (-larger_repr) = eq2_repr->id;
    check_not_tmp_binary_clause (eq1_repr);
    check_not_tmp_binary_clause (eq2_repr);
    CADICAL_assert (std::find (begin (*eq2_repr), end (*eq2_repr), larger_repr) !=
            end (*eq2_repr));
  }
  LOG ("updating %d -> %d", larger_repr, smaller_repr);
  representative (larger_repr) = smaller_repr;
  representative (-larger_repr) = -smaller_repr;
  schedule_literal (larger_repr);
  ++internal->stats.congruence.congruent;
  CADICAL_assert (lrat_chain.empty ());
  return true;
}

inline void Closure::promote_clause (Clause *c) {
  if (internal->lrat)
    check_not_tmp_binary_clause (c);
  if (!c)
    return;
  if (!c->redundant)
    return;
  LOG (c, "turning redundant subsuming clause into irredundant clause");
  c->redundant = false;
  if (internal->proof)
    internal->proof->strengthen (c->id);
  internal->stats.current.irredundant++;
  internal->stats.added.irredundant++;
  internal->stats.irrlits += c->size;
  CADICAL_assert (internal->stats.current.redundant > 0);
  internal->stats.current.redundant--;
  CADICAL_assert (internal->stats.added.redundant > 0);
  internal->stats.added.redundant--;
  // ... and keep 'stats.added.total'.
}

// This function is rather tricky for LRAT. If you have 2 = 1 and 3=4 you
// cannot add 2=3. You really to connect the representatives directly
// therefore you actually need to learn the clauses 2->3->4 and -2->1 and
// vice-versa
bool Closure::merge_literals_equivalence (int lit, int other, Clause *c1,
                                          Clause *c2) {
  CADICAL_assert (!internal->unsat);
  LRAT_ID id1 = c1 ? c1->id : 0;
  LRAT_ID id2 = c2 ? c2->id : 0;
  if (internal->lrat) {
    CADICAL_assert (c1);
    CADICAL_assert (c2);
    CADICAL_assert (c1->size == 2);
    CADICAL_assert (c2->size == 2);
    CADICAL_assert (c1->literals[0] == lit || c1->literals[1] == lit);
    CADICAL_assert (c2->literals[0] == other || c2->literals[1] == other);
    CADICAL_assert (c1->literals[0] == -other || c1->literals[1] == -other);
    CADICAL_assert (c2->literals[0] == -lit || c2->literals[1] == -lit);
    check_not_tmp_binary_clause (c1);
    check_not_tmp_binary_clause (c2);
  }
  int repr_lit = find_representative (lit);
  int repr_other = find_representative (other);
  find_representative_and_compress_both (lit);
  find_representative_and_compress_both (other);
  LOG ("merging literals %d [=%d] and %d [=%d] lrat", lit, repr_lit, other,
       repr_other);

  if (repr_lit == repr_other) {
    LOG ("already merged %d and %d", lit, other);
    return false;
  }
  const int val_lit = internal->val (lit);
  const int val_other = internal->val (other);

  if (val_lit) {
    if (val_lit == val_other) {
      LOG ("not merging lits %d and %d assigned to same value", lit, other);
      return false;
    }
    if (val_lit == -val_other) {
      if (internal->lrat)
        lrat_chain.push_back (internal->unit_id (lit)),
            lrat_chain.push_back (internal->unit_id (other));
      LOG ("merging lits %d and %d assigned to inconsistent value", lit,
           other);
      internal->learn_empty_clause ();
      return false;
    }

    CADICAL_assert (!val_other);
    LOG ("merging assigned %d and unassigned %d", lit, other);
    const int unit = (val_lit < 0) ? -other : other;
    if (internal->lrat)
      lrat_chain.push_back (internal->unit_id (unit));
    if (val_lit < 0)
      lrat_chain.push_back (c2->id);
    else
      lrat_chain.push_back (c1->id);
    learn_congruence_unit (unit);
    return false;
  }

  if (!val_lit && val_other) {
    LOG ("merging assigned %d and unassigned %d", other, lit);
    const int unit = (val_other < 0) ? -lit : lit;
    if (internal->lrat)
      lrat_chain.push_back (
          internal->unit_id (val_other < 0 ? -other : other));
    if (val_lit < 0)
      lrat_chain.push_back (c1->id);
    else
      lrat_chain.push_back (c2->id);
    learn_congruence_unit (unit);
    return false;
  }

  int smaller_repr = repr_lit;
  int larger_repr = repr_other;
  int smaller = lit;
  int larger = other;

  if (abs (smaller_repr) > abs (larger_repr)) {
    swap (smaller_repr, larger_repr);
    swap (smaller, larger);
  }

  CADICAL_assert (find_representative (smaller_repr) == smaller_repr);
  CADICAL_assert (find_representative (larger_repr) == larger_repr);

  if (repr_lit == -repr_other) {
    LOG ("merging clashing %d [=%d] and %d[=%d], smaller: %d", lit,
         repr_lit, other, repr_other, smaller);
    if (internal->lrat) {
      Rewrite rew1 =
          Rewrite (lit, lit == repr_lit ? 0 : repr_lit,
                   lit == repr_lit ? 0 : find_representative_lrat (lit),
                   lit == repr_lit ? 0 : find_representative_lrat (-lit));
      Rewrite rew2 = Rewrite (
          other, other == repr_other ? 0 : repr_other,
          other == repr_other ? 0 : find_representative_lrat (other),
          other == repr_other ? 0 : find_representative_lrat (-other));
      push_id_and_rewriting_lrat_unit (c1, rew1, lrat_chain, true, rew2);
      LOG (lrat_chain, "lrat chain");
      swap (internal->lrat_chain, lrat_chain);
    }
    internal->assign_unit (repr_lit);
    if (internal->lrat) {
      lrat_chain.clear ();
      LRAT_ID id = internal->unit_id (repr_lit);
      CADICAL_assert (id);
      lrat_chain.push_back (id);
      if (lit != repr_lit) {
        const LRAT_ID repr_id2 = find_representative_lrat (-lit);
        lrat_chain.push_back (repr_id2);
      }
      lrat_chain.push_back (id2);
      if (other != repr_other) {
        const LRAT_ID repr_larger_id2 = find_representative_lrat (other);
        lrat_chain.push_back (repr_larger_id2);
      }
      LOG (lrat_chain, "lrat chain");
      swap (internal->lrat_chain, lrat_chain);
    }
    internal->learn_empty_clause ();
    return false;
  }

  LOG ("merging %d [=%d] and %d [=%d]", lit, repr_lit, other, repr_other);
  promote_clause (c1), promote_clause (c2);
  bool learn_clause = (lit != repr_lit) || (other != repr_other);
  if (learn_clause) {
    if (internal->lrat) {
      if (lit != repr_lit) {
        LOG ("adding chain for lit %d -> %d", lit, repr_lit);
        lrat_chain.push_back (find_representative_lrat (lit));
      }
      if (other != repr_other) {
        LOG ("adding chain for lit %d -> %d", -other, -repr_other);
        lrat_chain.push_back (find_representative_lrat (-other));
      }
      lrat_chain.push_back (id1);
    }
    Clause *eq1 = learn_binary_tmp_or_full_clause (repr_lit, -repr_other);

    if (internal->lrat) {
      lrat_chain.clear ();
      if (lit != repr_lit)
        lrat_chain.push_back (find_representative_lrat (-lit));
      if (other != repr_other)
        lrat_chain.push_back (find_representative_lrat (other));
      lrat_chain.push_back (id2);
    }
    Clause *eq2 = learn_binary_tmp_or_full_clause (-repr_lit, repr_other);
    if (internal->lrat) {
      lrat_chain.clear ();
      if (smaller_repr == repr_lit) {
        CADICAL_assert (larger_repr == repr_other);
        representative_id (-larger_repr) = eq2->id;
        CADICAL_assert (std::find (eq2->begin (), eq2->end (), larger_repr) !=
                eq2->end ());
        representative_id (larger_repr) = eq1->id;
        CADICAL_assert (std::find (eq1->begin (), eq1->end (), -larger_repr) !=
                eq1->end ());
      } else {
        CADICAL_assert (larger_repr == repr_lit);
        representative_id (-larger_repr) = eq1->id;
        CADICAL_assert (std::find (eq1->begin (), eq1->end (), larger_repr) !=
                eq1->end ());
        representative_id (larger_repr) = eq2->id;
        CADICAL_assert (std::find (eq2->begin (), eq2->end (), -larger_repr) !=
                eq2->end ());
      }
    }

  } else if (internal->lrat) {
    LOG ("not learning new clause, using already existing one");
    if (smaller_repr == repr_lit) {
      LOG ("setting ids of %d: %" PRIu64 "; %d: %" PRIu64 " (case 1)",
           larger, id1, -larger, id2);
      representative_id (-larger_repr) = id2;
      representative_id (larger_repr) = id1;
    } else {
      LOG ("setting ids of %d: %" PRIu64 "; %d: %" PRIu64 " (case 2)",
           larger, id2, -larger, id1);
      representative_id (-larger_repr) = id1;
      representative_id (larger_repr) = id2;
    }
  }

  LOG ("updating %d -> %d", larger_repr, smaller_repr);
  representative (larger_repr) = smaller_repr;
  representative (-larger_repr) = -smaller_repr;
  schedule_literal (larger_repr);
  ++internal->stats.congruence.congruent;
  return true;
}

/*------------------------------------------------------------------------*/
GOccs &Closure::goccs (int lit) { return gtab[internal->vlit (lit)]; }

void Closure::connect_goccs (Gate *g, int lit) {
  LOG (g, "connect %d to", lit);
  // incorrect for ITE
  // CADICAL_assert (std::find(begin (goccs (lit)), end (goccs (lit)), g) ==
  // std::end (goccs (lit)));
  goccs (lit).push_back (g);
}

uint64_t &Closure::largecount (int lit) {
  CADICAL_assert (internal->vlit (lit) < glargecounts.size ());
  return glargecounts[internal->vlit (lit)];
}

/*------------------------------------------------------------------------*/
// Initialization

void Closure::init_closure () {
  representant.resize (2 * internal->max_var + 3);
  eager_representant.resize (2 * internal->max_var + 3);
  if (internal->lrat) {
    eager_representant_id.resize (2 * internal->max_var + 3);
    representant_id.resize (2 * internal->max_var + 3);
    lazy_propagated_idx.resize (internal->max_var + 1, false);
    for (auto lit : internal->trail)
      lazy_propagated (lit) = true;
  }
  marks.resize (2 * internal->max_var + 3);
  mu1_ids.resize (2 * internal->max_var + 3);
  if (internal->lrat) {
    mu2_ids.resize (2 * internal->max_var + 3);
    mu4_ids.resize (2 * internal->max_var + 3);
  }
#ifndef CADICAL_NDEBUG
  for (auto &it : mu1_ids)
    it.current_lit = 0, it.clause = nullptr;
  for (auto &it : mu2_ids)
    it.current_lit = 0, it.clause = nullptr;
  for (auto &it : mu4_ids)
    it.current_lit = 0, it.clause = nullptr;
#endif
  scheduled.resize (internal->max_var + 1);
  gtab.resize (2 * internal->max_var + 3);
  for (auto v : internal->vars) {
    representative (v) = v;
    representative (-v) = -v;
    eager_representative (v) = v;
    eager_representative (-v) = -v;
  }
  units = internal->propagated;
  Random rand (internal->stats.congruence.rounds);
  for (auto &n : nonces) {
    n = 1 | rand.next ();
  }
#ifdef LOGGING
  fresh_id = internal->clause_id;
#endif
  internal->init_noccs ();
  internal->init_occs ();
}

void Closure::init_and_gate_extraction () {
  LOG ("[gate-extraction]");
  for (Clause *c : internal->clauses) {
    if (c->garbage)
      continue;
    if (c->redundant && c->size != 2)
      continue;
    if (c->size > 2)
      continue;
    CADICAL_assert (c->size == 2);
    const int lit = c->literals[0];
    const int other = c->literals[1];
    internal->noccs (lit)++;
    internal->noccs (other)++;
    internal->watch_clause (c);
  }
}

/*------------------------------------------------------------------------*/
void Closure::check_and_gate_implied (Gate *g) {
  CADICAL_assert (internal->clause.empty ());
  CADICAL_assert (g->tag == Gate_Type::And_Gate);
  if (internal->lrat)
    return;
  LOG (g, "checking implied");
  const int lhs = g->lhs;
  const int not_lhs = -lhs;
  for (auto other : g->rhs)
    check_binary_implied (not_lhs, other);
  internal->clause = g->rhs;
  check_implied ();
  internal->clause.clear ();
}

void Closure::delete_proof_chain () {
  if (!internal->proof) {
    CADICAL_assert (chain.empty ());
    return;
  }
  if (chain.empty ())
    return;
  LOG ("starting deletion of proof chain");
  auto &clause = internal->clause;
  CADICAL_assert (clause.empty ());
  uint32_t id1 = UINT32_MAX, id2 = UINT32_MAX;
  LRAT_ID id = 0;

  LOG (chain, "chain:");
  for (auto lit : chain) {
    LOG ("reading %d from chain", lit);
    if (id1 == UINT32_MAX) {
      id1 = lit;
      id = (LRAT_ID) id1;
      continue;
    }
    if (id2 == UINT32_MAX) {
      id2 = lit;
      id = ((LRAT_ID) id1 << 32) + id2;
      continue;
    }
    if (lit) { // parsing the id first
      LOG ("found %d as literal in chain", lit);
      clause.push_back (lit);
    } else {
      CADICAL_assert (id);
      internal->proof->delete_clause (id, false, clause);
      clause.clear ();
      id = 0, id1 = UINT32_MAX, id2 = UINT32_MAX;
    }
  }
  CADICAL_assert (clause.empty ());
  chain.clear ();
  LOG ("finished deletion of proof chain");
}

/*------------------------------------------------------------------------*/
// Simplification
bool Closure::skip_and_gate (Gate *g) {
  CADICAL_assert (g->tag == Gate_Type::And_Gate);
  if (g->garbage)
    return true;
  const int lhs = g->lhs;
  if (internal->val (lhs) > 0) {
    mark_garbage (g);
    return true;
  }

  CADICAL_assert (g->arity () > 1);
  return false;
}

bool Closure::skip_xor_gate (Gate *g) {
  CADICAL_assert (g->tag == Gate_Type::XOr_Gate);
  if (g->garbage)
    return true;
  CADICAL_assert (g->arity () > 1);
  return false;
}

void Closure::shrink_and_gate (Gate *g, int falsifies, int clashing) {
  if (falsifies) {
    g->rhs.resize (1);
    g->rhs[0] = falsifies;
    g->hash = hash_lits (nonces, g->rhs);
  } else if (clashing) {
    LOG (g, "gate before clashing on %d", clashing);
    g->rhs.resize (2);
    g->rhs[0] = clashing;
    g->rhs[1] = -clashing;
    g->hash = hash_lits (nonces, g->rhs);
    LOG (g, "gate after clashing on %d", clashing);
  }
  g->shrunken = true;
}

void Closure::update_and_gate_unit_build_lrat_chain (
    Gate *g, int src, LRAT_ID id1, LRAT_ID id2, int dst,
    std::vector<LRAT_ID> &extra_reasons_lit,
    std::vector<LRAT_ID> &extra_reasons_ulit) {
  LOG ("generate chain for gate boiling down to unit");
  if (g->neg_lhs_ids.size () != 1) {

    if (g->degenerated_and_neg || g->degenerated_and_pos) {
      // can happen for 4 = AND 3 4 (degenerated with only the clause -4 3)
      // with a rewriting 4 -> 1 (unchanged clause)
      // and later 1 -> 3 (unchanged clause)
      // but you do not know anymore from the gate that it is degenerated
      CADICAL_assert (g->pos_lhs_ids.size () == 1);
      push_id_and_rewriting_lrat_unit (g->pos_lhs_ids[0].clause, Rewrite (),
                                       extra_reasons_ulit, true, Rewrite ());
      return;
    }
    CADICAL_assert (g->lhs == g->rhs[0] || (g->lhs == src && g->rhs[0] == dst));
    CADICAL_assert (g->pos_lhs_ids.size () <= 1); // either degenerated or empty A = A
    return;
  }
  CADICAL_assert (g->neg_lhs_ids.size () == 1);
  CADICAL_assert (!g->pos_lhs_ids.empty ());

  const int repr_lit = find_representative (g->lhs);
  const int repr_other = find_representative (g->rhs[0]);
  if (repr_lit == repr_other) {
    LOG ("skipping already merged");
    return;
  }

  push_id_and_rewriting_lrat_unit (g->neg_lhs_ids[0].clause, Rewrite (),
                                   extra_reasons_ulit, true, Rewrite (),
                                   g->lhs, -dst);
  LOG (extra_reasons_ulit, "lrat chain for negative side");

  lrat_chain.clear ();

  CADICAL_assert (!g->pos_lhs_ids.empty ());
  for (auto litId : g->pos_lhs_ids)
    push_id_and_rewriting_lrat_unit (
        litId.clause, Rewrite (src, dst, id1, id2), extra_reasons_lit, true,
        Rewrite (), -g->lhs, dst);
  LOG (extra_reasons_lit, "lrat chain for positive side");
}

void Closure::update_and_gate_build_lrat_chain (
    Gate *g, Gate *h, std::vector<LRAT_ID> &extra_reasons_lit,
    std::vector<LRAT_ID> &extra_reasons_ulit, bool remove_units) {
  CADICAL_assert (g != h);
  LOG (g, "merging");
  LOG (h, "with");
  // If the LHS are identical, do not even attempt to build the LRAT chain
  if (find_representative (g->lhs) == find_representative (h->lhs))
    return;
  // set to same value, don't do anything
  if (internal->val (g->lhs) && internal->val (g->lhs) == internal->val (h->lhs))
    return;
  const bool g_tautology = gate_contains (g, g->lhs);
  const bool h_tautology = gate_contains (h, h->lhs);
  if (g_tautology && h_tautology) {
    LOG ("both gates are a tautology");
    // special case: actually we have an equivalence due to binary clauses
    // and all gate clauses (except one binary) are actually tautologies
    for (auto &litId : g->pos_lhs_ids) {
      if (litId.current_lit == h->lhs) {
        CADICAL_assert (extra_reasons_lit.empty ());
        LOG (litId.clause, "binary clause to push into the reason");
        litId.clause = produce_rewritten_clause_lrat (litId.clause, g->lhs,
                                                      remove_units);
        CADICAL_assert (litId.clause);
        extra_reasons_lit.push_back (litId.clause->id);
      }
    }
    CADICAL_assert (!extra_reasons_lit.empty ());
    CADICAL_assert (extra_reasons_lit.size () == 1);

    for (auto &litId : h->pos_lhs_ids) {
      if (litId.current_lit == g->lhs) {
        CADICAL_assert (extra_reasons_ulit.empty ());
	CADICAL_assert (litId.clause);
        LOG (litId.clause, "binary clause to push into the reason");
        litId.clause = produce_rewritten_clause_lrat (litId.clause, h->lhs,
                                                      remove_units);
        CADICAL_assert (litId.clause);
        extra_reasons_ulit.push_back (litId.clause->id);
      }
    }
    CADICAL_assert (!extra_reasons_ulit.empty ());
    CADICAL_assert (extra_reasons_ulit.size () == 1);
    return;
  }
  if (g_tautology || h_tautology) {
    // special case: actually we have an equivalence due to binary clauses
    // and some of the clauses from the gate are actually tautologies
    CADICAL_assert (g_tautology != h_tautology);
    Gate *tauto = (g_tautology ? g : h);
    Gate *other = (g_tautology ? h : g);
    LOG (tauto, "one gate is a tautology");
    CADICAL_assert (tauto != other);
    CADICAL_assert (tauto == h || tauto == g);

    auto &extra_reasons_tauto =
        (!g_tautology ? extra_reasons_lit : extra_reasons_ulit);
    auto &extra_reasons_other =
        (!g_tautology ? extra_reasons_ulit : extra_reasons_lit);

    // one direction: the binary clause already exists
    for (auto &litId : other->pos_lhs_ids) {
      if (litId.current_lit == tauto->lhs) {
	CADICAL_assert (litId.clause);
        CADICAL_assert (extra_reasons_tauto.empty ());
        LOG (litId.clause, "binary clause to push into the reason");
        litId.clause = produce_rewritten_clause_lrat (
            litId.clause, other->lhs, remove_units);
        CADICAL_assert (litId.clause);
        extra_reasons_tauto.push_back (litId.clause->id);
      }
    }
    CADICAL_assert (!extra_reasons_tauto.empty ());

    // other direction, we have to resolve
    LOG (tauto, "now the other direction");
    for (auto &litId : tauto->pos_lhs_ids) {
      CADICAL_assert (litId.clause);
      LOG (litId.clause,
           "binary clause from %d to push into the reason [avoiding %d]",
           litId.current_lit, tauto->lhs);
      if (litId.current_lit != tauto->lhs) {
        LOG (litId.clause, "binary clause to push into the reason");
	CADICAL_assert (litId.clause);
        litId.clause = produce_rewritten_clause_lrat (
            litId.clause, tauto->lhs, remove_units);
        if (!litId.clause) // degenerated but does not know yet
	  continue;
	CADICAL_assert (litId.clause);
        extra_reasons_other.push_back (litId.clause->id);
      }
      tauto->pos_lhs_ids.erase (std::remove_if (begin (tauto->pos_lhs_ids),
                                            end (tauto->pos_lhs_ids),
                                            [] (const LitClausePair &p) {
                                              return !p.clause;
                                            }),
                            end (tauto->pos_lhs_ids));
    }
    CADICAL_assert (!extra_reasons_other.empty ());
    produce_rewritten_clause_lrat_and_clean (other->neg_lhs_ids, other->lhs,
                                             remove_units);
    push_id_on_chain (extra_reasons_other, other->neg_lhs_ids);
    CADICAL_assert (!extra_reasons_tauto.empty ());
    CADICAL_assert (!extra_reasons_other.empty ());
    return;
  }
  // default: resolve all clauses
  // first rewrite
  // TODO: do we really need dest as second exclusion?
  produce_rewritten_clause_lrat_and_clean (h->pos_lhs_ids, -h->lhs,
                                           remove_units);
  CADICAL_assert (internal->clause.empty ());
  produce_rewritten_clause_lrat_and_clean (h->neg_lhs_ids, -h->lhs,
                                           remove_units);
  CADICAL_assert (internal->clause.empty ());
  produce_rewritten_clause_lrat_and_clean (g->pos_lhs_ids, -g->lhs,
                                           remove_units);
  CADICAL_assert (internal->clause.empty ());
  produce_rewritten_clause_lrat_and_clean (g->neg_lhs_ids, -g->lhs,
                                           remove_units);
  CADICAL_assert (internal->clause.empty ());

  push_id_on_chain (extra_reasons_ulit, h->pos_lhs_ids);
  push_id_on_chain (extra_reasons_ulit, g->neg_lhs_ids[0].clause);
  lrat_chain.clear ();
  LOG (extra_reasons_ulit, "lrat chain for negative side");

  push_id_on_chain (extra_reasons_lit, g->pos_lhs_ids);
  push_id_on_chain (extra_reasons_lit, h->neg_lhs_ids);

  lrat_chain.clear ();
  LOG (extra_reasons_lit, "lrat chain for positive side");
  CADICAL_assert (!extra_reasons_lit.empty ());
  CADICAL_assert (!extra_reasons_ulit.empty ());
}

void Closure::update_and_gate (Gate *g, GatesTable::iterator it, int src,
                               int dst, LRAT_ID id1, LRAT_ID id2,
                               int falsifies, int clashing) {
  LOG (g, "update and gate of arity %ld", g->arity ());
  CADICAL_assert (lrat_chain.empty ());
  bool garbage = true;
  if (falsifies || clashing) {
    if (internal->lrat)
      learn_congruence_unit_falsifies_lrat_chain (g, src, dst, clashing,
                                                  falsifies, 0);
    int unit = -g->lhs;
    if (unit == src)
      unit = dst;
    else if (unit == -src)
      unit = -dst;
    learn_congruence_unit (unit);
    if (internal->lrat)
      lrat_chain.clear ();
  } else if (g->arity () == 1) {
    const signed char v = internal->val (g->lhs);
    if (v > 0) {
      if (internal->lrat)
        learn_congruence_unit_falsifies_lrat_chain (g, src, dst, 0, 0,
                                                    g->lhs);
      learn_congruence_unit (g->rhs[0]);
      if (internal->lrat)
        lrat_chain.clear ();
    } else if (v < 0) {
      if (internal->lrat)
        learn_congruence_unit_when_lhs_set (g, src, id1, id2, dst);
      learn_congruence_unit (-g->rhs[0]);
      if (internal->lrat)
        lrat_chain.clear ();
    } else {
      std::vector<LRAT_ID> extra_reasons_lit;
      std::vector<LRAT_ID> extra_reasons_ulit;
      if (internal->lrat)
        update_and_gate_unit_build_lrat_chain (g, src, id1, id2, g->rhs[0],
                                               extra_reasons_lit,
                                               extra_reasons_ulit);
      if (merge_literals_lrat (g, g, g->lhs, g->rhs[0], extra_reasons_lit,
                               extra_reasons_ulit)) {
        ++internal->stats.congruence.unaries;
        ++internal->stats.congruence.unary_and;
      }
    }
  } else {
    CADICAL_assert (g->arity () > 1);
    sort_literals_by_var (g->rhs);
    Gate *h = find_and_lits (g->rhs, g);
    CADICAL_assert (g != h);
    if (h) {
      CADICAL_assert (garbage);
      std::vector<LRAT_ID> extra_reasons_lit2;
      std::vector<LRAT_ID> extra_reasons_ulit2;
      if (internal->lrat)
        update_and_gate_build_lrat_chain (g, h, extra_reasons_lit2,
                                          extra_reasons_ulit2);
      if (merge_literals_lrat (g, h, g->lhs, h->lhs, extra_reasons_lit2,
                               extra_reasons_ulit2))
        ++internal->stats.congruence.ands;
    } else {
      if (g->indexed) {
        LOG (g, "removing from table");
        (void) table.erase (it);
      }
      g->hash = hash_lits (nonces, g->rhs);
      LOG (g, "inserting gate into table");
      CADICAL_assert (table.count (g) == 0);
      table.insert (g);
      g->indexed = true;
      garbage = false;
      if (internal->lrat)
        lrat_chain.clear ();
    }
  }

  if (garbage && !internal->unsat)
    mark_garbage (g);
}

void Closure::update_xor_gate (Gate *g, GatesTable::iterator git) {
  CADICAL_assert (g->tag == Gate_Type::XOr_Gate);
  CADICAL_assert (!internal->unsat && chain.empty ());
  LOG (g, "updating");
  bool garbage = true;
  // TODO Florian LRAT for learn_congruence_unit
  CADICAL_assert (g->arity () == 0 || internal->clause.empty ());
  CADICAL_assert (clause.empty ());
  if (g->arity () == 0) {
    if (internal->clause.size ()) {
      CADICAL_assert (!internal->proof || (internal->clause.size () == 1 &&
                                   internal->clause.back () == -g->lhs));
      CADICAL_assert (!internal->lrat || lrat_chain.size ());
      internal->clause.clear ();

    } else if (internal->lrat) {
      simplify_unit_xor_lrat_clauses (g->pos_lhs_ids, g->lhs);
      CADICAL_assert (clause.size () && clause.back () == -g->lhs);
      clause.clear ();
    }

    if (internal->lrat && internal->val (-g->lhs) < 0) {
      internal->lrat_chain.push_back (internal->unit_id (g->lhs));
      for (auto id : lrat_chain)
        internal->lrat_chain.push_back (id);
      lrat_chain.clear ();
      internal->learn_empty_clause();
    } else
      learn_congruence_unit (-g->lhs);

    CADICAL_assert (clause.empty ());
  } else if (g->arity () == 1) {
    std::vector<LRAT_ID> reasons_implication, reasons_back;
    if (internal->lrat) {
      vector<LitClausePair> first;
      simplify_and_sort_xor_lrat_clauses (g->pos_lhs_ids, first, g->lhs);
      g->pos_lhs_ids = first;
      CADICAL_assert (g->pos_lhs_ids.size () == 2);
      reasons_implication.push_back (g->pos_lhs_ids[0].clause->id);
      reasons_back.push_back (g->pos_lhs_ids[1].clause->id);
    }
    const signed char v = internal->val (g->lhs);
    if (v > 0) {
      if (internal->lrat) {
        push_lrat_unit (g->lhs);
        lrat_chain.push_back (g->pos_lhs_ids[0].clause->id);
      }
      learn_congruence_unit (g->rhs[0]);
    } else if (v < 0) {
      if (internal->lrat) {
        push_lrat_unit (-g->lhs);
        lrat_chain.push_back (g->pos_lhs_ids[1].clause->id);
      }
      learn_congruence_unit (-g->rhs[0]);
    } else if (merge_literals_lrat (
                   g->lhs, g->rhs[0], reasons_implication,
                   reasons_back)) { // TODO Florian merge with LRAT
      ++internal->stats.congruence.unaries;
      ++internal->stats.congruence.unary_and;
    }
    CADICAL_assert (clause.empty ());
  } else {
    Gate *h = find_xor_gate (g);
    if (h) {
      CADICAL_assert (garbage);
      std::vector<LRAT_ID> reasons_implication, reasons_back;
      add_xor_matching_proof_chain (g, g->lhs, h->pos_lhs_ids, h->lhs,
                                    reasons_implication, reasons_back);
      if (merge_literals_lrat (g->lhs, h->lhs, reasons_implication,
                               reasons_back))
        ++internal->stats.congruence.xors;
      delete_proof_chain ();
    } else {
      if (g->indexed) {
        remove_gate (git);
      }
      g->hash = hash_lits (nonces, g->rhs);
      LOG (g, "reinserting in table");
      table.insert (g);
      g->indexed = true;
      CADICAL_assert (table.find (g) != end (table));
      garbage = false;
    }
    CADICAL_assert (clause.empty ());
  }
  if (garbage && !internal->unsat)
    mark_garbage (g);
  CADICAL_assert (clause.empty ());
}

void Closure::simplify_and_gate (Gate *g) {
  if (skip_and_gate (g))
    return;
  GatesTable::iterator git = (g->indexed ? table.find (g) : end (table));
  CADICAL_assert (!g->indexed || git != end (table));
  LOG (g, "simplifying");
  int falsifies = 0;
  std::vector<int>::iterator it = begin (g->rhs);
  bool ulhs_in_rhs = false;
  for (auto lit : g->rhs) {
    const signed char v = internal->val (lit);
    if (v > 0) {
      continue;
    }
    if (v < 0) {
      falsifies = lit;
      continue;
    }
    if (lit == -g->lhs)
      ulhs_in_rhs = true;
    *it++ = lit;
    if (lit == g->lhs)
      g->degenerated_and_pos = true;
    if (lit == -g->lhs)
      g->degenerated_and_neg = true;
  }

  if (internal->lrat) { // updating reasons
    size_t i = 0, size = g->pos_lhs_ids.size ();
    for (size_t j = 0; j < size; ++j) {
      LOG ("looking at %d [%ld %ld]", g->pos_lhs_ids[j].current_lit, i, j);
      g->pos_lhs_ids[i] = g->pos_lhs_ids[j];
      if (!g->degenerated_and_pos &&
          internal->val (g->pos_lhs_ids[i].current_lit) &&
          g->pos_lhs_ids[i].current_lit != falsifies)
        continue;
      LOG ("keeping %d [%ld %ld]", g->pos_lhs_ids[i].current_lit, i, j);
      ++i;
    }
    LOG ("resizing to %ld", i);
    g->pos_lhs_ids.resize (i);
  }

  CADICAL_assert (it <= end (g->rhs)); // can be equal when ITE are converted to
                               // ands leading to
  CADICAL_assert (it >= begin (g->rhs));
  LOG (g, "shrunken");

  g->shrunken = true;
  g->rhs.resize (it - std::begin (g->rhs));
  g->hash = hash_lits (nonces, g->rhs);

  LOG (g, "shrunken");
  shrink_and_gate (g, falsifies);
  std::vector<LRAT_ID> reasons_lrat_src, reasons_lrat_usrc;

  update_and_gate (g, git, 0, 0, 0, 0, falsifies, 0);
  ++internal->stats.congruence.simplified_ands;
  ++internal->stats.congruence.simplified;

  if (ulhs_in_rhs) { // missing in Kissat, TODO: port back
    CADICAL_assert (gate_contains (g, -g->lhs));
    if (internal->lrat) {
      for (auto litId : g->pos_lhs_ids) {
        if (litId.current_lit == g->lhs) {
          compute_rewritten_clause_lrat_simple (litId.clause, 0);
          break;
        }
      }
    }
    learn_congruence_unit (-g->lhs);
  }
}

bool Closure::simplify_gate (Gate *g) {
  switch (g->tag) {
  case Gate_Type::And_Gate:
    simplify_and_gate (g);
    break;
  case Gate_Type::XOr_Gate:
    simplify_xor_gate (g);
    break;
  case Gate_Type::ITE_Gate:
    simplify_ite_gate (g);
    break;
  default:
    CADICAL_assert (false);
    break;
  }
  CADICAL_assert (lrat_chain.empty ());
  return !internal->unsat;
}

bool Closure::simplify_gates (int lit) {
  const auto &occs = goccs (lit);
  for (Gate *g : occs) {
    CADICAL_assert (lrat_chain.empty ());
    CADICAL_assert (clause.empty ());
    if (!simplify_gate (g))
      return false;
  }
  return true;
}
/*------------------------------------------------------------------------*/
// AND gates

Gate *Closure::find_and_lits (const vector<int> &rhs, Gate *except) {
  CADICAL_assert (is_sorted (begin (rhs), end (rhs),
                     sort_literals_by_var_smaller (internal)));
  return find_gate_lits (rhs, Gate_Type::And_Gate, except);
}

// search for the gate in the hash-table.  We cannot use find, as we might
// be changing a gate, so there might be 2 gates with the same LHS (the one
// we are changing and the other we are looking for)
Gate *Closure::find_gate_lits (const vector<int> &rhs, Gate_Type typ,
                               Gate *except) {
  Gate *g = new Gate;
  g->tag = typ;
  g->rhs = rhs;
  g->hash = hash_lits (nonces, g->rhs);
  g->lhs = 0;
  g->garbage = false;
#ifdef LOGGING
  g->id = 0;
#endif
  const auto &its = table.equal_range (g);
  Gate *h = nullptr;
  for (auto it = its.first; it != its.second; ++it) {
    LOG ((*it), "checking gate in the table");
    if (*it == except)
      continue;
    CADICAL_assert ((*it)->lhs != g->lhs);
    if ((*it)->tag != g->tag)
      continue;
    if ((*it)->rhs != g->rhs)
      continue;
    h = *it;
    break;
  }

  if (h) {
    LOG (g, "searching");
    LOG (h, "already existing");
    delete g;
    return h;
  }

  else {
    LOG (g->rhs, "gate not found in table");
    delete g;
    return nullptr;
  }
}

Gate *Closure::new_and_gate (Clause *base_clause, int lhs) {
  rhs.clear ();
  auto &lits = this->lits;

  for (auto lit : lits) {
    if (lhs != lit) {
      CADICAL_assert (lhs != -lit);
      rhs.push_back (-lit);
    }
  }

  CADICAL_assert (rhs.size () + 1 == lits.size ());
  sort_literals_by_var (this->rhs);

  Gate *h = find_and_lits (this->rhs);
  Gate *g = new Gate;
  g->lhs = lhs;
  g->tag = Gate_Type::And_Gate;
  if (internal->lrat) {
    g->neg_lhs_ids.push_back (LitClausePair (lhs, base_clause));
    for (auto i : lrat_chain_and_gate)
      g->pos_lhs_ids.push_back (i);
#ifdef LOGGING
    std::vector<LRAT_ID> result;
    transform (begin (g->pos_lhs_ids), end (g->pos_lhs_ids),
               back_inserter (result),
               [] (const LitClausePair &x) { return x.clause->id; });
    LOG (result, "lrat chain positive (%d):", lhs);
    result.clear ();
    transform (begin (g->neg_lhs_ids), end (g->neg_lhs_ids),
               back_inserter (result),
               [] (const LitClausePair &x) { return x.clause->id; });
    LOG (result, "lrat chain negative (%d):", lhs);
#endif
  }

  if (internal->lrat)
    lrat_chain_and_gate.clear ();

  if (h) {
    std::vector<LRAT_ID> reasons_lrat_src, reasons_lrat_usrc;
    if (internal->lrat)
      merge_and_gate_lrat_produce_lrat (g, h, reasons_lrat_src,
                                        reasons_lrat_usrc);
    if (merge_literals_lrat (g, h, lhs, h->lhs, reasons_lrat_src,
                             reasons_lrat_usrc)) {
      LOG ("found merged literals");
      ++internal->stats.congruence.ands;
    }
    return nullptr;
  } else {
    g->rhs = {rhs};
    CADICAL_assert (!internal->lrat ||
            g->pos_lhs_ids.size () ==
                g->arity ()); // otherwise we need intermediate clauses
    g->garbage = false;
    g->indexed = true;
    g->shrunken = false;
    g->hash = hash_lits (nonces, g->rhs);

    table.insert (g);
    ++internal->stats.congruence.gates;
#ifdef LOGGING
    g->id = fresh_id++;
#endif
    LOG (g, "creating new");
    for (auto lit : g->rhs) {
      connect_goccs (g, lit);
    }
  }
  return g;
}

Gate *Closure::find_first_and_gate (Clause *base_clause, int lhs) {
  CADICAL_assert (internal->analyzed.empty ());
  const int not_lhs = -lhs;
  LOG ("trying to find AND gate with first LHS %d", (lhs));
  LOG ("negated LHS %d occurs in %zd binary clauses", (not_lhs),
       internal->occs (not_lhs).size ());
  unsigned matched = 0;

  const size_t arity = lits.size () - 1;

  for (auto w : internal->watches (not_lhs)) {
    LOG (w.clause, "checking clause for candidates");
    CADICAL_assert (w.binary ());
    CADICAL_assert (w.clause->size == 2);
    CADICAL_assert (w.clause->literals[0] == -lhs || w.clause->literals[1] == -lhs);
    const int other = w.blit;
    signed char &mark = marked (other);
    if (mark) {
      LOG ("marking %d mu2", other);
      ++matched;
      CADICAL_assert (~(mark & 2));
      mark |= 2;
      internal->analyzed.push_back (other);
      set_mu2_reason (other, w.clause);
      if (internal->lrat)
        lrat_chain_and_gate.push_back (LitClausePair (other, w.clause));
    }
  }

  LOG ("found %zd initial LHS candidates", internal->analyzed.size ());
  if (matched < arity) {
    if (internal->lrat)
      lrat_chain_and_gate.clear ();
    return nullptr;
  }

  Gate *g = new_and_gate (base_clause, lhs);

  if (internal->lrat) {
    lrat_chain_and_gate.clear ();
  }
  return g;
}

Clause *Closure::learn_binary_tmp_or_full_clause (int a, int b) {
  Clause *eq1;
  if (internal->lrat) {
    eq1 = add_tmp_binary_clause (a, b);
    eq1 = maybe_promote_tmp_binary_clause (eq1);
  } else
    eq1 = maybe_add_binary_clause (a, b);
  return eq1;
}

Clause *Closure::maybe_add_binary_clause (int a, int b) {
  CADICAL_assert (internal->clause.empty ());
  CADICAL_assert (internal->lrat_chain.empty ());
  CADICAL_assert (!internal->lrat);
  CADICAL_assert (lrat_chain.empty ());
  LOG ("learning binary clause %d %d", a, b);
  if (internal->unsat)
    return nullptr;
  if (a == -b)
    return nullptr;
  if (!internal->lrat) {
    const signed char a_value = internal->val (a);
    if (a_value > 0)
      return nullptr;
    const signed char b_value = internal->val (b);
    if (b_value > 0)
      return nullptr;
    int unit = 0;
    if (a == b)
      unit = a;
    else if (a_value < 0 && !b_value) {
      unit = b;
    } else if (!a_value && b_value < 0)
      unit = a;
    if (unit) {
      LOG ("clause reduced to unit %d", unit);
      learn_congruence_unit (unit);
      return nullptr;
    }
    CADICAL_assert (!a_value), CADICAL_assert (!b_value);
  }
  return add_binary_clause (a, b);
}

Clause *Closure::add_binary_clause (int a, int b) {
  CADICAL_assert (internal->clause.empty ());
  internal->clause.push_back (a);
  internal->clause.push_back (b);
  if (internal->lrat) {
    CADICAL_assert (lrat_chain.size () >= 1);
    CADICAL_assert (internal->lrat_chain.empty ());
    swap (internal->lrat_chain, lrat_chain);
  }
  LOG (internal->lrat_chain, "chain");
  Clause *res = internal->new_hyper_ternary_resolved_clause_and_watch (
      false, full_watching);
  const bool already_sorted = internal->vlit (a) < internal->vlit (b);
  binaries.push_back (CompactBinary (res, res->id, already_sorted ? a : b,
                                     already_sorted ? b : a));
  if (!full_watching)
    new_unwatched_binary_clauses.push_back (res);
  LOG (res, "learning clause");
  internal->clause.clear ();
  if (internal->lrat) {
    internal->lrat_chain.clear ();
  }
  CADICAL_assert (internal->clause.empty ());
  CADICAL_assert (internal->lrat_chain.empty ());
  return res;
}

void Closure::check_not_tmp_binary_clause (Clause *c) {
#ifndef CADICAL_NDEBUG
  CADICAL_assert (internal->lrat);
  CADICAL_assert (internal->lrat_chain.empty ());
  CADICAL_assert (c->size == 2);
  if (internal->val (c->literals[0]) || internal->val (c->literals[1]))
    return;
  CADICAL_assert (std::find (begin (extra_clauses), end (extra_clauses), c) ==
          end (extra_clauses));
#else
  (void) c;
#endif
};

Clause *Closure::maybe_promote_tmp_binary_clause (Clause *c) {
  CADICAL_assert (internal->lrat);
  CADICAL_assert (internal->lrat_chain.empty ());
  CADICAL_assert (c->size == 2);
  LOG (c, "promoting tmp");
#ifndef CADICAL_NDEBUG
  CADICAL_assert (std::find (begin (extra_clauses), end (extra_clauses), c) !=
          end (extra_clauses));
#endif
  if (internal->val (c->literals[0]) || internal->val (c->literals[1]))
    return c;
  lrat_chain.push_back (c->id);
  Clause *res = add_binary_clause (c->literals[0], c->literals[1]);
  LOG (res, "promoted to");
  return res;
};

Clause *Closure::add_tmp_binary_clause (int a, int b) {
  CADICAL_assert (internal->clause.empty ());
  CADICAL_assert (internal->lrat_chain.empty ());
  CADICAL_assert (internal->lrat);
  LOG ("learning tmp binary clause %d %d", a, b);
  if (internal->unsat)
    return nullptr;
  if (a == -b)
    return nullptr;
  CADICAL_assert (internal->clause.empty ());
  internal->clause.push_back (a);
  internal->clause.push_back (b);
  if (internal->lrat) {
    CADICAL_assert (lrat_chain.size () >= 1);
    CADICAL_assert (internal->lrat_chain.empty ());
  }
  LOG (lrat_chain, "chain");
  Clause *res = new_tmp_clause (internal->clause);
  internal->clause.clear ();
  if (internal->lrat) {
    lrat_chain.clear ();
  }
  CADICAL_assert (internal->clause.empty ());
  CADICAL_assert (internal->lrat_chain.empty ());
  LOG (res, "promoted to");
  return res;
}

Gate *Closure::find_remaining_and_gate (Clause *base_clause, int lhs) {
  const int not_lhs = -lhs;

  if (marked (not_lhs) < 2) {
    LOG ("skipping no-candidate LHS %d (%d)", lhs, marked (not_lhs));
    return nullptr;
  }

  LOG ("trying to find AND gate with remaining LHS %d", (lhs));
  LOG ("negated LHS %d occurs times in %zd binary clauses", (not_lhs),
       internal->noccs (-lhs));

  const size_t arity = lits.size () - 1;
  size_t matched = 0;
  CADICAL_assert (1 < arity);

  for (auto w : internal->watches (not_lhs)) {
    CADICAL_assert (w.binary ());
#ifdef LOGGING
    Clause *c = w.clause;
    LOG (c, "checking");
    CADICAL_assert (c->size == 2);
    CADICAL_assert (c->literals[0] == not_lhs || c->literals[1] == not_lhs);
#endif
    const int other = w.blit;
    signed char &mark = marked (other);
    if (!mark)
      continue;
    ++matched;
    if (!(mark & 2)) {
      lrat_chain_and_gate.push_back (LitClausePair (other, w.clause));
      LOG ("pushing %d -> %zd", other, w.clause->id);
      continue;
    }
    LOG ("marking %d mu4", other);
    CADICAL_assert (!(mark & 4));
    mark |= 4;
    lrat_chain_and_gate.push_back (LitClausePair (other, w.clause));
    if (internal->lrat)
      set_mu4_reason (other, w.clause);
  }

  {
    auto q = begin (internal->analyzed);
    CADICAL_assert (!internal->analyzed.empty ());
    CADICAL_assert (marked (not_lhs) == 3);
    for (auto lit : internal->analyzed) {
      signed char &mark = marked (lit);
      if (lit == not_lhs) {
        mark = 1;
        continue;
      }

      CADICAL_assert ((mark & 3) == 3);
      if (mark & 4) {
        mark = 3;
        *q = lit;
        ++q;
        LOG ("keeping LHS candidate %d", -lit);
      } else {
        LOG ("dropping LHS candidate %d", -lit);
        mark = 1;
      }
    }
    CADICAL_assert (q != end (internal->analyzed));
    CADICAL_assert (marked (not_lhs) == 1);
    internal->analyzed.resize (q - begin (internal->analyzed));
    LOG ("after filtering %zu LHS candidate remain",
         internal->analyzed.size ());
  }

  if (matched < arity) {
    if (internal->lrat)
      lrat_chain_and_gate.clear ();
    return nullptr;
  }

  if (!internal->lrat)
    lrat_chain_and_gate.clear ();
  return new_and_gate (base_clause, lhs);
}

struct congruence_occurrences_rank {
  Internal *internal;
  congruence_occurrences_rank (Internal *s) : internal (s) {}
  typedef uint64_t Type;
  Type operator() (int a) {
    uint64_t res = internal->noccs (-a);
    res <<= 32;
    res |= a;
    return res;
  }
};

struct congruence_occurrences_larger {
  Internal *internal;
  congruence_occurrences_larger (Internal *s) : internal (s) {}
  bool operator() (const int &a, const int &b) const {
    return congruence_occurrences_rank (internal) (a) <
           congruence_occurrences_rank (internal) (b);
  }
};

void Closure::extract_and_gates_with_base_clause (Clause *c) {
  CADICAL_assert (!c->garbage);
  CADICAL_assert (lrat_chain.empty ());
  LOG (c, "extracting and gates with clause");
  unsigned size = 0;
  const unsigned arity_limit =
      min (internal->opts.congruenceandarity, MAX_ARITY);
  const unsigned size_limit = arity_limit + 1;
  size_t max_negbincount = 0;
  lits.clear ();

  for (int lit : *c) {
    signed char v = internal->val (lit);
    if (v < 0) {
      // push_lrat_unit (-lit);
      continue;
    }
    if (v > 0) {
      CADICAL_assert (!internal->level);
      LOG (c, "found satisfied clause");
      internal->mark_garbage (c);
      if (internal->lrat)
        lrat_chain.clear ();
      return;
    }
    if (++size > size_limit) {
      LOG (c, "clause is actually too large, thus skipping");
      if (internal->lrat)
        lrat_chain.clear ();
      return;
    }
    const size_t count = internal->noccs (-lit);
    if (!count) {
      LOG (c,
           "%d negated does not occur in any binary clause, thus skipping",
           lit);
      if (internal->lrat)
        lrat_chain.clear ();
      return;
    }

    if (count > max_negbincount)
      max_negbincount = count;
    lits.push_back (lit);
  }

  if (size < 3) {
    LOG (c, "is actually too small, thus skipping");
    if (internal->lrat)
      lrat_chain.clear ();
    CADICAL_assert (lrat_chain.empty ());
    return;
  }

  const size_t arity = size - 1;
  if (max_negbincount < arity) {
    LOG (c,
         "all literals have less than %lu negated occurrences"
         "thus skipping",
         arity);
    if (internal->lrat)
      lrat_chain.clear ();
    return;
  }

  internal->analyzed.clear ();
  size_t reduced = 0;
  const size_t clause_size = lits.size ();
  for (size_t i = 0; i < clause_size; ++i) {
    const int lit = lits[i];
    const unsigned count = internal->noccs (-lit);
    marked (-lit) = 1;
    set_mu1_reason (-lit, c);
    if (count < arity) {
      if (reduced < i) {
        lits[i] = lits[reduced];
        lits[reduced++] = lit;
      } else if (reduced == i)
        ++reduced;
    }
  }
  const size_t reduced_size = clause_size - reduced;
  CADICAL_assert (reduced_size);
  LOG (c, "trying as base arity %lu AND gate", arity);
  CADICAL_assert (begin (lits) + reduced_size <= end (lits));
  MSORT (internal->opts.radixsortlim, begin (lits),
         begin (lits) + reduced_size,
         congruence_occurrences_rank (internal),
         congruence_occurrences_larger (internal));
  bool first = true;
  unsigned extracted = 0;

  for (size_t i = 0; i < clause_size; ++i) {
    CADICAL_assert (lrat_chain.empty ());
    if (internal->unsat)
      break;
    if (c->garbage)
      break;
    const int lhs = lits[i];
    LOG ("trying LHS candidate literal %d with %ld negated occurrences",
         (lhs), internal->noccs (-lhs));

    if (first) {
      first = false;
      CADICAL_assert (internal->analyzed.empty ());
      if (find_first_and_gate (c, lhs) != nullptr) {
        CADICAL_assert (lrat_chain.empty ());
        ++extracted;
      }
    } else if (internal->analyzed.empty ()) {
      LOG ("early abort AND gate search");
      break;
    } else if (find_remaining_and_gate (c, lhs)) {
      CADICAL_assert (lrat_chain.empty ());
      ++extracted;
    }
  }

  unmark_all ();
  LOG (lits, "finish unmarking");
  for (auto lit : lits) {
    marked (-lit) = 0;
  }
  lrat_chain_and_gate.clear ();
  if (extracted)
    LOG (c, "extracted %u with arity %lu AND base", extracted, arity);
}

void Closure::reset_and_gate_extraction () {
  internal->clear_noccs ();
  internal->clear_watches ();
}

void Closure::extract_and_gates () {
  CADICAL_assert (!full_watching);
  if (!internal->opts.congruenceand)
    return;
  START (extractands);
  marks.resize (internal->max_var * 2 + 3);
  init_and_gate_extraction ();

  const size_t size = internal->clauses.size ();
  for (size_t i = 0; i < size && !internal->terminated_asynchronously ();
       ++i) { // we can learn new binary clauses, but no for loop
    CADICAL_assert (lrat_chain.empty ());
    Clause *c = internal->clauses[i];
    if (c->garbage)
      continue;
    if (c->size == 2)
      continue;
    if (c->hyper)
      continue;
    if (c->redundant)
      continue;
    extract_and_gates_with_base_clause (c);
    CADICAL_assert (lrat_chain.empty ());
  }

  reset_and_gate_extraction ();
  STOP (extractands);
}

/*------------------------------------------------------------------------*/
// XOR gates

uint64_t &Closure::new_largecounts (int lit) {
  CADICAL_assert (internal->vlit (lit) < gnew_largecounts.size ());
  return gnew_largecounts[internal->vlit (lit)];
}

uint64_t &Closure::largecounts (int lit) {
  CADICAL_assert (internal->vlit (lit) < glargecounts.size ());
  return glargecounts[internal->vlit (lit)];
}

bool parity_lits (const vector<int> &lits) {
  unsigned res = 0;
  for (auto lit : lits)
    res ^= (lit < 0);
  return res;
}

void inc_lits (vector<int> &lits) {
  bool carry = true;
  for (size_t i = 0; i < lits.size () && carry; ++i) {
    int lit = lits[i];
    carry = (lit < 0);
    lits[i] = -lit;
  }
}

void Closure::check_ternary (int a, int b, int c) {
  CADICAL_assert (internal->clause.empty ());
  if (internal->lrat)
    return;
  auto &clause = internal->clause;
  CADICAL_assert (clause.empty ());
  clause.push_back (a);
  clause.push_back (b);
  clause.push_back (c);
  internal->external->check_learned_clause ();
  if (internal->proof) {
    const LRAT_ID id = internal->clause_id++;
    internal->proof->add_derived_clause (id, false, clause, {});
    internal->proof->delete_clause (id, false, clause);
  }

  clause.clear ();
}

void Closure::check_binary_implied (int a, int b) {
  CADICAL_assert (internal->clause.empty ());
  if (internal->lrat)
    return;
  auto &clause = internal->clause;
  CADICAL_assert (clause.empty ());
  clause.push_back (a);
  clause.push_back (b);
  check_implied ();
  clause.clear ();
}

void Closure::check_implied () {
  if (internal->lrat)
    return;
  internal->external->check_learned_clause ();
}

void Closure::add_xor_shrinking_proof_chain (Gate *g, int pivot) {
  CADICAL_assert (internal->clause.empty ());
  CADICAL_assert (clause.empty ());
  if (!internal->proof)
    return;
  LOG (g, "starting XOR shrinking proof chain");
  vector<LitClausePair> first;
  vector<LitClausePair> newclauses;
  if (internal->lrat) {
    simplify_and_sort_xor_lrat_clauses (g->pos_lhs_ids, first, g->lhs,
                                        pivot);
    gate_sort_lrat_reasons (first, pivot, g->lhs);
  }

  auto &clause = internal->clause;

  const int lhs = g->lhs;
  clause.push_back (-lhs);
  for (auto lit : g->rhs)
    clause.push_back (lit);

  const bool parity = (lhs > 0);
  CADICAL_assert (parity == parity_lits (clause));
  const size_t size = clause.size ();
  const unsigned end = 1u << (size - 1);
  CADICAL_assert (!internal->lrat || first.size () == 2 * end);
#ifdef LOGGING
  for (auto pair : first) {
    LOG (pair.clause, "key %d", pair.current_lit);
  }
#endif
  // TODO Florian adjust indices of first depending on order...
  //
  for (unsigned i = 0; i != end; ++i) {
    while (i && parity != parity_lits (clause))
      inc_lits (clause);
    LOG (clause, "xor shrinking clause");
    if (!internal->lrat) {
      clause.push_back (pivot);
      check_and_add_to_proof_chain (clause);
      clause.pop_back ();
      clause.push_back (-pivot);
      check_and_add_to_proof_chain (clause);
      clause.pop_back ();
    }
    if (internal->lrat) {
      CADICAL_assert (lrat_chain.empty ());
      lrat_chain.push_back (first[2 * i].clause->id);
      lrat_chain.push_back (first[2 * i + 1].clause->id);
    }
    if (clause.size () > 1) {
      if (internal->lrat) {
        Clause *c = new_tmp_clause (clause);
        newclauses.push_back (LitClausePair (0, c));
        lrat_chain.clear ();
      } else {
        check_and_add_to_proof_chain (clause);
      }
    }
    if (clause.size () == 1)
      return;
    inc_lits (clause);
  }
  g->pos_lhs_ids.swap (newclauses);

  clause.clear ();
}

void Closure::check_xor_gate_implied (Gate const *const g) {
  CADICAL_assert (internal->clause.empty ());
  CADICAL_assert (g->tag == Gate_Type::XOr_Gate);
  if (internal->lrat) {
    return;
  }
  const int lhs = g->lhs;
  LOG (g, "checking implied");
  auto &clause = internal->clause;
  CADICAL_assert (clause.empty ());
  for (auto other : g->rhs) {
    CADICAL_assert (other > 0);
    clause.push_back (other);
  }
  clause.push_back (-lhs);
  const unsigned arity = g->arity ();
  const unsigned end = 1u << arity;
  const bool parity = (lhs > 0);

  for (unsigned i = 0; i != end; ++i) {
    while (i && parity_lits (clause) != parity)
      inc_lits (clause);
    internal->external->check_learned_clause ();
    if (internal->proof) {
      internal->proof->add_derived_clause (internal->clause_id, false,
                                           clause, {});
      internal->proof->delete_clause (internal->clause_id, false, clause);
    }
    inc_lits (clause);
  }
  clause.clear ();
}

Gate *Closure::find_xor_lits (const vector<int> &rhs) {
  CADICAL_assert (is_sorted (begin (rhs), end (rhs),
                     sort_literals_by_var_smaller (internal)));
  return find_gate_lits (rhs, Gate_Type::XOr_Gate);
}

Gate *Closure::find_xor_gate (Gate *g) {
  CADICAL_assert (g->tag == Gate_Type::XOr_Gate);
  CADICAL_assert (is_sorted (begin (g->rhs), end (g->rhs),
                     sort_literals_by_var_smaller (internal)));
  return find_gate_lits (g->rhs, Gate_Type::XOr_Gate);
}

void Closure::reset_xor_gate_extraction () { internal->clear_occs (); }

bool Closure::normalize_ite_lits_gate (Gate *g) {
  auto &rhs = g->rhs;
  CADICAL_assert (rhs.size () == 3);
  if (internal->lrat)
    check_correct_ite_flags (g);
  LOG (rhs, "RHS = ");
  if (rhs[0] < 0) {
    rhs[0] = -rhs[0];
    std::swap (rhs[1], rhs[2]);
    if (internal->lrat) {
      CADICAL_assert (g->pos_lhs_ids.size () == 4);
      std::swap (g->pos_lhs_ids[0], g->pos_lhs_ids[2]);
      std::swap (g->pos_lhs_ids[1], g->pos_lhs_ids[3]);
      const int8_t flag = g->degenerated_ite;
      const int8_t plus_then = flag & NO_PLUS_THEN;
      const int8_t neg_then = flag & NO_NEG_THEN;
      const int8_t plus_else = flag & NO_PLUS_ELSE;
      const int8_t neg_else = flag & NO_NEG_ELSE;
      g->degenerated_ite = (plus_then ? Special_ITE_GATE::NO_PLUS_ELSE
                                      : Special_ITE_GATE::NORMAL) |
                           (neg_then ? Special_ITE_GATE::NO_NEG_ELSE
                                     : Special_ITE_GATE::NORMAL) |
                           (plus_else ? Special_ITE_GATE::NO_PLUS_THEN
                                      : Special_ITE_GATE::NORMAL) |
                           (neg_else ? Special_ITE_GATE::NO_NEG_THEN
                                     : Special_ITE_GATE::NORMAL);
      CADICAL_assert (g->pos_lhs_ids[0].current_lit == rhs[1]);
      CADICAL_assert (g->pos_lhs_ids[2].current_lit == rhs[2]);
      if (internal->lrat)
        check_correct_ite_flags (g);
    }
  }
  if (rhs[1] > 0)
    return false;
  if (internal->lrat)
    check_correct_ite_flags (g);
  rhs[1] = -rhs[1];
  rhs[2] = -rhs[2];
  LOG (rhs, "RHS = ");
  if (internal->lrat) {
    CADICAL_assert (g->pos_lhs_ids.size () == 4);
    std::swap (g->pos_lhs_ids[0], g->pos_lhs_ids[1]);
    std::swap (g->pos_lhs_ids[2], g->pos_lhs_ids[3]);
    const int8_t flag = g->degenerated_ite;
    const int8_t plus_then = flag & NO_PLUS_THEN;
    const int8_t neg_then = flag & NO_NEG_THEN;
    const int8_t plus_else = flag & NO_PLUS_ELSE;
    const int8_t neg_else = flag & NO_NEG_ELSE;
    g->degenerated_ite = (plus_then ? Special_ITE_GATE::NO_NEG_THEN
                                    : Special_ITE_GATE::NORMAL) |
                         (neg_then ? Special_ITE_GATE::NO_PLUS_THEN
                                   : Special_ITE_GATE::NORMAL) |
                         (plus_else ? Special_ITE_GATE::NO_NEG_ELSE
                                    : Special_ITE_GATE::NORMAL) |
                         (neg_else ? Special_ITE_GATE::NO_PLUS_ELSE
                                   : Special_ITE_GATE::NORMAL);
    CADICAL_assert (g->pos_lhs_ids[0].current_lit == rhs[1]);
    CADICAL_assert (g->pos_lhs_ids[2].current_lit == rhs[2]);
    // incorrect as we have not negated the LHS yet!
    // check_correct_ite_flags (g);
  }

  LOG (g->rhs, "g/RHS = ");
  return true;
}

#ifndef CADICAL_NDEBUG
bool is_tautological_ite_gate (Gate *g) {
  CADICAL_assert (g->tag == Gate_Type::ITE_Gate);
  CADICAL_assert (g->rhs.size () == 3);
  const int cond_lit = g->rhs[0];
  const int then_lit = g->rhs[1];
  const int else_lit = g->rhs[2];
  return cond_lit == then_lit || cond_lit == else_lit;
}
#endif

Gate *Closure::find_ite_gate (Gate *g, bool &negate_lhs) {
  negate_lhs = normalize_ite_lits_gate (g);
  LOG (g, "post normalize");
  return find_gate_lits (g->rhs, Gate_Type::ITE_Gate, g);
}

LRAT_ID Closure::check_and_add_to_proof_chain (vector<int> &clause) {
  internal->external->check_learned_clause ();
  const LRAT_ID id = ++internal->clause_id;
  if (internal->proof) {
    if (internal->lrat) {
      CADICAL_assert (internal->lrat_chain.empty ());
      CADICAL_assert (lrat_chain.size () >= 1);
    }
    internal->proof->add_derived_clause (id, true, clause, lrat_chain);
    lrat_chain.clear ();
  }
  return id;
}

void Closure::add_clause_to_chain (std::vector<int> unsimplified,
                                   LRAT_ID id) {
  const uint32_t id2_higher = (id >> 32);
  const uint32_t id2_lower = (uint32_t) (id & (LRAT_ID) (uint32_t) (-1));
  CADICAL_assert (id == ((LRAT_ID) id2_higher << 32) + (LRAT_ID) id2_lower);
  chain.push_back (id2_higher);
  chain.push_back (id2_lower);
  LOG (unsimplified, "pushing to chain");
  chain.insert (end (chain), begin (unsimplified), end (unsimplified));
  chain.push_back (0);
}

LRAT_ID Closure::simplify_and_add_to_proof_chain (vector<int> &unsimplified,
                                                  LRAT_ID delete_id) {
  vector<int> &clause = internal->clause;
  CADICAL_assert (clause.empty ());
#ifndef CADICAL_NDEBUG
  for (auto lit : unsimplified) {
    CADICAL_assert (!(marked (lit) & 4));
  }
#endif

  bool trivial = false;
  for (auto lit : unsimplified) {
    signed char &lit_mark = marked (lit);
    if (lit_mark & 4)
      continue;
    signed char &not_lit_mark = marked (-lit);
    if (not_lit_mark & 4) {
      trivial = true;
      break;
    }
    lit_mark |= 4;
    clause.push_back (lit);
  }
  for (auto lit : clause) {
    signed char &mark = marked (lit);
    CADICAL_assert (mark & 4);
    mark &= ~4u;
  }

  LRAT_ID id = 0;
  if (!trivial) {
    if (delete_id) {
      if (internal->proof) {
        internal->proof->delete_clause (delete_id, true, clause);
        lrat_chain.clear ();
      }
    } else {
      id = check_and_add_to_proof_chain (clause);
      add_clause_to_chain (clause, id);
    }
  } else {
    LOG ("skipping trivial proof");
    lrat_chain.clear ();
  }
  clause.clear ();
  return id;
}
/*------------------------------------------------------------------------*/
void Closure::add_ite_turned_and_binary_clauses (Gate *g) {
  if (!internal->proof)
    return;
  if (internal->lrat)
    return;
  LOG ("starting ITE turned AND supporting binary clauses");
  CADICAL_assert (unsimplified.empty ());
  CADICAL_assert (chain.empty ());
  int not_lhs = -g->lhs;
  unsimplified.push_back (not_lhs);
  unsimplified.push_back (g->rhs[0]);
  simplify_and_add_to_proof_chain (unsimplified);
  unsimplified.pop_back ();
  unsimplified.push_back (g->rhs[1]);
  simplify_and_add_to_proof_chain (unsimplified);
  unsimplified.clear ();
}
void Closure::simplify_unit_xor_lrat_clauses (
    const vector<LitClausePair> &source, int lhs) {
  CADICAL_assert (internal->lrat);
  for (auto pair : source) {
    compute_rewritten_clause_lrat_simple (pair.clause, lhs);
    if (lrat_chain.size ())
      break;
  }
  CADICAL_assert (clause.size () == 1);
}
void Closure::simplify_and_sort_xor_lrat_clauses (
    const vector<LitClausePair> &source, vector<LitClausePair> &target,
    int lhs, int except2, bool flip) {
  CADICAL_assert (internal->lrat);
  for (auto pair : source) {
    Clause *c = produce_rewritten_clause_lrat (pair.clause, lhs);
    if (c) {
      target.push_back (LitClausePair (0, c));
    }
  }
  gate_sort_lrat_reasons (target, lhs, except2, flip);
}
void Closure::add_xor_matching_proof_chain (
    Gate *g, int lhs1, const vector<LitClausePair> &clauses2, int lhs2,
    vector<LRAT_ID> &to_lrat, vector<LRAT_ID> &back_lrat) {
  if (lhs1 == lhs2)
    return;
  if (!internal->proof)
    return;
  CADICAL_assert (unsimplified.empty ());
  unsimplified = g->rhs;
  vector<LitClausePair> first;
  vector<LitClausePair> second;
  if (internal->lrat) {
    simplify_and_sort_xor_lrat_clauses (g->pos_lhs_ids, first, lhs1);
    simplify_and_sort_xor_lrat_clauses (clauses2, second, lhs2, 0, 1);
    g->pos_lhs_ids = first;
  }
  LOG ("starting XOR matching proof");
  // for lrat
  vector<LitIdPair> first_ids;
  vector<LitIdPair> second_ids;
  for (auto pair : first) {
    bool first = pair.current_lit & 1;
    int rest = pair.current_lit >> 1;
    rest &= ~(1 << (g->rhs.size () - 1));
    if (first == (lhs1 > 0)) {
      first_ids.push_back (LitIdPair (rest, pair.clause->id));
    } else {
      second_ids.push_back (LitIdPair (rest, pair.clause->id));
    }
    LOG (pair.clause, "key %d", pair.current_lit);
  }
  for (auto pair : second) {
    bool first = pair.current_lit & 1;
    int rest = pair.current_lit >> 1;
    rest &= ~(1 << (g->rhs.size () - 1));
    if (first == (lhs2 < 0)) {
      first_ids.push_back (LitIdPair (rest, pair.clause->id));
    } else {
      second_ids.push_back (LitIdPair (rest, pair.clause->id));
    }
    LOG (pair.clause, "key %d", pair.current_lit);
  }
  // TODO Florian: resort and ids after every round
  do {
    vector<LitIdPair> first_tmp;
    vector<LitIdPair> second_tmp;
    CADICAL_assert (!unsimplified.empty ());
    unsimplified.pop_back ();
    const size_t size = unsimplified.size ();
    CADICAL_assert (size < 32);
    const size_t off = 1u << size;
    for (size_t i = 0; i != off; ++i) {
      int32_t n = 0;
      if (internal->lrat) {
        n = number_from_xor_reason_reversed (unsimplified);
        CADICAL_assert (lrat_chain.empty ());
        for (auto pair : first_ids) {
          if (pair.lit == n)
            lrat_chain.push_back (pair.id);
        }
        CADICAL_assert (lrat_chain.size () == 2);
      }
      unsimplified.push_back (-lhs1);
      unsimplified.push_back (lhs2);
      const LRAT_ID id1 = simplify_and_add_to_proof_chain (unsimplified);
      unsimplified.resize (unsimplified.size () - 2);
      if (internal->lrat) {
        int32_t rest = n &= ~(1 << (unsimplified.size () - 1));
        first_tmp.push_back (LitIdPair (rest, id1));
        n = number_from_xor_reason_reversed (unsimplified);
        lrat_chain.clear ();
        for (auto pair : second_ids) {
          if (pair.lit == n)
            lrat_chain.push_back (pair.id);
        }
        CADICAL_assert (lrat_chain.size () == 2);
      }
      unsimplified.push_back (lhs1);
      unsimplified.push_back (-lhs2);
      const LRAT_ID id2 = simplify_and_add_to_proof_chain (unsimplified);
      unsimplified.resize (unsimplified.size () - 2);
      if (internal->lrat) {
        lrat_chain.clear ();
        int32_t rest = n &= ~(1 << (unsimplified.size () - 1));
        second_tmp.push_back (LitIdPair (rest, id2));
      }
      inc_lits (unsimplified);
    }
    if (internal->lrat) {
      first_ids.swap (first_tmp);
      second_ids.swap (second_tmp);
    }
  } while (!unsimplified.empty ());
  if (internal->lrat) {
    CADICAL_assert (first_ids.size () == 1);
    CADICAL_assert (second_ids.size () == 1);
    to_lrat.push_back (first_ids.back ().id);
    back_lrat.push_back (second_ids.back ().id);
  }
  CADICAL_assert (!internal->lrat || to_lrat.size () == 1);
  CADICAL_assert (!internal->lrat || back_lrat.size () == 1);
  LOG ("finished XOR matching proof");
  CADICAL_assert (unsimplified.empty ());
}

// this function needs to either put the clauses from
// lrat_chain_and_gate into g->pos_neg_ids or clear it or do something with
// it if you merge gates.
Gate *Closure::new_xor_gate (const vector<LitClausePair> &glauses,
                             int lhs) {
  rhs.clear ();

  for (auto lit : lits) {
    if (lhs != lit && -lhs != lit) {
      CADICAL_assert (lit > 0);
      rhs.push_back (lit);
    }
  }
  CADICAL_assert (rhs.size () + 1 == lits.size ());
  sort_literals_by_var (rhs);
  Gate *g = find_xor_lits (this->rhs);
  if (g) {
    check_xor_gate_implied (g);
    std::vector<LRAT_ID> reasons_implication, reasons_back;
    add_xor_matching_proof_chain (g, g->lhs, glauses, lhs,
                                  reasons_implication, reasons_back);
    if (merge_literals_lrat (g->lhs, lhs, reasons_implication,
                             reasons_back)) {
      ++internal->stats.congruence.xors;
    }
    delete_proof_chain ();
    CADICAL_assert (internal->unsat || chain.empty ());
  } else {
    g = new Gate;
    g->lhs = lhs;
    g->tag = Gate_Type::XOr_Gate;
    g->rhs = {rhs};
    g->garbage = false;
    g->indexed = true;
    g->shrunken = false;
    g->hash = hash_lits (nonces, g->rhs);
    for (auto pair : glauses)
      g->pos_lhs_ids.push_back (pair);
    table.insert (g);
    ++internal->stats.congruence.gates;
#ifdef LOGGING
    g->id = fresh_id++;
#endif
    LOG (g, "creating new");
    check_xor_gate_implied (g);
    for (auto lit : g->rhs) {
      connect_goccs (g, lit);
    }
  }
  return g;
}
uint32_t
Closure::number_from_xor_reason_reversed (const std::vector<int> &rhs) {
  uint32_t n = 0;
  CADICAL_assert (is_sorted (rhs.rbegin (), rhs.rend (),
                     sort_literals_by_var_smaller_except (internal, 0, 0)));
  CADICAL_assert (rhs.size () <= 32);
  for (auto r = rhs.rbegin (); r != rhs.rend (); r++) {
    int lit = *r;
    n *= 2;
    n += !(lit > 0);
  }
  return n;
}

uint32_t Closure::number_from_xor_reason (const std::vector<int> &rhs,
                                          int lhs, int except, bool flip) {
  uint32_t n = 0;
  CADICAL_assert (is_sorted (
      begin (rhs), end (rhs),
      sort_literals_by_var_smaller_except (internal, lhs, except)));
  (void) lhs, (void) except;
  CADICAL_assert (rhs.size () <= 32);
  for (auto lit : rhs) {
    n *= 2;
    n += !(lit > 0) ^ flip;
    flip = 0;
  }
  return n;
}

// this is how I planned to sort it and produce the number
// Look at this first
void Closure::gate_sort_lrat_reasons (LitClausePair &litId, int lhs,
                                      int except2, bool flip) {
  CADICAL_assert (clause.empty ());
  std::copy (begin (*litId.clause), end (*litId.clause),
             back_inserter (clause));
  sort_literals_by_var_except (clause, lhs, except2);
  litId.current_lit = number_from_xor_reason (clause, lhs, except2, flip);
  clause.clear ();
}

struct smaller_pair_first_rank {
  typedef size_t Type;
  Type operator() (const LitClausePair &a) { return a.current_lit; }
};

// this is how I planned to sort it and produce the number
// Look at this first
void Closure::gate_sort_lrat_reasons (std::vector<LitClausePair> &xs,
                                      int lhs, int except2, bool flip) {
  CADICAL_assert (clause.empty ());
  CADICAL_assert (!xs.empty ());
  for (auto &litId : xs) {
    gate_sort_lrat_reasons (litId, lhs, except2, flip);
  }

  rsort (begin (xs), end (xs), smaller_pair_first_rank ());

#ifndef CADICAL_NDEBUG
  std::for_each (begin (xs), end (xs), [&xs] (const LitClausePair &x) {
    CADICAL_assert (x.clause->size == xs[1].clause->size);
  });
#endif
}

void Closure::init_xor_gate_extraction (std::vector<Clause *> &candidates) {
  const unsigned arity_limit = internal->opts.congruencexorarity;
  CADICAL_assert (arity_limit < 32); // we use unsigned int.
  const unsigned size_limit = arity_limit + 1;
  glargecounts.resize (2 * internal->vsize, 0);

  for (auto c : internal->clauses) {
    LOG (c, "considering clause for XOR");
    if (c->redundant)
      continue;
    if (c->garbage)
      continue;
    if (c->size < 3)
      continue;
    unsigned size = 0;
    for (auto lit : *c) {
      const signed char v = internal->val (lit);
      if (v < 0)
        continue;
      if (v > 0) {
        LOG (c, "satisfied by %d", lit);
        internal->mark_garbage (c);
        goto CONTINUE_COUNTING_NEXT_CLAUSE;
      }
      if (size == size_limit)
        goto CONTINUE_COUNTING_NEXT_CLAUSE;
      ++size;
    }

    if (size < 3)
      continue;
    for (auto lit : *c) {
      if (internal->val (lit))
        continue;
      ++largecounts (lit);
    }

    LOG (c, "considering clause for XOR as candidate");
    candidates.push_back (c);
  CONTINUE_COUNTING_NEXT_CLAUSE:;
  }

  LOG ("considering %zd out of %zd", candidates.size (),
       internal->irredundant ());
  const unsigned rounds = internal->opts.congruencexorcounts;
#ifdef LOGGING
  const size_t original_size = candidates.size ();
#endif
  LOG ("resizing glargecounts to size %zd", glargecounts.size ());
  for (unsigned round = 0; round < rounds; ++round) {
    LOG ("round %d of XOR extraction", round);
    size_t removed = 0;
    gnew_largecounts.resize (2 * internal->vsize);
    unsigned cand_size = candidates.size ();
    size_t j = 0;
    for (size_t i = 0; i < cand_size; ++i) {
      Clause *c = candidates[i];
      LOG (c, "considering");
      unsigned size = 0;
      for (auto lit : *c) {
        if (!internal->val (lit))
          ++size;
      }
      CADICAL_assert (3 <= size);
      CADICAL_assert (size <= size_limit);
      const unsigned arity = size - 1;
      const unsigned needed_clauses = 1u << (arity - 1);
      for (auto lit : *c) {
        if (largecounts (lit) < needed_clauses) {
          LOG (c, "not enough occurrences, so ignoring");
          removed++;
          goto CONTINUE_WITH_NEXT_CANDIDATE_CLAUSE;
        }
      }
      for (auto lit : *c)
        if (!internal->val (lit))
          new_largecounts (lit)++;
      candidates[j++] = candidates[i];

    CONTINUE_WITH_NEXT_CANDIDATE_CLAUSE:;
    }
    candidates.resize (j);
    glargecounts = std::move (gnew_largecounts);
    gnew_largecounts.clear ();
    LOG ("moving counts %zd", glargecounts.size ());
    if (!removed)
      break;

    LOG ("after round %d, %zd (%ld %%) remain", round, candidates.size (),
         candidates.size () / (1 + original_size) * 100);
  }

  for (auto c : candidates) {
    for (auto lit : *c)
      internal->occs (lit).push_back (c);
  }
}

Clause *Closure::find_large_xor_side_clause (std::vector<int> &lits) {
  unsigned least_occurring_literal = 0;
  unsigned count_least_occurring = UINT_MAX;
  const size_t size_lits = lits.size ();
#if defined(LOGGING) || !defined(CADICAL_NDEBUG)
  const unsigned arity = size_lits - 1;
#endif
#ifndef CADICAL_NDEBUG
  const unsigned count_limit = 1u << (arity - 1);
#endif
  LOG (lits, "trying to find arity %u XOR side clause", arity);
  for (auto lit : lits) {
    CADICAL_assert (!internal->val (lit));
    marked (lit) = 1;
    unsigned count = largecount (lit);
    CADICAL_assert (count_limit <= count);
    if (count >= count_least_occurring)
      continue;
    count_least_occurring = count;
    least_occurring_literal = lit;
  }
  Clause *res = 0;
  CADICAL_assert (least_occurring_literal);
  LOG ("searching XOR side clause watched by %d#%u",
       least_occurring_literal, count_least_occurring);
  LOG ("searching for size %ld", size_lits);
  for (auto c : internal->occs (least_occurring_literal)) {
    LOG (c, "checking");
    CADICAL_assert (c->size != 2); // TODO kissat has break
    if (c->garbage)
      continue;
    if ((size_t) c->size < size_lits)
      continue;
    size_t found = 0;
    for (auto other : *c) {
      const signed char value = internal->val (other);
      if (value < 0)
        continue;
      if (value > 0) {
        LOG (c, "found satisfied %d in", other);
        internal->mark_garbage (c);
        CADICAL_assert (c->garbage);
        break;
      }
      if (marked (other))
        found++;
      else {
        LOG ("not marked %d", other);
        found = 0;
        break;
      }
    }
    if (found == size_lits && !c->garbage) {
      res = c;
      break;
    } else {
      LOG ("too few literals");
    }
  }
  for (auto lit : lits)
    marked (lit) = 0;
  if (res)
    LOG (res, "found matching XOR side");
  else
    LOG ("no matching XOR side clause found");
  return res;
}

void Closure::extract_xor_gates_with_base_clause (Clause *c) {
  LOG (c, "checking clause");
  lits.clear ();
  int smallest = 0;
  int largest = 0;
  const unsigned arity_limit = internal->opts.congruencexorarity;
  const unsigned size_limit = arity_limit + 1;
  unsigned negated = 0, size = 0;
  bool first = true;
  for (auto lit : *c) {
    const signed char v = internal->val (lit);
    if (v < 0)
      continue;
    if (v > 0) {
      internal->mark_garbage (c);
      return;
    }
    if (size == size_limit) {
      LOG (c, "size limit reached");
      return;
    }

    if (first) {
      largest = smallest = lit;
      first = false;
    } else {
      CADICAL_assert (smallest);
      CADICAL_assert (largest);
      if (internal->vlit (lit) < internal->vlit (smallest)) {
        LOG ("new smallest %d", lit);
        smallest = lit;
      }
      if (internal->vlit (lit) > internal->vlit (largest)) {
        if (largest < 0) {
          LOG (c, "not largest %d (largest: %d) occurs negated in XOR base",
               lit, largest);
          return;
        }
        largest = lit;
      }
    }
    if (lit < 0 && internal->vlit (lit) < internal->vlit (largest)) {
      LOG (c, "negated literal %d not largest in XOR base", lit);
      return;
    }
    if (lit < 0 && negated++) {
      LOG (c, "more than one negated literal in XOR base");
      return;
    }
    lits.push_back (lit);
    ++size;
  }
  CADICAL_assert (size == lits.size ());
  if (size < 3) {
    LOG (c, "short XOR base clause");
    return;
  }

  LOG ("double checking if possible");
  const unsigned arity = size - 1;
  const unsigned needed_clauses = 1u << (arity - 1);
  for (auto lit : lits) {
    for (int sign = 0; sign != 2; ++sign, lit = -lit) {
      unsigned count = largecount (lit);
      if (count >= needed_clauses)
        continue;
      LOG (c,
           "literal %d in XOR base clause only occurs %u times in large "
           "clause thus skipping",
           lit, count);
      return;
    }
  }

  LOG ("checking for XOR side clauses");
  CADICAL_assert (smallest && largest);
  const unsigned end = 1u << arity;
  CADICAL_assert (negated == parity_lits (lits));
  unsigned found = 0;
  vector<LitClausePair> glauses;
  glauses.push_back (LitClausePair (0, c));
  for (unsigned i = 0; i != end; ++i) {
    while (i && parity_lits (lits) != negated)
      inc_lits (lits);
    if (i) {
      // the clause must be stored
      // you can use `lrat_chain_and_gate` for this
      Clause *d = find_large_xor_side_clause (lits);
      if (!d)
        return;
      CADICAL_assert (!d->redundant);
      glauses.push_back (LitClausePair (i, d));
    } else
      CADICAL_assert (!c->redundant);
    inc_lits (lits);
    ++found;
  }

  while (parity_lits (lits) != negated)
    inc_lits (lits);
  LOG (lits, "found all needed %u matching clauses:", found);
  CADICAL_assert (found == 1u << arity);
  if (negated) {
    auto p = begin (lits);
    int lit;
    while ((lit = *p) > 0)
      p++;
    LOG ("flipping RHS literal %d", (lit));
    *p = -lit;
  }
  LOG (lits, "normalized negations");
  unsigned extracted = 0;
  for (auto lhs : lits) {
    if (!negated)
      lhs = -lhs;
    Gate *g = new_xor_gate (glauses, lhs);
    if (g)
      extracted++;
    if (internal->unsat)
      break;
  }
  if (!extracted)
    LOG ("no arity %u XOR gate extracted", arity);
}
void Closure::extract_xor_gates () {
  CADICAL_assert (!full_watching);
  if (!internal->opts.congruencexor)
    return;
  START (extractxors);
  LOG ("starting extracting XOR");
  std::vector<Clause *> candidates = {};
  init_xor_gate_extraction (candidates);
  for (auto c : candidates) {
    if (internal->unsat)
      break;
    if (c->garbage)
      continue;
    extract_xor_gates_with_base_clause (c);
  }
  reset_xor_gate_extraction ();
  STOP (extractxors);
}

/*------------------------------------------------------------------------*/
void Closure::find_units () {
  size_t units = 0;
  for (auto v : internal->vars) {
  RESTART:
    if (!internal->flags (v).active ())
      continue;
    for (int sgn = -1; sgn < 1; sgn += 2) {
      int lit = v * sgn;
      for (auto w : internal->watches (lit)) {
        if (!w.binary ())
          continue; // todo check that binaries first
        const int other = w.blit;
        if (marked (-other)) {
          LOG (w.clause, "binary clause %d %d and %d %d give unit %d", lit,
               other, lit, -other, lit);
          ++units;
          if (internal->lrat) {
            lrat_chain.push_back (w.clause->id);
            lrat_chain.push_back (marked_mu1 (-other).clause->id);
          }
          bool failed = !learn_congruence_unit (lit);
          unmark_all ();
          if (failed)
            return;
          else
            goto RESTART;
        }
        if (marked (other))
          continue;
        marked (other) = 1;
        set_mu1_reason (other, w.clause);
        internal->analyzed.push_back (other);
      }
      unmark_all ();
    }
    CADICAL_assert (internal->analyzed.empty ());
  }
  LOG ("found %zd units", units);
}

void Closure::find_equivalences () {
  CADICAL_assert (!internal->unsat);

  for (auto v : internal->vars) {
  RESTART:
    if (!internal->flags (v).active ())
      continue;
    int lit = v;
    for (auto w : internal->watches (lit)) {
      if (!w.binary ())
        break;
      CADICAL_assert (w.size == 2);
      const int other = w.blit;
      if (internal->vlit (lit) > internal->vlit (other))
        continue;
      if (marked (other))
        continue;
      internal->analyzed.push_back (other);
      marked (other) = true;
      set_mu1_reason (other, w.clause);
    }

    if (internal->analyzed.empty ())
      continue;

    for (auto w : internal->watches (-lit)) {
      if (!w.binary ())
        break; // binary clauses are first
      const int other = w.blit;
      if (internal->vlit (-lit) > internal->vlit (other))
        continue;
      CADICAL_assert (-lit != other);
      LOG ("binary clause %d %d", -lit, other);
      if (marked (-other)) {
        int lit_repr = find_representative (lit);
        int other_repr = find_representative (other);
        LOG ("found equivalence %d %d with %d and %d as the representative",
             lit, other, lit_repr, other_repr);
        if (lit_repr != other_repr) {
          // if (internal->lrat) {
          //   // This cannot work
          //   // if you have 2 = 1 and 3=4
          //   // you cannot add 2=3. You really to connect the
          //   representatives directly
          //   // therefore you actually need to learn the clauses 2->3->4
          //   and -2->1 and vice-versa eager_representative_id (other) =
          //   marked_mu1 (-other).clause->id; eager_representative_id
          //   (-other) = w.clause->id; CADICAL_assert (eager_representative_id
          //   (other) != -1); LOG ("lrat: %d (%zd) %d (%zd)", other,
          //   eager_representative_id (other), -other,
          //   eager_representative_id (-other));
          // }
          promote_clause (marked_mu1 (-other).clause);
          promote_clause (w.clause);
          LOG (w.clause, "merging");
          LOG (marked_mu1 (-other).clause, "with");
          if (merge_literals_equivalence (
                  lit, other,
                  internal->lrat ? marked_mu1 (-other).clause : nullptr,
                  w.clause)) {
            ++internal->stats.congruence.congruent;
          }
          unmark_all ();
          if (internal->unsat)
            return;
          else
            goto RESTART;
        }
      }
    }
    unmark_all ();
  }
  CADICAL_assert (internal->analyzed.empty ());
  LOG ("found %zd equivalences", schedule.size ());
}

/*------------------------------------------------------------------------*/
// Initialization

void Closure::rewrite_and_gate (Gate *g, int dst, int src, LRAT_ID id1,
                                LRAT_ID id2) {
  if (skip_and_gate (g))
    return;
  if (!gate_contains (g, src))
    return;
  if (internal->val (src)) {
    // In essence the code below does the same thing as simplify_and_gate
    // but the necessary LRAT chain are different.
    simplify_and_gate (g);
    return;
  }
  CADICAL_assert (src);
  CADICAL_assert (dst);
  CADICAL_assert (internal->val (src) == internal->val (dst));
  GatesTable::iterator git = (g->indexed ? table.find (g) : end (table));
  LOG (g, "rewriting %d into %d in", src, dst);
  int clashing = 0, falsifies = 0;
  unsigned dst_count = 0, not_dst_count = 0;
  auto q = begin (g->rhs);
  for (int &lit : g->rhs) {
    if (lit == src)
      lit = dst;
    if (lit == -g->lhs) {
      LOG ("found negated LHS literal %d", lit);
      clashing = lit;
      g->degenerated_and_neg = true;
      break;
    }
    if (lit == g->lhs)
      g->degenerated_and_pos = true;
    const signed char val = internal->val (lit);
    if (val > 0) {
      continue;
    }
    if (val < 0) {
      LOG ("found falsifying literal %d", (lit));
      falsifies = lit;
      break;
    }
    if (lit == dst) {
      if (not_dst_count) {
        LOG ("clashing literals %d and %d", (-dst), (dst));
        clashing = -dst;
        break;
      }
      if (dst_count++)
        continue;
    }
    if (lit == -dst) {
      if (dst_count) {
        CADICAL_assert (!not_dst_count);
        LOG ("clashing literals %d and %d", (dst), (-dst));
        clashing = -dst;
        break;
      }
      CADICAL_assert (!not_dst_count);
      ++not_dst_count;
    }
    *q++ = lit;
  }
  LOG (lrat_chain, "lrat chain after rewriting");

  if (internal->lrat) { // updating reasons in the chain.
#ifdef LOGGING
    for (auto litId : g->pos_lhs_ids) {
      LOG (litId.clause, "%d ->", litId.current_lit);
    }
#endif
    // We remove all assigned literals except the falsified literal such
    // that we can produce an LRAT chain
    size_t i = 0, size = g->pos_lhs_ids.size ();
    bool found = false;
    CADICAL_assert (!falsifies || !clashing);
    const int orig_falsifies = falsifies == dst ? src : falsifies;
    const int orig_clashing =
      clashing == -dst ? -src : (clashing == dst ? src : clashing);
    int keep_clashing = clashing;
    LOG ("keeping chain for falsifies: %d aka %d and clashing: %d aka %d",
         falsifies, orig_falsifies, clashing, orig_clashing);
    for (size_t j = 0; j < size; ++j) {
      LOG (g->pos_lhs_ids[j].clause, "looking at %d [%zd %zd] with val %d",
           g->pos_lhs_ids[j].current_lit, i, j,
           internal->val (g->pos_lhs_ids[i].current_lit));
      g->pos_lhs_ids[i] = g->pos_lhs_ids[j];
      if (keep_clashing && g->pos_lhs_ids[i].current_lit != orig_clashing &&
          g->pos_lhs_ids[i].current_lit != -orig_clashing &&
          g->pos_lhs_ids[i].current_lit != keep_clashing &&
          g->pos_lhs_ids[i].current_lit != -keep_clashing)
        continue;
      if (internal->val (g->pos_lhs_ids[i].current_lit) &&
          g->pos_lhs_ids[i].current_lit != src &&
          g->pos_lhs_ids[i].current_lit != orig_falsifies)
        continue;
      if (g->pos_lhs_ids[i].current_lit == dst) {
        if (!found)
          found = true;
        else
          continue; // we have already one defining clause
      }

      LOG ("maybe keeping %d [%zd %zd], src: %d, found: %d",
           g->pos_lhs_ids[i].current_lit, i, j, src, found);
      if (g->pos_lhs_ids[i].current_lit == src) {
        if (!found)
          g->pos_lhs_ids[i].current_lit = dst, found = true;
        else
          continue; // we have already one defining clause
      }
      LOG ("keeping %d [%zd %zd]", g->pos_lhs_ids[i].current_lit, i, j);
      ++i;
    }
    LOG ("resizing to %zd", i);
    CADICAL_assert (i);
    g->pos_lhs_ids.resize (i);
  }

  if (q != end (g->rhs)) {
    g->rhs.resize (q - begin (g->rhs));
    g->shrunken = true;
  }
  CADICAL_assert (dst_count <= 2);
  CADICAL_assert (not_dst_count <= 1);

  std::vector<LRAT_ID> reasons_lrat_src, reasons_lrat_usrc;
  shrink_and_gate (g, falsifies, clashing);
  LOG (g, "rewritten as");
  CADICAL_assert (!internal->lrat || !g->pos_lhs_ids.empty ());
  //  check_and_gate_implied (g);
  update_and_gate (g, git, src, dst, id1, id2, falsifies, clashing);
  ++internal->stats.congruence.rewritten_ands;
}

bool Closure::rewrite_gate (Gate *g, int dst, int src, LRAT_ID id1,
                            LRAT_ID id2) {
  switch (g->tag) {
  case Gate_Type::And_Gate:
    rewrite_and_gate (g, dst, src, id1, id2);
    break;
  case Gate_Type::XOr_Gate:
    rewrite_xor_gate (g, dst, src);
    break;
  case Gate_Type::ITE_Gate:
    rewrite_ite_gate (g, dst, src);
    break;
  default:
    CADICAL_assert (false);
    break;
  }
  CADICAL_assert (internal->unsat || lrat_chain.empty ());
  return !internal->unsat;
}

bool Closure::rewrite_gates (int dst, int src, LRAT_ID id1, LRAT_ID id2) {
  const auto &occs = goccs (src);
  for (auto g : occs) {
    CADICAL_assert (lrat_chain.empty ());
    if (!rewrite_gate (g, dst, src, id1, id2))
      return false;
    else if (!g->garbage && gate_contains (g, dst))
      goccs (dst).push_back (g);
  }
  goccs (src).clear ();

#ifndef CADICAL_NDEBUG
  for (const auto &occs : gtab) {
    for (auto g : occs) {
      CADICAL_assert (g);
      CADICAL_assert (g->garbage || !gate_contains (g, src));
    }
  }
#endif
  CADICAL_assert (lrat_chain.empty ());
  return true;
}

bool Closure::rewriting_lhs (Gate *g, int dst) {
  if (dst != g->lhs && dst != -g->lhs)
    return false;
  mark_garbage (g);
  return true;
}

// update to produce proofs
void Closure::rewrite_xor_gate (Gate *g, int dst, int src) {
  if (skip_xor_gate (g))
    return;
  if (rewriting_lhs (g, dst))
    return;
  if (!gate_contains (g, src))
    return;
  LOG (g, "rewriting (%d -> %d)", src, dst);
  check_xor_gate_implied (g);
  GatesTable::iterator git = (g->indexed ? table.find (g) : end (table));
  size_t j = 0, dst_count = 0;
  bool original_dst_negated = (dst < 0);
  dst = abs (dst);
  unsigned negate = original_dst_negated;
  const size_t size = g->rhs.size ();
  for (size_t i = 0; i < size; ++i) {
    int lit = g->rhs[i];
    CADICAL_assert (lit > 0);
    if (lit == src)
      lit = dst;
    const signed char v = internal->val (lit);
    if (v > 0) {
      negate ^= true;
    }
    if (v)
      continue;
    if (lit == dst)
      dst_count++;
    LOG ("keeping value %d", lit);
    g->rhs[j++] = lit;
  }
  if (negate) {
    LOG ("flipping LHS %d", g->lhs);
    g->lhs = -g->lhs;
  }
  CADICAL_assert (dst_count <= 2);
  if (dst_count == 2) {
    LOG ("destination found twice, removing");
    size_t k = 0;
    for (size_t i = 0; i < j; ++i) {
      const int lit = g->rhs[i];
      if (lit != dst)
        g->rhs[k++] = g->rhs[i];
    }
    CADICAL_assert (k == j - 2);
    g->rhs.resize (k);
    g->shrunken = true;
    CADICAL_assert (is_sorted (begin (g->rhs), end (g->rhs),
                       sort_literals_by_var_smaller (internal)));
    g->hash = hash_lits (nonces, g->rhs);
  } else if (j != size) {
    g->shrunken = true;
    g->rhs.resize (j);
    sort_literals_by_var (g->rhs);
    g->hash = hash_lits (
        nonces,
        g->rhs); // all but one (the dst) is sorted correctly actually
  } else {
    CADICAL_assert (j == size);
    sort_literals_by_var (g->rhs);
  }

  CADICAL_assert (clause.empty ());
  // LRAT for add_xor_shrinking_proof_chain
  // this should be unnecessary...
  // TODO check if really unnecessary
  if (dst_count > 1)
    add_xor_shrinking_proof_chain (g, dst);
  CADICAL_assert (internal->clause.size () <= 1);
  update_xor_gate (g, git);

  if (!g->garbage && !internal->unsat && original_dst_negated &&
      dst_count == 1) {
    connect_goccs (g, dst);
  }

  check_xor_gate_implied (g);
  // TODO stats
}

// update to produce proofs
void Closure::simplify_xor_gate (Gate *g) {
  LOG (g, "simplifying");
  if (skip_xor_gate (g))
    return;
  check_xor_gate_implied (g);
  unsigned negate = 0;
  GatesTable::iterator git = (g->indexed ? table.find (g) : end (table));
  const size_t size = g->rhs.size ();
  size_t j = 0;
  for (size_t i = 0; i < size; ++i) {
    int lit = g->rhs[i];
    CADICAL_assert (lit > 0);
    const signed char v = internal->val (lit);
    if (v > 0)
      negate ^= 1;
    if (!v) {
      g->rhs[j++] = lit;
    }
  }
  if (negate) {
    LOG ("flipping LHS literal %d", (g->lhs));
    g->lhs = -(g->lhs);
  }
  if (j != size) {
    LOG ("shrunken gate");
    g->shrunken = true;
    g->rhs.resize (j);
    CADICAL_assert (is_sorted (begin (g->rhs), end (g->rhs),
                       sort_literals_by_var_smaller (internal)));
    g->hash = hash_lits (nonces, g->rhs);
  } else {
    CADICAL_assert (g->hash == hash_lits (nonces, g->rhs));
  }

  check_xor_gate_implied (g);
  CADICAL_assert (clause.empty ());
  update_xor_gate (g, git);
  LOG (g, "simplified");
  check_xor_gate_implied (g);
  internal->stats.congruence.simplified++;
  internal->stats.congruence.simplified_xors++;
}

/*------------------------------------------------------------------------*/
// propagation of clauses and simplification
void Closure::schedule_literal (int lit) {
  const int idx = abs (lit);
  if (scheduled[idx])
    return;
  scheduled[idx] = true;
  schedule.push (lit);
  CADICAL_assert (lit != find_representative (lit));
  LOG ("scheduled literal %d", lit);
}

bool Closure::propagate_unit (int lit) {
  LOG ("propagation of congruence unit %d", lit);
  if (internal->lrat)
    lazy_propagated (lit) = true;
  return simplify_gates (lit) && simplify_gates (-lit);
}

bool Closure::propagate_units () {
  while (units !=
         internal->trail
             .size ()) { // units are added during propagation, so reloading
    LOG ("propagating %d over gates", internal->trail[units]);
    if (!propagate_unit (internal->trail[units++]))
      return false;
  }
  return true;
}

// The replacement has to be done eagerly, not lazily to make sure that the
// gates are in normalized form. Otherwise, some merges might be missed.
bool Closure::propagate_equivalence (int lit) {
  if (internal->val (lit))
    return true;
  LOG ("propagating literal %d", lit);
  import_lazy_and_find_eager_representative_and_compress_both (lit);
  const int repr = find_eager_representative_and_compress (lit);
  const LRAT_ID id1 = find_eager_representative_lrat (lit);
  const LRAT_ID id2 = find_eager_representative_lrat (-lit);
  CADICAL_assert (lrat_chain.empty ());
  return rewrite_gates (repr, lit, id1, id2) &&
         rewrite_gates (-repr, -lit, id2, id1);
}

size_t Closure::propagate_units_and_equivalences () {
  START (congruencemerge);
  size_t propagated = 0;
  LOG ("propagating at least %zd units", schedule.size ());
  CADICAL_assert (lrat_chain.empty ());
  while (propagate_units () && !schedule.empty ()) {
    CADICAL_assert (!internal->unsat);
    CADICAL_assert (lrat_chain.empty ());
    ++propagated;
    int lit = schedule.front ();
    schedule.pop ();
    scheduled[abs (lit)] = false;
    if (!propagate_equivalence (lit))
      break;
  }

  CADICAL_assert (internal->unsat || schedule.empty ());
  CADICAL_assert (internal->unsat || lrat_chain.empty ());

  LOG ("propagated %zu congruence units", units);
  LOG ("propagated %zu congruence equivalences", propagated);

#ifndef CADICAL_NDEBUG
  if (!internal->unsat) {
    for (const auto &occs : gtab) {
      for (auto g : occs) {
        if (g->garbage)
          continue;
        CADICAL_assert (g->tag == Gate_Type::ITE_Gate ||
                g->tag == Gate_Type::XOr_Gate ||
                !gate_contains (g, -g->lhs));
        // TODO: this would be nice to have!
        //      CADICAL_assert (g->tag != Gate_Type::ITE_Gate || (g->rhs.size() == 3
        //      && g->rhs[1] != -g->lhs && g->rhs[2] != -g->lhs));
        // CADICAL_assert (table.count(g) == 1);
        for (auto lit : g->rhs) {
          CADICAL_assert (!internal->val (lit));
          CADICAL_assert (representative (lit) == lit);
        }
      }
    }
    for (Gate *g : table) {
      if (g->garbage)
        continue;
      if (g->tag == Gate_Type::And_Gate) {
        // CADICAL_assert (find_and_lits(g->arity, g->rhs));
      }
    }
  }
#endif
  STOP (congruencemerge);
  return propagated;
}

std::string string_of_gate (Gate_Type t) {
  switch (t) {
  case Gate_Type::And_Gate:
    return "And";
  case Gate_Type::XOr_Gate:
    return "XOr";
  case Gate_Type::ITE_Gate:
    return "ITE";
  default:
    return "buggy";
  }
}

void Closure::reset_closure () {
  scheduled.clear ();
  for (Gate *g : table) {
    CADICAL_assert (g->indexed);
    LOG (g, "deleting");
    if (!g->garbage)
      delete g;
  }
  table.clear ();

  for (auto &occ : gtab) {
    occ.clear ();
  }
  gtab.clear ();

  for (auto gate : garbage)
    delete gate;
  garbage.clear ();

  if (internal->lrat) {
    CADICAL_assert (internal->proof);
    for (auto c : extra_clauses) {
      CADICAL_assert (!c->garbage);
      internal->proof->delete_clause (c);
      delete c;
    }
    extra_clauses.clear ();
  } else {
    CADICAL_assert (extra_clauses.empty ());
  }
}

void Closure::reset_extraction () {
  full_watching = true;
  if (!internal->unsat && !internal->propagate ()) {
    internal->learn_empty_clause ();
  }

#if 0
  // remove delete watched clauses from the watch list
  for (auto v : internal->vars) {
    for (auto sgn = -1; sgn <= 1; sgn += 2) {
      const int lit = v * sgn;
      auto &watchers = internal->watches (lit);
      const size_t size = watchers.size ();
      size_t j = 0;
      for (size_t i = 0; i != size; ++i) {
	const auto w = watchers[i];
	watchers[j] = watchers[i];
	if (!w.clause->garbage)
	  ++j;
      }
      watchers.resize(j);
    }
  }
  // watch the remaining non-watched clauses
  for (auto c : new_unwatched_binary_clauses)
    internal->watch_clause (c);
  new_unwatched_binary_clauses.clear();
  for (auto c : internal->clauses) {
    if (c->garbage)
      continue;
    if (c->size != 2)
      internal->watch_clause (c);
  }
#else // simpler implementation
  new_unwatched_binary_clauses.clear ();
  internal->clear_watches ();
  internal->connect_watches ();
#endif
}

void Closure::forward_subsume_matching_clauses () {
  START (congruencematching);
  reset_closure ();
  std::vector<signed char> matchable;
  matchable.resize (internal->max_var + 1);
  size_t count_matchable = 0;

  for (auto idx : internal->vars) {
    if (!internal->flags (idx).active ())
      continue;
    const int lit = idx;
    const int repr = find_representative (lit);
    if (lit == repr)
      continue;
    const int repr_idx = abs (repr);
    if (!matchable[idx]) {
      LOG ("matchable %d", idx);
      matchable[idx] = true;
      count_matchable++;
    }

    if (!matchable[repr_idx]) {
      LOG ("matchable %d", repr_idx);
      matchable[repr_idx] = true;
      count_matchable++;
    }
  }

  LOG ("found %.0f%%",
       (double) count_matchable /
           (double) (internal->max_var ? internal->max_var : 1));
  std::vector<Clause *> candidates;
  auto &analyzed = internal->analyzed;

  for (auto *c : internal->clauses) {
    if (c->garbage)
      continue;
    if (c->redundant)
      continue;
    if (c->size == 2)
      continue;
    CADICAL_assert (analyzed.empty ());
    bool contains_matchable = false;
    for (auto lit : *c) {
      const signed char v = internal->val (lit);
      if (v < 0)
        continue;
      if (v > 0) {
        LOG (c, "mark satisfied");
        internal->mark_garbage (c);
        break;
      }
      if (!contains_matchable) {
        const int idx = abs (lit);
        if (matchable[idx])
          contains_matchable = true;
      }

      const int repr = find_representative (lit);
      CADICAL_assert (!internal->val (repr));
      if (marked (repr))
        continue;
      const int not_repr = -repr;
      if (marked (not_repr)) {
        LOG (c, "matches both %d and %d", (lit), (not_repr));
        internal->mark_garbage (c);
        break;
      }
      marked (repr) = 1;
      analyzed.push_back (repr);
    }

    for (auto lit : analyzed)
      marked (lit) = 0;
    analyzed.clear ();
    if (c->garbage)
      continue;
    if (!contains_matchable) {
      LOG ("no matching variable");
      continue;
    }
    LOG (c, "candidate");
    candidates.push_back (c);
  }

  rsort (begin (candidates), end (candidates), smaller_clause_size_rank ());
  size_t tried = 0, subsumed = 0;
  internal->init_occs ();
  for (auto c : candidates) {
    CADICAL_assert (c->size != 2);
    // TODO if terminated
    ++tried;
    if (find_subsuming_clause (c)) {
      ++subsumed;
    }
  }
  LOG ("[congruence] subsumed %.0f%%",
       (double) subsumed / (double) (tried ? tried : 1));
  STOP (congruencematching);
}

/*------------------------------------------------------------------------*/
// Candidate clause 'subsumed' is subsumed by 'subsuming'.  We need to copy
// the function because 'congruence' is too early to include the version
// from subsume

void Closure::subsume_clause (Clause *subsuming, Clause *subsumed) {
  //  CADICAL_assert (!subsuming->redundant);
  // CADICAL_assert (!subsumed->redundant);
  auto &stats = internal->stats;
  stats.subsumed++;
  CADICAL_assert (subsuming->size <= subsumed->size);
  LOG (subsumed, "subsumed");
  if (subsumed->redundant)
    stats.subred++;
  else
    stats.subirr++;
  if (subsumed->redundant || !subsuming->redundant) {
    internal->mark_garbage (subsumed);
    return;
  }
  LOG ("turning redundant subsuming clause into irredundant clause");
  subsuming->redundant = false;
  if (internal->proof)
    internal->proof->strengthen (subsuming->id);
  internal->mark_garbage (subsumed);
  stats.current.irredundant++;
  stats.added.irredundant++;
  stats.irrlits += subsuming->size;
  CADICAL_assert (stats.current.redundant > 0);
  stats.current.redundant--;
  CADICAL_assert (stats.added.redundant > 0);
  stats.added.redundant--;
  // ... and keep 'stats.added.total'.
}

bool Closure::find_subsuming_clause (Clause *subsumed) {
  CADICAL_assert (!subsumed->garbage);
  Clause *subsuming = nullptr;
  for (auto lit : *subsumed) {
    CADICAL_assert (internal->val (lit) <= 0);
    const int repr_lit = find_representative (lit);
    const signed char repr_val = internal->val (repr_lit);
    CADICAL_assert (repr_val <= 0);
    if (repr_val < 0)
      continue;
    if (marked (repr_lit))
      continue;
    CADICAL_assert (!marked (-repr_lit));
    marked (repr_lit) = 1;
  }
  int least_occuring_lit = 0;
  size_t count_least_occurring = INT_MAX;
  LOG (subsumed, "trying to forward subsume");

  for (auto lit : *subsumed) {
    const int repr_lit = find_representative (lit);
    const size_t count = internal->occs (lit).size ();
    CADICAL_assert (count <= UINT_MAX);
    if (count < count_least_occurring) {
      count_least_occurring = count;
      least_occuring_lit = repr_lit;
    }
    for (auto d : internal->occs (lit)) {
      CADICAL_assert (!d->garbage);
      CADICAL_assert (subsumed != d);
      if (!subsumed->redundant && d->redundant)
        continue;
      for (auto other : *d) {
        const signed char v = internal->val (other);
        if (v < 0)
          continue;
        CADICAL_assert (!v);
        const int repr_other = find_representative (other);
        if (!marked (repr_other))
          goto CONTINUE_WITH_NEXT_CLAUSE;
        LOG ("subsuming due to %d -> %d", other, repr_other);
      }
      subsuming = d;
      goto FOUND_SUBSUMING;

    CONTINUE_WITH_NEXT_CLAUSE:;
    }
  }
  CADICAL_assert (least_occuring_lit);

FOUND_SUBSUMING:
  for (auto lit : *subsumed) {
    const int repr_lit = find_representative (lit);
    const signed char v = internal->val (lit);
    if (!v)
      marked (repr_lit) = 0;
  }
  if (subsuming) {
    LOG (subsumed, "subsumed");
    LOG (subsuming, "subsuming");
    subsume_clause (subsuming, subsumed);
    ++internal->stats.congruence.subsumed;
    return true;
  } else {
    internal->occs (least_occuring_lit).push_back (subsumed);
    return false;
  }
}

/*------------------------------------------------------------------------*/
static bool skip_ite_gate (Gate *g) {
  CADICAL_assert (g->tag == Gate_Type::ITE_Gate);
  if (g->garbage)
    return true;
  return false;
}

void Closure::produce_ite_merge_then_else_reasons (
    Gate *g, int src, int dst, std::vector<LRAT_ID> &reasons_implication,
    std::vector<LRAT_ID> &reasons_back) {
  CADICAL_assert (!g->garbage);
  if (!internal->lrat)
    return;
  check_correct_ite_flags (g);
  // no merge is happening actually
  CADICAL_assert (g->rhs[1] == find_eager_representative(g->rhs[1]) || g->rhs[2] == find_eager_representative(g->rhs[2]));
  if (find_eager_representative (g->lhs) == g->rhs[1] || find_eager_representative (g->lhs) == g->rhs[2])
    return;
  if ((g->rhs[1] == src && g->lhs == dst && g->rhs[2] == g->lhs) ||
      (g->rhs[2] == src && g->lhs == dst && g->rhs[1] == g->lhs) ||
      (g->rhs[1] == -src && g->lhs == -dst && g->rhs[2] == g->lhs) ||
      (g->rhs[2] == -src && g->lhs == -dst && g->rhs[1] == g->lhs))
    return;
  check_ite_lrat_reasons (g, false);
  CADICAL_assert (g->rhs.size () == 3);
  CADICAL_assert (src == g->rhs[1] || src == g->rhs[2]);
  CADICAL_assert (dst == g->rhs[1] || dst == g->rhs[2]);
  const int8_t flag = g->degenerated_ite;
  CADICAL_assert (!ite_flags_no_then_clauses (flag)); // e = lhs: already merged
  CADICAL_assert (!ite_flags_no_else_clauses (flag)); // t = lhs: already merged
  produce_rewritten_clause_lrat (g->pos_lhs_ids, g->lhs, false);
  if (ite_flags_neg_cond_lhs (flag)) {
    LOG ("degenerated case with lhs = -cond");
    LOG (g->pos_lhs_ids[0].clause, "1:");
    LOG (g->pos_lhs_ids[1].clause, "2:");
    reasons_back.push_back (g->pos_lhs_ids[0].clause->id);
    reasons_implication.push_back (g->pos_lhs_ids[1].clause->id);
    return;
  }
  if (ite_flags_cond_lhs (flag)) {
    LOG ("degenerated case with lhs = cond");
    CADICAL_assert (g->pos_lhs_ids[0].clause);
    CADICAL_assert (g->pos_lhs_ids[3].clause);
    reasons_back.push_back (g->pos_lhs_ids[3].clause->id);
    reasons_implication.push_back (g->pos_lhs_ids[0].clause->id);
    return;
  }
  reasons_implication.push_back (g->pos_lhs_ids[0].clause->id);
  reasons_implication.push_back (g->pos_lhs_ids[2].clause->id);
  reasons_back.push_back (g->pos_lhs_ids[1].clause->id);
  reasons_back.push_back (g->pos_lhs_ids[3].clause->id);
}

void Closure::rewrite_ite_gate_update_lrat_reasons (Gate *g, int src,
                                                    int dst) {
  if (!internal->lrat)
    return;
  LOG (g, "updating lrat from");
  for (auto &litId : g->pos_lhs_ids) {
    CADICAL_assert (litId.clause);
    if (litId.current_lit == src)
      litId.current_lit = dst;
    if (litId.current_lit == -src)
      litId.current_lit = -dst;
  }
  check_ite_lrat_reasons (g, false);
}

bool Closure::rewrite_ite_gate_to_and (
    Gate *g, int src, int dst, size_t idx1, size_t idx2,
    int cond_lit_to_learn_if_degenerated) {
  CADICAL_assert (internal->lrat_chain.empty ());
  CADICAL_assert (!g->garbage);
  LOG (g, "rewriting to proper AND");
  if (internal->val (g->lhs) > 0) {
    {
      const int lit = g->rhs[0];
      const char v = internal->val (lit);
      if (v > 0) {
      } else if (!v) {
        if (internal->lrat) {
          push_id_and_rewriting_lrat_unit (g->pos_lhs_ids[idx1].clause,
                                           Rewrite (), lrat_chain);
        }
        learn_congruence_unit (cond_lit_to_learn_if_degenerated);
      } else {
        if (internal->lrat)
          push_id_and_rewriting_lrat_unit (g->pos_lhs_ids[idx1].clause,
                                           Rewrite (), lrat_chain);
        push_lrat_unit (-lit);
        internal->learn_empty_clause ();
        return true;
      }
    }
    if (!internal->unsat) {
      const int lit = g->rhs[1];
      const char v = internal->val (lit);
      CADICAL_assert (dst == g->rhs[0] || dst == g->rhs[1] || -dst == g->rhs[0] ||
              -dst == g->rhs[1]);
      const int other = (dst == g->rhs[0] || dst == g->rhs[1])
                            ? dst ^ g->rhs[0] ^ g->rhs[1]
                            : (-dst) ^ g->rhs[0] ^ g->rhs[1];
      if (v > 0) {
        // already set by propagation
        return true;
      } else if (!v) {
        if (internal->lrat) {
          push_id_and_rewriting_lrat_unit (g->pos_lhs_ids[idx2].clause,
                                           Rewrite (), lrat_chain);
        }
        learn_congruence_unit (other);
      } else {
        if (internal->lrat) {
          push_lrat_unit (cond_lit_to_learn_if_degenerated);
          push_id_and_rewriting_lrat_unit (g->pos_lhs_ids[idx2].clause,
                                           Rewrite (), lrat_chain);
        }
        internal->learn_empty_clause ();
        return true;
      }
    }
    return true;
  }
  if (!internal->lrat)
    return false;
  LOG ("updating flags");
  g->degenerated_and_neg = (g->rhs[1] == -g->lhs || g->rhs[0] == -g->lhs);
  g->degenerated_and_pos = (g->rhs[0] == g->lhs || g->rhs[1] == g->lhs);
  CADICAL_assert (g->rhs.size () == 3);
  CADICAL_assert (g->pos_lhs_ids.size () == 4);
  CADICAL_assert (idx1 < g->pos_lhs_ids.size ());
  CADICAL_assert (idx2 < g->pos_lhs_ids.size ());
  int lit = g->pos_lhs_ids[idx2].current_lit, other = g->lhs;
  // TODO: remove argument
  (void) src;
  produce_rewritten_clause_lrat_and_clean (g->pos_lhs_ids, g->lhs, idx1,
                                           idx2, false);

  if ((idx1 == (size_t) -1 || idx2 == (size_t) -1)) {
    // degenerated and gate
    return false;
  }

  CADICAL_assert (idx1 != (size_t) -1);
  CADICAL_assert (idx2 != (size_t) -1);
  CADICAL_assert (idx1 < g->pos_lhs_ids.size ());
  CADICAL_assert (idx2 < g->pos_lhs_ids.size ());
  Clause *c = g->pos_lhs_ids[idx1].clause;
  CADICAL_assert (c->size == 2);
  Clause *d = g->pos_lhs_ids[idx2].clause;
  CADICAL_assert (c != d);
  CADICAL_assert (c);
  CADICAL_assert (d);
  g->pos_lhs_ids.erase (std::remove_if (begin (g->pos_lhs_ids),
                                        end (g->pos_lhs_ids),
                                        [d] (const LitClausePair &p) {
                                          return p.clause == d || !p.clause;
                                        }),
                        end (g->pos_lhs_ids));
  CADICAL_assert (g->pos_lhs_ids.size () == 2);
  CADICAL_assert (lit);
  CADICAL_assert (other);
  CADICAL_assert (lit != dst);
  CADICAL_assert (other != dst);
  CADICAL_assert (lit != other);
  lrat_chain.push_back (c->id);
  lrat_chain.push_back (d->id);
  Clause *e = learn_binary_tmp_or_full_clause (lit, -other);
  CADICAL_assert (e);

  auto long_clause =
      std::find_if (begin (g->pos_lhs_ids), end (g->pos_lhs_ids),
                    [] (LitClausePair l) { return l.clause->size == 3; });
  CADICAL_assert (long_clause != end (g->pos_lhs_ids));
  LOG (long_clause->clause, "new long clause");
  g->neg_lhs_ids.push_back (*long_clause);
  g->pos_lhs_ids.erase (long_clause);

  CADICAL_assert (g->pos_lhs_ids.size () == 1);

  (void) maybe_promote_tmp_binary_clause (g->pos_lhs_ids[0].clause);
  g->pos_lhs_ids.push_back ({lit, e});
#ifndef CADICAL_NDEBUG
  for (auto litId : g->pos_lhs_ids) {
    bool found = false;
    CADICAL_assert (litId.clause);
    for (auto other : *litId.clause) {
      found = (find_eager_representative (other) == litId.current_lit);
      if (found)
        break;
    }
    CADICAL_assert (found);
  }
  for (auto id : g->pos_lhs_ids) {
    LOG (id.clause, "clause after rewriting:");
    CADICAL_assert (id.clause->size == 2);
  }

#endif
  return false;
}

void Closure::produce_ite_merge_lhs_then_else_reasons (
    Gate *g, std::vector<LRAT_ID> &reasons_implication,
    std::vector<LRAT_ID> &reasons_back, std::vector<LRAT_ID> &reasons_unit,
    bool rewritting_then, bool &learn_units) {

  const size_t idx1 = rewritting_then ? 0 : 2;
  const size_t idx2 = idx1 + 1;
  const size_t other_idx1 = rewritting_then ? 2 : 0;
  const size_t other_idx2 = other_idx1 + 1;
  const int cond_lit = g->rhs[0];
  const int lit_to_merge = g->rhs[rewritting_then ? 2 : 1];
  const int other_lit = g->rhs[rewritting_then ? 1 : 2];
  const int repr_cond_lit = find_eager_representative (g->rhs[0]);
  const int repr_lit_to_merge = find_eager_representative (lit_to_merge);
  const int repr_other_lit = find_eager_representative (other_lit);
  const int repr_lhs = find_eager_representative(g->lhs);
  if (!internal->proof)
    return;


  LOG ("cond: %d, merging %d and rewriting to %d", cond_lit, lit_to_merge,
       other_lit);
  if (internal->lrat) {
    CADICAL_assert (internal->lrat);
    CADICAL_assert (g->pos_lhs_ids.size () == 4);

    if (repr_lhs == -repr_other_lit) {
      LOG ("special case: %s=%s, checking if other: %s %s", LOGLIT (g->lhs),
           LOGLIT (-lit_to_merge), LOGLIT (cond_lit), LOGLIT (other_lit));
      CADICAL_assert (repr_lit_to_merge != -repr_lhs); // should have been rewritten before

      if (rewritting_then && repr_cond_lit == repr_lhs) {
        LOG ("t=-lhs/c=lhs");
        learn_units = true;
        // is a unit
        push_id_and_rewriting_lrat_unit (g->pos_lhs_ids[0].clause,
                                         Rewrite (), lrat_chain);
        unsimplified.push_back (-cond_lit);
        LRAT_ID id_unit = simplify_and_add_to_proof_chain (unsimplified);
        reasons_unit = {id_unit};
        // don't bother finding out which one is used
        reasons_implication.push_back (id_unit);
        g->pos_lhs_ids[3].clause = produce_rewritten_clause_lrat (
            g->pos_lhs_ids[3].clause, g->lhs, false);
        CADICAL_assert (g->pos_lhs_ids[3].clause);
        reasons_implication.push_back (g->pos_lhs_ids[3].clause->id);
        unsimplified.clear ();
        return;
      }
      if (!rewritting_then && repr_cond_lit == repr_lhs) {
        LOG ("e=-lhs/c=lhs");
        learn_units = true;
        // is a unit
        push_id_and_rewriting_lrat_unit (g->pos_lhs_ids[3].clause,
                                         Rewrite (), lrat_chain);
        unsimplified.push_back (cond_lit);
        LRAT_ID id_unit = simplify_and_add_to_proof_chain (unsimplified);
        reasons_unit = {id_unit};
        // don't bother finding out which one is used
        reasons_implication.push_back (id_unit);
        g->pos_lhs_ids[0].clause = produce_rewritten_clause_lrat (
            g->pos_lhs_ids[0].clause, g->lhs, false);
        CADICAL_assert (g->pos_lhs_ids[0].clause);
        reasons_implication.push_back (g->pos_lhs_ids[0].clause->id);
        unsimplified.clear ();
        return;
      }
      if (!rewritting_then && repr_cond_lit == -repr_lhs) {
        LOG ("e=-lhs/c=-lhs");
        learn_units = true;
        // TODO: this function does not work to produce units for this case
        // c LOG 0 rewriting 4 by 3 in gate[42] (arity: 3) -3 := ITE 3 7
        // ...
        // c LOG 0 clause[50] 4 -3
        // c LOG 0 clause[44] 5 3
        // c LOG 0 clause[2] -3 -4 -5
        // the first two are rewriting, but they are not ordered properly
        // and we need the '5' clause to come after
        push_id_and_rewriting_lrat_unit (g->pos_lhs_ids[2].clause,
                                         Rewrite (), lrat_chain);
        unsimplified.push_back (cond_lit);
        LRAT_ID id_unit = simplify_and_add_to_proof_chain (unsimplified);
        reasons_unit = {id_unit};
        g->pos_lhs_ids[1].clause = produce_rewritten_clause_lrat (
            g->pos_lhs_ids[1].clause, g->lhs, false);
        CADICAL_assert (g->pos_lhs_ids[1].clause);

        // don't bother finding out which one is used
        reasons_implication.push_back (id_unit);
        reasons_implication.push_back (g->pos_lhs_ids[1].clause->id);
        unsimplified.clear ();
        return;
      }
      if (rewritting_then && repr_cond_lit == -repr_lhs) {
        LOG ("t=-lhs/c=-lhs");
        learn_units = true;
        push_id_and_rewriting_lrat_unit (g->pos_lhs_ids[1].clause,
                                         Rewrite (), lrat_chain);
        unsimplified.push_back (-cond_lit);
        LRAT_ID id_unit = simplify_and_add_to_proof_chain (unsimplified);
        reasons_unit = {id_unit};
        g->pos_lhs_ids[2].clause = produce_rewritten_clause_lrat (
            g->pos_lhs_ids[2].clause, g->lhs, false);
        CADICAL_assert (g->pos_lhs_ids[2].clause);

        reasons_implication.push_back (id_unit);
        reasons_implication.push_back (g->pos_lhs_ids[2].clause->id);
        unsimplified.clear ();
        return;
      }
      if (rewritting_then && repr_lit_to_merge == repr_lhs) {
        LOG ("t=-lhs/e=lhs from rewriting then");
        learn_units = true;
        g->pos_lhs_ids[idx1].clause = produce_rewritten_clause_lrat (
            g->pos_lhs_ids[idx1].clause, g->lhs, false);
        g->pos_lhs_ids[idx2].clause = produce_rewritten_clause_lrat (
            g->pos_lhs_ids[idx2].clause, g->lhs, false);
        CADICAL_assert (g->pos_lhs_ids[idx1].clause);
        CADICAL_assert (g->pos_lhs_ids[idx2].clause);
        lrat_chain.push_back (g->pos_lhs_ids[idx1].clause->id);
        lrat_chain.push_back (g->pos_lhs_ids[idx2].clause->id);
        unsimplified.push_back (-cond_lit);
        LRAT_ID id_unit = simplify_and_add_to_proof_chain (unsimplified);
        reasons_unit = {id_unit};
        unsimplified.clear ();

        return;
      }
      if (!rewritting_then && repr_lit_to_merge == repr_lhs) {
        LOG ("t=-lhs/e=lhs from rewriting else");
        learn_units = true;
        g->pos_lhs_ids[idx1].clause = produce_rewritten_clause_lrat (
            g->pos_lhs_ids[idx1].clause, g->lhs, false);
        g->pos_lhs_ids[idx2].clause = produce_rewritten_clause_lrat (
            g->pos_lhs_ids[idx2].clause, g->lhs, false);
        CADICAL_assert (g->pos_lhs_ids[idx1].clause);
        CADICAL_assert (g->pos_lhs_ids[idx2].clause);
        lrat_chain.push_back (g->pos_lhs_ids[idx1].clause->id);
        lrat_chain.push_back (g->pos_lhs_ids[idx2].clause->id);
        unsimplified.push_back (cond_lit);
        LRAT_ID id_unit = simplify_and_add_to_proof_chain (unsimplified);
        reasons_unit = {id_unit};
        unsimplified.clear ();
        return;
      }

      if (other_lit == repr_lhs) {
        LOG ("TODO FIX ME t=-lhs/e=lhs");
        learn_units = true;
        // in the other direction we are merging a literal with itself
        g->pos_lhs_ids[idx1].clause = produce_rewritten_clause_lrat (
            g->pos_lhs_ids[idx1].clause, g->lhs, false);
        g->pos_lhs_ids[idx2].clause = produce_rewritten_clause_lrat (
            g->pos_lhs_ids[idx2].clause, g->lhs, false);
        CADICAL_assert (g->pos_lhs_ids[idx1].clause);
        CADICAL_assert (g->pos_lhs_ids[idx2].clause);
        reasons_unit.push_back (g->pos_lhs_ids[idx1].clause->id);
        reasons_unit.push_back (g->pos_lhs_ids[idx2].clause->id);
        return;
      }
      // if (other_lit == -g->lhs) {
      //   CADICAL_assert (false);
      // }
      // if (other_lit == g->lhs) {
      //   CADICAL_assert (false);
      // }
    }

    LOG ("normal path");
    produce_rewritten_clause_lrat (g->pos_lhs_ids, g->lhs, false);
    CADICAL_assert (g->pos_lhs_ids.size () == 4);

    reasons_unit.push_back (g->pos_lhs_ids[idx1].clause->id);
    reasons_unit.push_back (g->pos_lhs_ids[idx2].clause->id);

    // already merged: only unit is important
    if (!rewritting_then && repr_lhs == repr_lit_to_merge) {
      return;
    }

    reasons_implication.push_back (g->pos_lhs_ids[other_idx1].clause->id);
    reasons_implication.push_back (g->pos_lhs_ids[idx1].clause->id);
    reasons_implication.push_back (g->pos_lhs_ids[idx2].clause->id);

    reasons_back.push_back (g->pos_lhs_ids[other_idx2].clause->id);
    reasons_back.push_back (g->pos_lhs_ids[idx1].clause->id);
    reasons_back.push_back (g->pos_lhs_ids[idx2].clause->id);
  } else {
    LOG ("learn extra clauses XXXXXXXXXXXXXXXXXXXXXXXXX");
    const int lhs = g->lhs;
    const int cond = g->rhs[0];
    if (rewritting_then) {
      unsimplified.push_back (-cond);
      unsimplified.push_back (lhs);
      simplify_and_add_to_proof_chain (unsimplified);
      unsimplified.push_back (-cond);
      unsimplified.push_back (-lhs);
      simplify_and_add_to_proof_chain (unsimplified);
    } else {
      unsimplified.push_back (cond);
      unsimplified.push_back (-lhs);
      simplify_and_add_to_proof_chain (unsimplified);
      unsimplified.push_back (cond);
      unsimplified.push_back (lhs);
      simplify_and_add_to_proof_chain (unsimplified);
    }
    unsimplified.clear ();
  }
}

void Closure::rewrite_ite_gate (Gate *g, int dst, int src) {
  CADICAL_assert (unsimplified.empty ());
  if (skip_ite_gate (g))
    return;
  if (!gate_contains (g, src))
    return;
  LOG (g, "rewriting %d by %d in", src, dst);
  CADICAL_assert (!g->shrunken);
  CADICAL_assert (g->rhs.size () == 3);
  CADICAL_assert (!internal->lrat || g->pos_lhs_ids.size () == 4);
  auto &rhs = g->rhs;
  const int lhs = g->lhs;
  const int cond = g->rhs[0];
  const int then_lit = g->rhs[1];
  const int else_lit = g->rhs[2];
  const int not_lhs = -(lhs);
  const int not_dst = -(dst);
  const int not_cond = -(cond);
  const int not_then_lit = -(then_lit);
  const int not_else_lit = -(else_lit);
  Gate_Type new_tag = Gate_Type::And_Gate;

  bool garbage = false;
  bool shrink = true;
  const auto git = g->indexed ? table.find (g) : end (table);
  CADICAL_assert (!g->indexed || git != end (table));
  CADICAL_assert (*git == g);
  if (internal->val (cond) && internal->val (then_lit) &&
      internal->val (else_lit)) { // propagation has set all value anyway
    LOG (g, "all values are set");
    CADICAL_assert (internal->val (g->lhs));
    garbage = true;
  } else if (internal->val (g->lhs) && internal->val (cond)) {
    LOG (g, "all values are set 2");
    CADICAL_assert (internal->val (g->lhs));
    garbage = true;
  }
  // this code is taken one-to-one from kissat
  else if (src == cond) {
    if (dst == then_lit) {
      // then_lit ? then_lit : else_lit
      // then_lit & then_lit | !then_lit & else_lit
      // then_lit | !then_lit & else_lit
      // then_lit | else_lit
      // !(!then_lit & !else_lit)
      g->lhs = not_lhs;
      rhs[0] = not_then_lit;
      rhs[1] = not_else_lit;
      if (then_lit == lhs || else_lit == lhs)
        garbage = true;
      else
        garbage = rewrite_ite_gate_to_and (g, src, dst, 1, 3, -dst);
    } else if (not_dst == then_lit) {
      // !then_lit ? then_lit : else_lit
      // !then_lit & then_lit | then_lit & else_lit
      // then_lit & else_lit
      rhs[0] = else_lit;
      CADICAL_assert (rhs[1] == then_lit);
      garbage = rewrite_ite_gate_to_and (g, src, dst, 0, 2, -dst);
    } else if (dst == else_lit) {
      // else_list ? then_lit : else_lit
      // else_list & then_lit | !else_list & else_lit
      // else_list & then_lit
      rhs[0] = else_lit;
      CADICAL_assert (rhs[1] == then_lit);
      garbage = rewrite_ite_gate_to_and (g, src, dst, 2, 0, dst);
    } else if (not_dst == else_lit) {
      // !else_list ? then_lit : else_lit
      // !else_list & then_lit | else_lit & else_lit
      // !else_list & then_lit | else_lit
      // then_lit | else_lit
      // !(!then_lit & !else_lit)
      g->lhs = not_lhs;
      rhs[0] = not_then_lit;
      rhs[1] = not_else_lit;
      if (then_lit == lhs || else_lit == lhs)
        garbage = true;
      else
        garbage = rewrite_ite_gate_to_and (g, src, dst, 3, 1, dst);
    } else {
      shrink = false;
      rhs[0] = dst;
      rewrite_ite_gate_update_lrat_reasons (g, src, dst);
    }
  } else if (src == then_lit) {
    if (not_dst == g->lhs) { // TODO not in kissat
      rhs[1] = dst;
      check_ite_implied (g->lhs, cond, then_lit, else_lit);
      std::vector<LRAT_ID> reasons_implication, reasons_back, reasons_unit;
      LOG ("%d = %d ?", g->lhs, -g->rhs[0]);
      bool learn_units_instead_of_equivalence = false;
      produce_ite_merge_lhs_then_else_reasons (
          g, reasons_implication, reasons_back, reasons_unit, true,
          learn_units_instead_of_equivalence);
      if (learn_units_instead_of_equivalence) { // it is too hard to produce
                                                // LRAT chains
                                                // in this case

        if (internal->lrat)
          lrat_chain = reasons_unit;
        learn_congruence_unit (-cond, true);
        if (-else_lit == lhs) {
          if (internal->lrat)
            lrat_chain = reasons_implication;
          learn_congruence_unit (cond == -lhs ? -else_lit : else_lit, false, true);
        } else fully_propagate ();
      } else {
        if (merge_literals_lrat (g->lhs, else_lit, reasons_implication,
                                 reasons_back)) {
          ++internal->stats.congruence.unaries;
          ++internal->stats.congruence.unary_ites;
        }
        if (!internal->unsat) {
          if (internal->lrat)
            lrat_chain = reasons_unit;
          learn_congruence_unit (-cond);
        }
      }
      delete_proof_chain ();
      garbage = true;
    } else if (dst == cond) {
      // cond ? cond : else_lit
      // cond & cond | !cond & else_lit
      // cond | !cond & else_lit
      // cond | else_lit
      // !(!cond & !else_lit)
      g->lhs = not_lhs;
      rhs[0] = not_cond;
      rhs[1] = not_else_lit;
      if (else_lit == lhs || cond == lhs)
        garbage = true;
      else
        garbage = rewrite_ite_gate_to_and (g, src, dst, 1, 3, -cond);
    } else if (not_dst == cond) {
      // cond ? !cond : else_lit
      // cond & !cond | !cond & else_lit
      // !cond & else_lit
      rhs[0] = not_cond;
      rhs[1] = else_lit;
      garbage = rewrite_ite_gate_to_and (g, src, dst, 0, 2, -cond);
    } else if (dst == else_lit) {
      // cond ? else_lit : else_lit
      // else_lit
      std::vector<LRAT_ID> reasons_implication, reasons_back;
      produce_ite_merge_then_else_reasons (g, src, dst, reasons_implication,
                                           reasons_back);
      if (merge_literals_lrat (lhs, else_lit, reasons_implication,
                               reasons_back)) {
        ++internal->stats.congruence.unaries;
        ++internal->stats.congruence.unary_ites;
      }
      delete_proof_chain ();
      garbage = true;
    } else if (not_dst == else_lit) {
      // cond ? !else_lit : else_lit
      // cond & !else_lit | !cond & else_lit
      // cond ^ else_lit
      if (g->lhs == cond) {
        produce_rewritten_clause_lrat_and_clean (g->pos_lhs_ids, g->lhs,
                                                 false);
        if (internal->lrat) {
          CADICAL_assert (g->pos_lhs_ids.size () == 2);
          lrat_chain.push_back (g->pos_lhs_ids[0].clause->id);
          lrat_chain.push_back (g->pos_lhs_ids[1].clause->id);
        }
        learn_congruence_unit (-else_lit);
        garbage = true;
      } else {
        LOG ("changing to xor");
        new_tag = Gate_Type::XOr_Gate;
        CADICAL_assert (rhs[0] == cond);
        rhs[1] = else_lit;
        CADICAL_assert (!internal->lrat || !g->pos_lhs_ids.empty ());
        {
#ifdef LOGGING
          for (auto litId : g->pos_lhs_ids) {
            LOG (litId.clause, "%d ->", litId.current_lit);
          }
#endif
          produce_rewritten_clause_lrat_and_clean (g->pos_lhs_ids, g->lhs,
                                                   true);
#ifdef LOGGING
          for (auto litId : g->pos_lhs_ids) {
            LOG (litId.clause, "%d ->", litId.current_lit);
          }
#endif
        }
      }
    } else {
      shrink = false;
      rhs[1] = dst;
      rewrite_ite_gate_update_lrat_reasons (g, src, dst);
    }
  } else {
    CADICAL_assert (src == else_lit);
    if (not_dst == g->lhs) { // TODO not in kissat
      rhs[2] = dst;
      std::vector<LRAT_ID> reasons_implication, reasons_back, reasons_unit;
      bool learn_units_instead_of_equivalence = false;
      produce_ite_merge_lhs_then_else_reasons (
          g, reasons_implication, reasons_back, reasons_unit, false,
          learn_units_instead_of_equivalence);
      if (learn_units_instead_of_equivalence) { // Too hard to produce LRAT
        if (internal->lrat)
          lrat_chain = reasons_unit;
        learn_congruence_unit (cond, true);
        if (then_lit != lhs) {
	  LOG ("special case, learning %d",cond == -lhs ? -then_lit : then_lit);
          if (internal->lrat)
            lrat_chain = reasons_implication;
          learn_congruence_unit (cond == -lhs ? -then_lit : then_lit, false, true);
        } else fully_propagate ();
      } else {
        if (merge_literals_lrat (lhs, then_lit, reasons_implication,
                                 reasons_back)) {
          ++internal->stats.congruence.unaries;
          ++internal->stats.congruence.unary_ites;
        }
        if (!internal->unsat) {
          if (internal->lrat)
            lrat_chain = reasons_unit;
          learn_congruence_unit (cond);
        }
      }
      delete_proof_chain ();
      garbage = true;
    } else if (dst == cond) {
      // cond ? then_lit : cond
      // cond & then_lit | !cond & cond
      // cond & then_lit
      CADICAL_assert (rhs[0] == cond);
      CADICAL_assert (rhs[1] == then_lit);
      garbage = rewrite_ite_gate_to_and (g, src, dst, 2, 0, cond);
    } else if (not_dst == cond) {
      // cond ? then_lit : !cond
      // cond & then_lit | !cond & !cond
      // cond & then_lit | !cond
      // then_lit | !cond
      // !(!then_lit & cond)
      g->lhs = not_lhs;
      CADICAL_assert (rhs[0] == cond);
      rhs[1] = not_then_lit;
      if (then_lit == lhs || cond == lhs)
        garbage = true;
      else
        garbage = rewrite_ite_gate_to_and (g, src, dst, 3, 1, cond);
    } else if (dst == then_lit) {
      // cond ? then_lit : then_lit
      // then_lit
      std::vector<LRAT_ID> reasons_implication, reasons_back;
      produce_ite_merge_then_else_reasons (g, src, dst, reasons_implication,
                                           reasons_back);
      if (merge_literals_lrat (lhs, then_lit, reasons_implication,
                               reasons_back)) {
        ++internal->stats.congruence.unaries;
        ++internal->stats.congruence.unary_ites;
      }
      garbage = true;
    } else if (not_dst == then_lit) {
      // cond ? then_lit : !then_lit
      // cond & then_lit | !cond & !then_lit
      // !(cond ^ then_lit)
      if (lhs == cond) {
        produce_rewritten_clause_lrat_and_clean (g->pos_lhs_ids, not_lhs,
                                                 false);
        if (internal->lrat) {
          CADICAL_assert (g->pos_lhs_ids.size () == 2);
          lrat_chain.push_back (g->pos_lhs_ids[0].clause->id);
          lrat_chain.push_back (g->pos_lhs_ids[1].clause->id);
        }
        learn_congruence_unit (then_lit);
        garbage = true;
      } else if (not_lhs == cond) {
        produce_rewritten_clause_lrat_and_clean (g->pos_lhs_ids, not_lhs,
                                                 false);
        if (internal->lrat) {
          CADICAL_assert (g->pos_lhs_ids.size () == 2);
          lrat_chain.push_back (g->pos_lhs_ids[0].clause->id);
          lrat_chain.push_back (g->pos_lhs_ids[1].clause->id);
        }
        learn_congruence_unit (-then_lit);
        garbage = true;
      } else if (not_lhs == then_lit) {
        produce_rewritten_clause_lrat_and_clean (g->pos_lhs_ids, not_lhs,
                                                 false);
        if (internal->lrat) {
          CADICAL_assert (g->pos_lhs_ids.size () == 2);
          lrat_chain.push_back (g->pos_lhs_ids[0].clause->id);
          lrat_chain.push_back (g->pos_lhs_ids[1].clause->id);
        }
        learn_congruence_unit (cond);
        garbage = true;
      } else {
        new_tag = Gate_Type::XOr_Gate;
        g->lhs = not_lhs;
        CADICAL_assert (rhs[0] == cond);
        CADICAL_assert (rhs[1] == then_lit);
        CADICAL_assert (rhs[0] != g->lhs);
        CADICAL_assert (rhs[1] != g->lhs);
        produce_rewritten_clause_lrat_and_clean (g->pos_lhs_ids, not_lhs,
                                                 false);
      }
    } else {
      shrink = false;
      rhs[2] = dst;
      rewrite_ite_gate_update_lrat_reasons (g, src, dst);
    }
  }
  if (!garbage && !internal->unsat) {
    CADICAL_assert (new_tag != Gate_Type::ITE_Gate || g->lhs != -rhs[1]);
    CADICAL_assert (new_tag != Gate_Type::ITE_Gate || g->lhs != -rhs[2]);
    if (shrink) {
      if (new_tag == Gate_Type::XOr_Gate) {
        bool negate_lhs = false;
        if (rhs[0] < 0) {
          rhs[0] = -rhs[0];
          negate_lhs = !negate_lhs;
        }
        if (rhs[1] < 0) {
          rhs[1] = -rhs[1];
          negate_lhs = !negate_lhs;
        }
        if (negate_lhs)
          g->lhs = -g->lhs;
      }
      if (internal->vlit (rhs[0]) >
          internal->vlit (
              rhs[1])) { // unlike kissat, we need to do it after negating
        std::swap (rhs[0], rhs[1]);
        CADICAL_assert (new_tag != Gate_Type::ITE_Gate);
      }
      CADICAL_assert (internal->vlit (rhs[0]) < internal->vlit (rhs[1]));
      CADICAL_assert (!g->shrunken);
      g->shrunken = true;
      rhs[2] = 0;
      g->tag = new_tag;
      rhs.resize (2);
      CADICAL_assert (rhs[0] != -rhs[1]);
      if (new_tag == Gate_Type::XOr_Gate) {
        if (rhs[0] == -g->lhs || rhs[1] == -g->lhs) {
          LOG (g, "special XOR:");
          const int unit = rhs[0] ^ -g->lhs ^ rhs[1];
          produce_rewritten_clause_lrat_and_clean (g->pos_lhs_ids, not_lhs,
                                                   false);
          if (internal->lrat) {
            CADICAL_assert (g->pos_lhs_ids.size () == 2);
            lrat_chain.push_back (g->pos_lhs_ids[0].clause->id);
            lrat_chain.push_back (g->pos_lhs_ids[1].clause->id);
          }
          learn_congruence_unit (unit);
          garbage = true;
        } else if (rhs[0] == g->lhs || rhs[1] == g->lhs) {
          LOG (g, "special XOR:");
          const int unit = rhs[0] ^ g->lhs ^ rhs[1];
          produce_rewritten_clause_lrat_and_clean (g->pos_lhs_ids, not_lhs,
                                                   false);
          if (internal->lrat) {
            CADICAL_assert (g->pos_lhs_ids.size () == 2);
            lrat_chain.push_back (g->pos_lhs_ids[0].clause->id);
            lrat_chain.push_back (g->pos_lhs_ids[1].clause->id);
          }
          learn_congruence_unit (-unit);
          garbage = true;
        } else {
          int i = 0;
          bool negated = false;
          for (int j = 0; j < 2; ++i, ++j) {
            CADICAL_assert (i <= j);
            const int lit = rhs[i] = rhs[j];
            const char v = internal->val (lit);
            if (v > 0) {
              --i;
              negated = !negated;
            } else if (v < 0) {
              --i;
            }
          }
          CADICAL_assert (i <= 2);
          rhs.resize (i);
          if (negated) {
            g->lhs = -g->lhs;
          }
          if (i != 2) {
            LOG (g, "removed units");
          }
          if (!i)
            garbage = true;
          else if (i == 1) {
#ifdef LOGGING
            for (auto litId : g->pos_lhs_ids) {
              LOG (litId.clause, "%d ->", litId.current_lit);
            }
#endif
            produce_rewritten_clause_lrat_and_clean (g->pos_lhs_ids, g->lhs);
            CADICAL_assert (!internal->lrat || g->pos_lhs_ids.size () == 2);
            Clause *c1 = nullptr, *c2 = nullptr;
            if (internal->lrat) {
              CADICAL_assert (g->pos_lhs_ids[0].clause);
              bool rhs_as_src_first =
                  g->pos_lhs_ids[0].clause->literals[0] == g->lhs ||
                  g->pos_lhs_ids[0].clause->literals[1] == g->lhs;
              c1 = (rhs_as_src_first ? g->pos_lhs_ids[0].clause
                                     : g->pos_lhs_ids[1].clause);
              c2 = (rhs_as_src_first ? g->pos_lhs_ids[1].clause
                                     : g->pos_lhs_ids[0].clause);
              c1 = maybe_promote_tmp_binary_clause (c1);
              c2 = maybe_promote_tmp_binary_clause (c2);
            } else {
              maybe_add_binary_clause (-g->lhs, g->rhs[0]);
              maybe_add_binary_clause (g->lhs, -g->rhs[0]);
            }
            merge_literals_equivalence (g->lhs, g->rhs[0], c1, c2);
            garbage = true;
          }
        }
        if (!garbage) {
          CADICAL_assert (rhs[0] != g->lhs);
          CADICAL_assert (rhs[1] != g->lhs);
          CADICAL_assert (rhs[0] != -g->lhs);
          CADICAL_assert (rhs[1] != -g->lhs);
        }
      }

      if (!garbage) {
        g->hash = hash_lits (nonces, g->rhs);
        LOG (g, "rewritten");

        if (internal->lrat) {
          if (new_tag == Gate_Type::XOr_Gate) {
#ifndef CADICAL_NDEBUG
            std::for_each (begin (g->pos_lhs_ids), end (g->pos_lhs_ids),
                           [g] (LitClausePair l) {
                             CADICAL_assert ((size_t) l.clause->size ==
                                     1 + g->arity ());
                           });
#endif
          } else if (new_tag == Gate_Type::And_Gate) {
            // we have to get rid of one clause, two become binaries, and
            // becomes ternary

#if defined(LOGGING) || !defined(CADICAL_NDEBUG)
            for (auto id : g->pos_lhs_ids) {
              LOG (id.clause, "clause after rewriting:");
              CADICAL_assert (id.clause->size == 2);
            }
#endif
            CADICAL_assert (g->pos_lhs_ids.size () == 2 ||
                    gate_contains (g, g->lhs));
            CADICAL_assert (g->neg_lhs_ids.size () == 1 ||
                    gate_contains (g, g->lhs));
            CADICAL_assert (g->arity () == 2);
#ifndef CADICAL_NDEBUG
            std::for_each (
                begin (g->pos_lhs_ids), end (g->pos_lhs_ids),
                [] (LitClausePair l) { CADICAL_assert (l.clause->size == 2); });
#endif
          } else {
            CADICAL_assert (false);
#if defined(WIN32) && !defined(__MINGW32__)
            __assume(false);
#else
            __builtin_unreachable ();
#endif
          }
        }
        Gate *h;
        if (new_tag == Gate_Type::And_Gate) {
          check_and_gate_implied (g);
          h = find_and_lits (rhs);
        } else {
          CADICAL_assert (new_tag == Gate_Type::XOr_Gate);
          check_xor_gate_implied (g);
          h = find_xor_gate (g);
        }
        if (h) {
          garbage = true;
          if (new_tag == Gate_Type::XOr_Gate) {
            std::vector<LRAT_ID> reasons_implication, reasons_back;
            add_xor_matching_proof_chain (g, g->lhs, h->pos_lhs_ids, h->lhs,
                                          reasons_implication,
                                          reasons_back);
            if (merge_literals_lrat (g->lhs, h->lhs, reasons_implication,
                                     reasons_back))
              ++internal->stats.congruence.xors;
          } else {
            add_ite_turned_and_binary_clauses (g);
            std::vector<LRAT_ID> reasons_implication, reasons_back;
            if (internal->lrat)
              merge_and_gate_lrat_produce_lrat (g, h, reasons_implication,
                                                reasons_back, false);
            if (merge_literals_lrat (g->lhs, h->lhs, reasons_implication,
                                     reasons_back))
              ++internal->stats.congruence.ands;
          }
          delete_proof_chain ();
        } else {
          garbage = false;
          if (g->indexed)
            remove_gate (git);
          index_gate (g);
          CADICAL_assert (g->arity () == 2);
          for (auto lit : g->rhs)
            if (lit != dst)
              if (lit != cond && lit != then_lit && lit != else_lit)
                connect_goccs (g, lit);
          if (g->tag == Gate_Type::And_Gate && !internal->lrat)
            for (auto lit : g->rhs)
              maybe_add_binary_clause (-g->lhs, lit);
        }
      }
    } else {
      LOG (g, "rewritten");
      if (internal->lrat)
	update_ite_flags (g), check_correct_ite_flags(g);
      CADICAL_assert (rhs[0] != rhs[1]);
      CADICAL_assert (rhs[0] != rhs[2]);
      CADICAL_assert (rhs[1] != rhs[2]);
      CADICAL_assert (rhs[0] != -(rhs[1]));
      CADICAL_assert (rhs[0] != -(rhs[2]));
      CADICAL_assert (rhs[1] != -(rhs[2]));
      check_ite_gate_implied (g);
      check_ite_lrat_reasons (g, false);
      bool negate_lhs;
      Gate *h = find_ite_gate (g, negate_lhs);
      CADICAL_assert (lhs == g->lhs);
      CADICAL_assert (not_lhs == -(g->lhs));
      if (negate_lhs)
        g->lhs = -lhs;
      check_ite_lrat_reasons (g);
      if (internal->lrat)
	check_correct_ite_flags (g);
      if (h) {
        garbage = true;
        check_ite_gate_implied (g);
        check_ite_lrat_reasons (g, false);
        check_ite_gate_implied (h);
        check_ite_lrat_reasons (h, false);
        int normalized_lhs = negate_lhs ? not_lhs : lhs;
        std::vector<LRAT_ID> extra_reasons_lit, extra_reasons_ulit;
        add_ite_matching_proof_chain (g, h, normalized_lhs, h->lhs,
                                      extra_reasons_lit,
                                      extra_reasons_ulit);
        if (merge_literals_lrat (normalized_lhs, h->lhs, extra_reasons_lit,
                                 extra_reasons_ulit))
          ++internal->stats.congruence.ites;
        delete_proof_chain ();
        CADICAL_assert (internal->unsat || chain.empty ());
      } else {
        garbage = false;
        if (g->indexed)
          remove_gate (git);
        LOG (g, "normalized");
        g->hash = hash_lits (nonces, g->rhs);
        index_gate (g);
        CADICAL_assert (g->arity () == 3);
        for (auto lit : g->rhs)
          if (lit != dst)
            if (lit != cond && lit != then_lit && lit != else_lit)
              connect_goccs (g, lit);
      }
    }
  }
  if (garbage && !internal->unsat)
    mark_garbage (g);

  CADICAL_assert (chain.empty ());
}

void Closure::simplify_ite_gate_produce_unit_lrat (Gate *g, int lit,
                                                   size_t idx1,
                                                   size_t idx2) {
  if (!internal->lrat)
    return;
  // TODO
  if (internal->val (lit) > 0)
    return;
  CADICAL_assert (internal->lrat);
  CADICAL_assert (g);
  CADICAL_assert (idx1 < g->pos_lhs_ids.size ());
  CADICAL_assert (idx2 < g->pos_lhs_ids.size ());
  CADICAL_assert (g->pos_lhs_ids.size () == 4);

  CADICAL_assert (idx1 != idx2);
  Clause *c = g->pos_lhs_ids[idx1].clause;
  Clause *d = g->pos_lhs_ids[idx2].clause;

  if (g->lhs == -g->rhs[0]) {
    LOG ("special case of LHS=-cond where only one clause in LRAT is needed is needed");
    size_t idx = (internal->val (g->rhs[1]) < 0 ? idx2 : idx1);
    c = produce_rewritten_clause_lrat (g->pos_lhs_ids[idx].clause, g->lhs, false, false);
    CADICAL_assert (c);
    // not possible to do this in a single lrat chain
    push_id_and_rewriting_lrat_unit (c, Rewrite (), lrat_chain, true,
                                     Rewrite (), g->lhs);
    CADICAL_assert (d);
    return;
  }
  if (g->lhs == g->rhs[0]) {
    LOG ("special case of LHS=cond where only one clause in LRAT is needed is needed");
    size_t idx = (internal->val (g->rhs[1]) > 0 ? idx2 : idx1);
    c = produce_rewritten_clause_lrat (g->pos_lhs_ids[idx].clause, g->lhs, false, false);
    CADICAL_assert (c);
    // not possible to do this in a single lrat chain
    push_id_and_rewriting_lrat_unit (c, Rewrite (), lrat_chain, true,
                                     Rewrite (), g->lhs);
    CADICAL_assert (d);
    return;
  }

  c = produce_rewritten_clause_lrat (c, g->lhs, true);
  if (c) {
    lrat_chain.push_back (c->id);
    d = produce_rewritten_clause_lrat (d, g->lhs, true);
    if (d)
      lrat_chain.push_back (d->id);
  } else if (!c) {
    push_id_and_rewriting_lrat_unit (d, Rewrite (), lrat_chain, true,
                                     Rewrite (), g->lhs);
    CADICAL_assert (d);
    lrat_chain.push_back (d->id);
  }
}

// TODO merge
void Closure::merge_and_gate_lrat_produce_lrat (
    Gate *g, Gate *h, std::vector<LRAT_ID> &reasons_lrat_src,
    std::vector<LRAT_ID> &reasons_lrat_usrc, bool remove_units) {
  CADICAL_assert (internal->lrat);
  CADICAL_assert (g->tag == Gate_Type::And_Gate);
  CADICAL_assert (h->tag == Gate_Type::And_Gate);
  CADICAL_assert (g->neg_lhs_ids.size () <= 1);
  update_and_gate_build_lrat_chain (g, h, reasons_lrat_src,
                                    reasons_lrat_usrc, remove_units);
}

// odd copy of rewrite_ite_gate_lrat_and
bool Closure::simplify_ite_gate_to_and (Gate *g, size_t idx1, size_t idx2,
                                        int removed_lit) {
  CADICAL_assert (internal->lrat_chain.empty ());
  CADICAL_assert (g->rhs.size () == 3);
#ifdef LOGGING
  for (auto litId : g->pos_lhs_ids) {
    LOG (litId.clause, "%d ->", litId.current_lit);
  }
#endif
  if (g->lhs == -g->rhs[0] || g->lhs == -g->rhs[1]) {
    if (internal->lrat) {
      Clause *c = g->pos_lhs_ids[idx1].clause;
      push_id_and_rewriting_lrat_unit (c, Rewrite (), lrat_chain);
      CADICAL_assert (!lrat_chain.empty ());
    }
    learn_congruence_unit (-g->lhs);
    return true;
  }
  if (!internal->lrat)
    return false;
  g->degenerated_and_neg = (g->degenerated_and_neg || g->rhs[1] == -g->lhs || g->rhs[0] == -g->lhs);
  g->degenerated_and_pos = (g->degenerated_and_pos || g->rhs[0] == g->lhs || g->rhs[1] == g->lhs);

  CADICAL_assert (g->pos_lhs_ids.size () == 4);
  CADICAL_assert (idx1 < g->pos_lhs_ids.size ());
  CADICAL_assert (idx2 < g->pos_lhs_ids.size ());
  int lit = g->pos_lhs_ids[idx2].current_lit, other = g->lhs;
  size_t new_idx1 = idx1;
  size_t new_idx2 = idx2;
  produce_rewritten_clause_lrat_and_clean (g->pos_lhs_ids, g->lhs, new_idx1,
                                           new_idx2);

  if (g->pos_lhs_ids.size () == 1) {
    LOG (g, "degenerated AND gate");
    const int replacement_lit = (g->rhs[1] == g->lhs ? g->rhs[0] : g->rhs[1]);
    for (auto &litId : g->pos_lhs_ids) {
      CADICAL_assert (litId.clause);
      LOG (litId.clause, "%d ->", litId.current_lit);
      if (litId.current_lit == removed_lit)
        litId.current_lit = -replacement_lit;
      if (litId.current_lit == -removed_lit)
        litId.current_lit = replacement_lit;
      LOG (litId.clause, "%d ->", litId.current_lit);
      // TODO we need a replacement index
      CADICAL_assert (std::find (begin (*litId.clause), end (*litId.clause), litId.current_lit) !=
              end (*litId.clause));
      CADICAL_assert (std::find (begin (g->rhs), end (g->rhs), litId.current_lit) !=
              end (g->rhs));
    }
    return false;
  }
  CADICAL_assert (new_idx1 < g->pos_lhs_ids.size ());
  CADICAL_assert (new_idx2 < g->pos_lhs_ids.size ());
  Clause *c = g->pos_lhs_ids[new_idx1].clause;
  CADICAL_assert (c->size == 2);
  CADICAL_assert (new_idx1 != new_idx2);
  Clause *d = g->pos_lhs_ids[new_idx2].clause;
  CADICAL_assert (c != d);
  CADICAL_assert (c);
  CADICAL_assert (d);
  g->pos_lhs_ids.erase (std::remove_if (begin (g->pos_lhs_ids),
                                        end (g->pos_lhs_ids),
                                        [d] (const LitClausePair &p) {
                                          return p.clause == d;
                                        }),
                        end (g->pos_lhs_ids));
  CADICAL_assert (g->pos_lhs_ids.size () == 2);
  CADICAL_assert (lit);
  CADICAL_assert (other);
  CADICAL_assert (lit != other);
  lrat_chain.push_back (c->id);
  lrat_chain.push_back (d->id);
  Clause *e = add_tmp_binary_clause (lit, -other);

  auto long_clause =
      std::find_if (begin (g->pos_lhs_ids), end (g->pos_lhs_ids),
                    [] (LitClausePair l) { return l.clause->size == 3; });
  CADICAL_assert (long_clause != end (g->pos_lhs_ids));
  LOG (long_clause->clause, "new long clause");
  g->neg_lhs_ids.push_back (*long_clause);
  g->pos_lhs_ids.erase (long_clause);
  CADICAL_assert (g->pos_lhs_ids.size () == 1);
  g->pos_lhs_ids.push_back ({lit, e});

  const int first_lit = g->rhs[0];
  const int second_lit = g->rhs[1];
  for (auto &litId : g->pos_lhs_ids) {
    CADICAL_assert (litId.clause);
    LOG (litId.clause, "%s ->", LOGLIT (litId.current_lit));
    if (internal->val (litId.current_lit)) {
      CADICAL_assert (litId.clause->size == 2);
      int replacement_lit = 0;
      for (int i = 0; i < 2; ++i) {
        if (litId.clause->literals[i] == first_lit) {
          replacement_lit = first_lit;
          break;
        } else if (litId.clause->literals[i] == second_lit) {
          replacement_lit = second_lit;
          break;
        }
      }
      CADICAL_assert (replacement_lit);

      litId.current_lit = replacement_lit;
    } else if (litId.current_lit == removed_lit)
      litId.current_lit = -g->rhs[0];
    else if (litId.current_lit == -removed_lit)
      litId.current_lit = g->rhs[0];
    LOG (litId.clause, "%d ->", litId.current_lit);
    // TODO we need a replacement index
    CADICAL_assert (std::find (begin (g->rhs), end (g->rhs), litId.current_lit) !=
            end (g->rhs));
    CADICAL_assert (std::find (begin (*litId.clause), end (*litId.clause),
                       litId.current_lit) != end (*litId.clause));
  }
  return false;
}

void Closure::merge_ite_gate_same_then_else_lrat (
    std::vector<LitClausePair> &clauses,
    std::vector<LRAT_ID> &reasons_implication,
    std::vector<LRAT_ID> &reasons_back) {
  CADICAL_assert (clauses.size () == 4);
  produce_rewritten_clause_lrat_and_clean (clauses);
  CADICAL_assert (clauses.size () == 4);
  auto then_imp = clauses[0];
  auto neg_then_imp = clauses[1];
  auto else_imp = clauses[2];
  auto neg_else_imp = clauses[3];
  reasons_implication.push_back (then_imp.clause->id);
  reasons_implication.push_back (else_imp.clause->id);
  reasons_back.push_back (neg_then_imp.clause->id);
  reasons_back.push_back (neg_else_imp.clause->id);
}

void Closure::simplify_ite_gate_then_else_set (
    Gate *g, std::vector<LRAT_ID> &reasons_implication,
    std::vector<LRAT_ID> &reasons_back, size_t idx1, size_t idx2) {
  CADICAL_assert (idx1 < g->pos_lhs_ids.size ());
  CADICAL_assert (idx2 < g->pos_lhs_ids.size ());
  Clause *c = g->pos_lhs_ids[idx1].clause;
  Clause *d = g->pos_lhs_ids[idx2].clause;
  push_id_and_rewriting_lrat_unit (c, Rewrite (), reasons_back, true,
                                   Rewrite (), g->lhs);
  push_id_and_rewriting_lrat_unit (d, Rewrite (), reasons_implication, true,
                                   Rewrite (), g->lhs);
  LOG (reasons_back, "LRAT");
  LOG (reasons_implication, "LRAT");
  // c = produce_rewritten_clause_lrat (c, g->lhs, false);
  // CADICAL_assert (c);
  // d = produce_rewritten_clause_lrat (d, g->lhs, false);
  // CADICAL_assert (d);
  // const int cond = g->rhs[0];
  // CADICAL_assert (internal->val (cond));
  // reasons_implication.push_back(internal->unit_id (internal->val (cond) >
  // 0 ? cond : -cond)); reasons_implication.push_back(c->id);
  // reasons_back.push_back(internal->unit_id (internal->val (cond) > 0 ?
  // cond : -cond)); reasons_back.push_back(d->id);
}

void Closure::simplify_ite_gate_condition_set (
    Gate *g, std::vector<LRAT_ID> &reasons_lrat,
    std::vector<LRAT_ID> &reasons_back_lrat, size_t idx1, size_t idx2) {
  CADICAL_assert (internal->lrat);
  Clause *c = g->pos_lhs_ids[idx1].clause;
  Clause *d = g->pos_lhs_ids[idx2].clause;
#if defined(LOGGING) || !defined(CADICAL_NDEBUG)
  const int cond = g->rhs[0];
  CADICAL_assert (internal->val (cond));
  LOG ("cond = %d", cond);
#endif
#ifdef LOGGING
  for (auto litid : g->pos_lhs_ids)
    LOG (litid.clause, "clause in gate:");
#endif
  push_id_and_rewriting_lrat_unit (c, Rewrite (), reasons_lrat, true,
                                   Rewrite (), -g->lhs);
  push_id_and_rewriting_lrat_unit (d, Rewrite (), reasons_back_lrat, true,
                                   Rewrite (), g->lhs);
}

void Closure::simplify_ite_gate (Gate *g) {
  if (skip_ite_gate (g))
    return;
  LOG (g, "simplifying");
  CADICAL_assert (g->arity () == 3);
  bool garbage = true;
  int lhs = g->lhs;
  auto &rhs = g->rhs;
  const int cond = rhs[0];
  const int then_lit = rhs[1];
  const int else_lit = rhs[2];
  const signed char v_cond = internal->val (cond);
  const signed char v_else = internal->val (else_lit);
  const signed char v_then = internal->val (then_lit);
  std::vector<LRAT_ID> reasons_lrat, reasons_back_lrat;
  if (v_cond && v_else && v_then) { // propagation has set all value anyway
    LOG (g, "all values are set");
    CADICAL_assert (internal->val (g->lhs));
    garbage = true;
  } else if (v_cond > 0) {
    if (internal->lrat)
      simplify_ite_gate_condition_set (g, reasons_lrat, reasons_back_lrat,
                                       0, 1);
    if (merge_literals_lrat (lhs, then_lit, reasons_lrat,
                             reasons_back_lrat)) {
      ++internal->stats.congruence.unary_ites;
      ++internal->stats.congruence.unaries;
    }
  } else if (v_cond < 0) {
    if (internal->lrat)
      simplify_ite_gate_condition_set (g, reasons_lrat, reasons_back_lrat,
                                       2, 3);
    if (merge_literals_lrat (lhs, else_lit, reasons_lrat,
                             reasons_back_lrat)) {
      ++internal->stats.congruence.unary_ites;
      ++internal->stats.congruence.unaries;
    }
  } else {
    LOG ("then %d: %d; else %d: %d", then_lit, v_then, else_lit, v_else);
    std::vector<LRAT_ID> extra_reasons, extra_reasons_back;
    CADICAL_assert (v_then || v_else);
    if (v_then > 0 && v_else > 0) {
      simplify_ite_gate_produce_unit_lrat (g, lhs, 1, 3);
      learn_congruence_unit (lhs);
    } else if (v_then < 0 && v_else < 0) {
      simplify_ite_gate_produce_unit_lrat (g, -lhs, 0, 2);
      learn_congruence_unit (-lhs);
    } else if (v_then > 0 && v_else < 0) {
      if (internal->lrat)
        simplify_ite_gate_then_else_set (g, extra_reasons,
                                         extra_reasons_back, 1, 2);
      if (merge_literals_lrat (lhs, cond, extra_reasons,
                               extra_reasons_back)) {
        ++internal->stats.congruence.unary_ites;
        ++internal->stats.congruence.unaries;
      }
    } else if (v_then < 0 && v_else > 0) {
      if (internal->lrat)
        simplify_ite_gate_then_else_set (g, extra_reasons,
                                         extra_reasons_back, 0, 3);
      if (merge_literals_lrat (lhs, -cond, extra_reasons_back,
                               extra_reasons)) {
        ++internal->stats.congruence.unary_ites;
        ++internal->stats.congruence.unaries;
      }
    } else {
      CADICAL_assert (!!v_then + !!v_else == 1);
      auto git = g->indexed ? table.find (g) : end (table);
      CADICAL_assert (!g->indexed || git != end (table));
      if (v_then > 0) {
        g->lhs = -lhs;
        rhs[0] = -cond;
        rhs[1] = -else_lit;
        simplify_ite_gate_to_and (g, 1, 3, then_lit);
      } else if (v_then < 0) {
        rhs[0] = -cond;
        rhs[1] = else_lit;
        simplify_ite_gate_to_and (g, 0, 2, -then_lit);
      } else if (v_else > 0) {
        g->lhs = -lhs;
        rhs[0] = -then_lit;
        rhs[1] = cond;
        simplify_ite_gate_to_and (g, 3, 1, else_lit);
      } else {
        CADICAL_assert (v_else < 0);
        rhs[0] = then_lit;
        rhs[1] = cond;
        simplify_ite_gate_to_and (g, 2, 0, -else_lit);
      }
      if (internal->unsat)
        return;
      if (internal->vlit (rhs[0]) > internal->vlit (rhs[1]))
        std::swap (rhs[0], rhs[1]);
      g->shrunken = true;
      g->tag = Gate_Type::And_Gate;
      rhs.resize (2);
      CADICAL_assert (is_sorted (begin (rhs), end (rhs),
                         sort_literals_by_var_smaller (internal)));
      g->hash = hash_lits (nonces, rhs);
      check_and_gate_implied (g);
      Gate *h = find_and_lits (rhs);
      if (h) {
        CADICAL_assert (garbage);
        std::vector<LRAT_ID> reasons_lrat, reasons_lrat_back;
        if (internal->lrat)
          merge_and_gate_lrat_produce_lrat (g, h, reasons_lrat,
                                            reasons_lrat_back, false);
        if (merge_literals_lrat (g->lhs, h->lhs, reasons_lrat,
                                 reasons_lrat_back)) {
          ++internal->stats.congruence.ites;
        }
      } else {
        remove_gate (git);
        index_gate (g);
        garbage = false;
        g->hash = hash_lits (nonces, g->rhs);
        for (auto lit : rhs)
          if (lit != cond && lit != then_lit && lit != else_lit) {
            connect_goccs (g, lit);
          }

        if (rhs[0] == -g->lhs || rhs[1] == -g->lhs)
          simplify_and_gate (
              g); // TODO Kissat does not do that, but it has also no
                  // checks to verify that it cannot happen...
      }
    }
  }
  if (garbage && !internal->unsat)
    mark_garbage (g);
  ++internal->stats.congruence.simplified;
  ++internal->stats.congruence.simplified_ites;
}

void Closure::add_ite_matching_proof_chain (
    Gate *g, Gate *h, int lhs1, int lhs2, std::vector<LRAT_ID> &reasons1,
    std::vector<LRAT_ID> &reasons2) {
  check_ite_lrat_reasons (g);
  check_ite_lrat_reasons (h, false);
  CADICAL_assert (g->lhs == lhs1);
  CADICAL_assert (h->lhs == lhs2);
  if (lhs1 == lhs2)
    return;
  if (!internal->proof)
    return;
  LOG (g, "starting ITE matching proof chain");
  LOG (h, "starting ITE matching proof chain with");
  CADICAL_assert (unsimplified.empty ());
  CADICAL_assert (chain.empty ());
  if (internal->lrat)
    check_correct_ite_flags (g);
  const auto &rhs = g->rhs;
  const int8_t flags_g = g->degenerated_ite;
  const int8_t flags_h = h->degenerated_ite;
  const int cond = rhs[0];
  LRAT_ID g_then_id = 0, g_neg_then_id = 0, g_neg_else_id = 0;
  LRAT_ID h_then_id = 0, h_neg_then_id = 0, h_else_id = 0;
  LRAT_ID g_else_id = 0, h_neg_else_id = 0;
  const bool degenerated_g_then = ite_flags_no_then_clauses (flags_g);
  const bool degenerated_g_else = ite_flags_no_else_clauses (flags_g);
  const bool degenerated_h_then = ite_flags_no_then_clauses (flags_h);
  const bool degenerated_h_else = ite_flags_no_else_clauses (flags_h);

  const bool degenerated_g_cond = ite_flags_cond_lhs (flags_g);
  const bool degenerated_h_cond = ite_flags_cond_lhs (flags_h);
  CADICAL_assert (!(degenerated_g_cond && degenerated_h_cond));

  const bool degenerated_g_not_cond = ite_flags_neg_cond_lhs (flags_g);
  const bool degenerated_h_not_cond = ite_flags_neg_cond_lhs (flags_h);
  CADICAL_assert (!(degenerated_g_not_cond && degenerated_h_not_cond));

  if (internal->lrat) {
    // the code can produce tautologies in the case that:
    // a = (c ? a : e)
    // b = (c ? a : e)
    // (no clauses with (then) index a/-a)
    // but also for
    // a = (a ? t : e)
    // b = (a ? t : e)
    // (no clauses with (then) index -a and (else) index e)
    // and same for
    // a = (c ? t : a)
    // b = (c ? t : a)
    // (no clauses with (then) index a and (else) index -e)
    LOG (g, "get ids from");
    CADICAL_assert (g->pos_lhs_ids.size () == 4);
    auto &g_then_clause = g->pos_lhs_ids[0].clause;
    g_then_clause =
      g_then_clause ? produce_rewritten_clause_lrat (g_then_clause, g->lhs, false) : nullptr;
    if (g_then_clause)
      g_then_id = g_then_clause->id;

    auto &g_neg_then_clause = g->pos_lhs_ids[1].clause;
    g_neg_then_clause =
      g_neg_then_clause ? produce_rewritten_clause_lrat (g_neg_then_clause, g->lhs, false) : nullptr;
    if (g_neg_then_clause)
      g_neg_then_id = g_neg_then_clause->id;

    auto &g_else_clause = g->pos_lhs_ids[2].clause;
    g_else_clause =
      g_else_clause ? produce_rewritten_clause_lrat (g_else_clause, g->lhs, false) : g_else_clause;
    if (g_else_clause)
      g_else_id = g_else_clause->id;

    auto &g_neg_else_clause = g->pos_lhs_ids[3].clause;
    g_neg_else_clause =
      g_neg_else_clause ? produce_rewritten_clause_lrat (g_neg_else_clause, g->lhs, false) : nullptr;
    if (g_neg_else_clause)
      g_neg_else_id = g_neg_else_clause->id;

    LOG (h, "now clauses from");
    CADICAL_assert (h->pos_lhs_ids.size () == 4);
    auto &h_then_clause = h->pos_lhs_ids[0].clause;
    h_then_clause =
      h_then_clause ? produce_rewritten_clause_lrat (h_then_clause, h->lhs, false) : nullptr;
    if (h_then_clause)
      h_then_id = h_then_clause->id;

    auto &h_neg_then_clause = h->pos_lhs_ids[1].clause;
    h_neg_then_clause =
      h_neg_then_clause ? produce_rewritten_clause_lrat (h_neg_then_clause, h->lhs, false) : nullptr;
    if (h_neg_then_clause)
      h_neg_then_id = h_neg_then_clause->id;

    auto &h_else_clause = h->pos_lhs_ids[2].clause;
    h_else_clause =
      h_else_clause ? produce_rewritten_clause_lrat (h_else_clause, h->lhs, false) : nullptr;
    if (h_else_clause)
      h_else_id = h_else_clause->id;

    auto &h_neg_else_clause = h->pos_lhs_ids[3].clause;
    h_neg_else_clause =
      h_neg_else_clause ? produce_rewritten_clause_lrat (h_neg_else_clause, h->lhs, false) : nullptr;
    if (h_neg_else_clause)
      h_neg_else_id = h_neg_else_clause->id;

  }

  if (degenerated_g_cond) {
    LOG ("special case: cond = lhs, g degenerated");
    unsimplified.push_back (-lhs1);
    unsimplified.push_back (lhs2);
    LRAT_ID id1 = 0;
    if (internal->lrat) {
      lrat_chain.push_back (g_then_id);
      lrat_chain.push_back (h_neg_then_id);
    }
    id1 = simplify_and_add_to_proof_chain (unsimplified);

    unsimplified.clear ();
    unsimplified.push_back (lhs1);
    unsimplified.push_back (-lhs2);
    LRAT_ID id2 = 0;
    if (internal->lrat) {
      lrat_chain.push_back (g_neg_else_id);
      lrat_chain.push_back (h_else_id);
    }
    id2 = simplify_and_add_to_proof_chain (unsimplified);

    CADICAL_assert (!internal->lrat || id1);
    CADICAL_assert (!internal->lrat || id2);
    reasons1.push_back (id1);
    reasons2.push_back (id2);
    unsimplified.clear ();
    return;
  }

  if (degenerated_h_cond) {
    LOG ("special case: cond = lhs, h degenerated");
    unsimplified.push_back (lhs1); // potentially lhs1 == lhs2
    unsimplified.push_back (-lhs2);
    LRAT_ID id1 = 0;
    if (internal->lrat) {
      lrat_chain.push_back (h_then_id);
      lrat_chain.push_back (g_neg_then_id);
    }
    id1 = simplify_and_add_to_proof_chain (unsimplified);

    unsimplified.clear ();
    unsimplified.push_back (-lhs1);
    unsimplified.push_back (lhs2);
    LRAT_ID id2 = 0;
    if (internal->lrat) {
      lrat_chain.push_back (h_neg_else_id);
      lrat_chain.push_back (g_else_id);
    }
    id2 = simplify_and_add_to_proof_chain (unsimplified);

    CADICAL_assert (!internal->lrat || id1);
    CADICAL_assert (!internal->lrat || id2);
    reasons2.push_back (id1);
    reasons1.push_back (id2);
    unsimplified.clear ();
    return;
  }

  if (degenerated_g_not_cond) {
    LOG ("special case: cond = -lhs, g degenerated");
    unsimplified.push_back (lhs1);
    unsimplified.push_back (-lhs2);
    LRAT_ID id1 = 0;
    if (internal->lrat) {
      CADICAL_assert (g_neg_then_id && h_then_id && g_else_id && h_neg_else_id);
      lrat_chain.push_back (g_neg_then_id);
      lrat_chain.push_back (h_then_id);
    }
    id1 = simplify_and_add_to_proof_chain (unsimplified);

    unsimplified.clear ();
    unsimplified.push_back (-lhs1);
    unsimplified.push_back (lhs2);
    LRAT_ID id2 = -1;

    if (internal->lrat) {
      lrat_chain.push_back (g_else_id);
      lrat_chain.push_back (h_neg_else_id);
    }
    id2 = simplify_and_add_to_proof_chain (unsimplified);
    CADICAL_assert (!internal->lrat || id1);
    CADICAL_assert (!internal->lrat || id2);
    reasons2.push_back (id1);
    reasons1.push_back (id2);
    unsimplified.clear ();
    return;
  }

  if (degenerated_h_not_cond) {
    LOG ("special case: cond = -lhs, h degenerated");
    unsimplified.push_back (lhs1);
    unsimplified.push_back (-lhs2);
    LRAT_ID id1 = 0;
    if (internal->lrat) {
      CADICAL_assert (g_neg_then_id && h_then_id && g_else_id && h_neg_else_id);
      lrat_chain.push_back (h_neg_then_id);
      lrat_chain.push_back (g_then_id);
    }
    id1 = simplify_and_add_to_proof_chain (unsimplified);

    unsimplified.clear ();
    unsimplified.push_back (-lhs1);
    unsimplified.push_back (lhs2);
    LRAT_ID id2 = -1;

    if (internal->lrat) {
      lrat_chain.push_back (h_else_id);
      lrat_chain.push_back (g_neg_else_id);
    }
    id2 = simplify_and_add_to_proof_chain (unsimplified);
    CADICAL_assert (!internal->lrat || id1);
    CADICAL_assert (!internal->lrat || id2);
    reasons2.push_back (id1);
    reasons1.push_back (id2);
    unsimplified.clear ();
    return;
  }
  
  LOG ("normal path");
  CADICAL_assert (!internal->lrat || degenerated_g_then ||
          (g_then_id && g_neg_then_id));
  CADICAL_assert (!internal->lrat || degenerated_g_else ||
          (g_else_id && g_neg_else_id));
  CADICAL_assert (!internal->lrat || degenerated_h_then ||
          (h_then_id && h_neg_then_id));
  CADICAL_assert (!internal->lrat || degenerated_h_else ||
          (h_else_id && h_neg_else_id));
  CADICAL_assert (!internal->lrat || g_then_id || h_neg_then_id);
  CADICAL_assert (!internal->lrat || g_neg_then_id || h_then_id);
  unsimplified.push_back (-lhs1);
  unsimplified.push_back (lhs2);
  unsimplified.push_back (-cond);
  LRAT_ID id1 = 0;
  if (degenerated_g_then || degenerated_h_then) {
    id1 = degenerated_g_then ? h_neg_then_id : g_then_id;
  } else {
    if (internal->lrat) {
      lrat_chain.push_back (g_then_id);
      lrat_chain.push_back (h_neg_then_id);
    }
    id1 = simplify_and_add_to_proof_chain (unsimplified);
  }
  unsimplified.pop_back ();
  unsimplified.push_back (cond);

  LRAT_ID id2 = 0;
  if (degenerated_g_else || degenerated_h_else) {
    id2 = degenerated_g_else ? h_neg_else_id : g_else_id;
  } else {
    if (internal->lrat) {
      lrat_chain.push_back (h_neg_else_id);
      lrat_chain.push_back (g_else_id);
    }
    id2 = simplify_and_add_to_proof_chain (unsimplified);
  }
  unsimplified.pop_back ();

  unsimplified.clear ();
  unsimplified.push_back (lhs1);
  unsimplified.push_back (-lhs2);
  unsimplified.push_back (-cond);
  CADICAL_assert (lrat_chain.empty ());

  LRAT_ID id3 = 0;
  if (degenerated_g_then || degenerated_h_then) {
    id3 = degenerated_g_then ? h_then_id : g_neg_then_id;
  } else {
    if (internal->lrat) {
      // lrat_chain.push_back (g_then_id);
      // lrat_chain.push_back (h_neg_then_id);
      lrat_chain.push_back (g_neg_then_id);
      lrat_chain.push_back (h_then_id);
    }
    id3 = simplify_and_add_to_proof_chain (unsimplified);
  }
  unsimplified.pop_back ();
  unsimplified.push_back (cond);
  CADICAL_assert (lrat_chain.empty ());

  LRAT_ID id4 = 0;
  if (degenerated_g_else || degenerated_h_else) {
    id4 = degenerated_g_else ? h_else_id : g_neg_else_id;
  } else {
    if (internal->lrat) {
      lrat_chain.push_back (g_neg_else_id);
      lrat_chain.push_back (h_else_id);
    }
    id4 = simplify_and_add_to_proof_chain (unsimplified);
  }
  unsimplified.pop_back ();

  if (internal->lrat) {
    CADICAL_assert (!internal->lrat || (id1 && id2 && id3 && id4));
    reasons1.push_back (id1), reasons1.push_back (id2);
    reasons2.push_back (id3), reasons2.push_back (id4);
  }

  unsimplified.clear ();

  LOG ("finished ITE matching proof chain");
}

Gate *Closure::new_ite_gate (int lhs, int cond, int then_lit, int else_lit,
                             std::vector<LitClausePair> &&clauses) {
  CADICAL_assert (chain.empty ());
  if (else_lit == -then_lit) {
    if (then_lit < 0)
      LOG ("skipping ternary XOR %d := %d ^ %d", lhs, cond, -then_lit);
    else
      LOG ("skipping ternary XOR %d := %d ^ %d", -lhs, cond, then_lit);
    return nullptr;
  }
  if (else_lit == then_lit) {
    LOG ("found trivial ITE gate %d := %d ? %d : %d", (lhs), (cond),
         (then_lit), (else_lit));
    std::vector<LRAT_ID> reasons_implication, reasons_back;
    if (internal->lrat) {
      merge_ite_gate_same_then_else_lrat (clauses, reasons_implication,
                                   reasons_back);
    }
    if (merge_literals_lrat (lhs, then_lit, reasons_implication,
                             reasons_back))
      ++internal->stats.congruence.trivial_ite;
    return 0;
  }

  rhs.clear ();
  rhs.push_back (cond);
  rhs.push_back (then_lit);
  rhs.push_back (else_lit);
  LOG ("ITE gate %d = %d ? %d : %d", lhs, cond, then_lit, else_lit);

  bool negate_lhs = false;
  Gate *g = new Gate;
  g->lhs = lhs;
  g->tag = Gate_Type::ITE_Gate;
  g->rhs = {rhs};
  g->pos_lhs_ids = clauses;
#ifdef LOGGING
  g->id = -1;
#endif
  Gate *h = find_ite_gate (g, negate_lhs);
  if (negate_lhs)
    lhs = -lhs;
  g->lhs = lhs;
  check_ite_gate_implied (g);
  check_ite_lrat_reasons (
      g, false); // due to merges done before during AND gate detection!

  if (h) {
    check_ite_gate_implied (h);
    std::vector<LRAT_ID> extra_reasons_lit, extra_reasons_ulit;
    add_ite_matching_proof_chain (h, g, h->lhs, lhs, extra_reasons_lit,
                                  extra_reasons_ulit);
    if (merge_literals_lrat (h, g, h->lhs, lhs, extra_reasons_lit,
                             extra_reasons_ulit)) {
      ++internal->stats.congruence.ites;
      LOG ("found merged literals");
    }
    delete_proof_chain ();
    delete g;
    return h;
  } else {
    // do not sort clauses here obviously!
    // sort (begin (g->rhs), end (g->rhs));
    g->garbage = false;
    g->indexed = true;
    g->shrunken = false;
    g->hash = hash_lits (nonces, g->rhs);
    table.insert (g);
    ++internal->stats.congruence.gates;
#ifdef LOGGING
    g->id = fresh_id++;
#endif
    LOG (g, "creating new");
    if (internal->lrat)
      update_ite_flags (g);
    check_ite_gate_implied (g);
    for (auto lit : g->rhs) {
      connect_goccs (g, lit);
    }
  }
  check_ite_lrat_reasons (g);
  return g;
}

void check_ite_lits_normalized (const std::vector<int> &lits) {
  CADICAL_assert (lits[0] > 0);
  CADICAL_assert (lits[1] > 0);
  CADICAL_assert (lits[0] != lits[1]);
  CADICAL_assert (lits[0] != lits[2]);
  CADICAL_assert (lits[1] != lits[2]);
  CADICAL_assert (lits[0] != -lits[1]);
  CADICAL_assert (lits[0] != -lits[2]);
  CADICAL_assert (lits[1] != -lits[2]);
#ifdef CADICAL_NDEBUG
  (void) lits;
#endif
}

void Closure::check_ite_implied (int lhs, int cond, int then_lit,
                                 int else_lit) {
  if (internal->lrat)
    return;
  check_ternary (cond, -else_lit, lhs);
  check_ternary (cond, else_lit, -lhs);
  check_ternary (-cond, -then_lit, lhs);
  check_ternary (-cond, then_lit, -lhs);
}

void Closure::check_contained_module_rewriting (Clause *c, int lit,
                                                bool normalized,
                                                int except) {
#ifndef CADICAL_NDEBUG
  if (lit == except) // happens for degenerated cases
    except = 0;
  const int normalize_lit =
      (lit == except ? except : find_eager_representative (lit));
  CADICAL_assert (!normalized || lit == -except || normalize_lit == lit);
  bool found = false;
  LOG (c, "looking for (normalized) %d in ", lit);
  for (auto other : *c) {
    const int normalize_other =
        (other == except ? except : find_eager_representative (other));
    LOG ("%d -> %d ", other, normalize_other);
    CADICAL_assert (!normalized || other == -except || normalize_other == other);
    if (normalize_other == normalize_lit) {
      found = true;
      break;
    }
  }
  CADICAL_assert (found);
#else
  (void) c, (void) lit, (void) normalized, (void) except;
#endif
}

void Closure::check_ite_lrat_reasons (Gate *g, bool normalized) {
#ifndef CADICAL_NDEBUG
  CADICAL_assert (g->tag == Gate_Type::ITE_Gate);
  if (!internal->lrat)
    return;
  CADICAL_assert (g->rhs.size () == 3);
  CADICAL_assert (is_tautological_ite_gate (g) || g->pos_lhs_ids.size () == 4);
  CADICAL_assert (g->neg_lhs_ids.empty ());
  CADICAL_assert (g->pos_lhs_ids.size () == 4);
#else
  (void) g, (void) normalized;
#endif
}

void Closure::check_ite_gate_implied (Gate *g) {
  CADICAL_assert (g->tag == Gate_Type::ITE_Gate);
  if (internal->lrat)
    return;
  check_ite_implied (g->lhs, g->rhs[0], g->rhs[1], g->rhs[2]);
}

void Closure::init_ite_gate_extraction (
    std::vector<ClauseSize> &candidates) {
  std::vector<Clause *> ternary;
  glargecounts.resize (2 * internal->vsize, 0);
  for (auto c : internal->clauses) {
    if (c->garbage)
      continue;
    if (c->redundant)
      continue;
    if (c->size < 3)
      continue;
    unsigned size = 0;

    CADICAL_assert (!c->garbage);
    for (auto lit : *c) {
      const signed char v = internal->val (lit);
      if (v < 0)
        continue;
      if (v > 0) {
        LOG (c, "deleting as satisfied due to %d", lit);
        internal->mark_garbage (c);
        goto CONTINUE_COUNTING_NEXT_CLAUSE;
      }
      if (size == 3)
        goto CONTINUE_COUNTING_NEXT_CLAUSE;
      size++;
    }
    if (size < 3)
      continue;
    CADICAL_assert (size == 3);
    ternary.push_back (c);
    LOG (c, "counting original ITE gate base");
    for (auto lit : *c) {
      if (!internal->val (lit))
        ++largecount (lit);
    }
  CONTINUE_COUNTING_NEXT_CLAUSE:;
  }

  for (auto c : ternary) {
    CADICAL_assert (!c->garbage);
    CADICAL_assert (!c->redundant);
    unsigned positive = 0, negative = 0, twice = 0;
    for (auto lit : *c) {
      if (internal->val (lit))
        continue;
      const int count_not_lit = largecount (-lit);
      if (!count_not_lit)
        goto CONTINUE_WITH_NEXT_TERNARY_CLAUSE;
      const unsigned count_lit = largecount (lit);
      CADICAL_assert (count_lit);
      if (count_lit > 1 && count_not_lit > 1)
        ++twice;
      if (lit < 0)
        ++negative;
      else
        ++positive;
    }
    if (twice < 2)
      goto CONTINUE_WITH_NEXT_TERNARY_CLAUSE;
    CADICAL_assert (c->size != 2);
    for (auto lit : *c)
      internal->occs (lit).push_back (c);
    if (positive && negative)
      candidates.push_back (c);
  CONTINUE_WITH_NEXT_TERNARY_CLAUSE:;
  }

  ternary.clear ();
}

void Closure::reset_ite_gate_extraction () {
  condbin[0].clear ();
  condbin[1].clear ();
  condeq[0].clear ();
  condeq[1].clear ();
  glargecounts.clear ();
  internal->clear_occs ();
}

void Closure::copy_conditional_equivalences (int lit,
                                             lit_implications &condbin) {
  CADICAL_assert (condbin.empty ());
  for (auto c : internal->occs (lit)) {
    CADICAL_assert (c->size != 2);
    int first = 0, second = 0;
    for (auto other : *c) {
      if (internal->val (other))
        continue;
      if (other == lit)
        continue;
      if (!first)
        first = other;
      else {
        CADICAL_assert (!second);
        second = other;
      }
    }
    CADICAL_assert (first), CADICAL_assert (second);
    lit_implication p (first, second, c);

    if (internal->vlit (first) < internal->vlit (second)) {
      CADICAL_assert (p.first == first);
      CADICAL_assert (p.second == second);
    } else {
      CADICAL_assert (internal->vlit (second) < internal->vlit (first));
      p.swap ();
      CADICAL_assert (p.first == second);
      CADICAL_assert (p.second == first);
    }
    LOG (c, "literal %d condition binary clause %d %d", lit, first, second);
    condbin.push_back (p);
  }
}

bool less_litpair (lit_equivalence p, lit_equivalence q) {
  const int a = p.first;
  const int b = q.first;
  if (a < b)
    return true;
  if (b > a)
    return false;
  const int c = p.second;
  const int d = q.second;
  return (c < d);
}
struct litpair_rank {
  CaDiCaL::Internal *internal;
  litpair_rank (Internal *i) : internal (i) {}
  typedef uint64_t Type;
  Type operator() (const lit_implication &a) const {
    uint64_t lita = internal->vlit (a.first);
    uint64_t litb = internal->vlit (a.second);
    return (lita << 32) + litb;
  }
};

struct litpair_smaller {
  CaDiCaL::Internal *internal;
  litpair_smaller (Internal *i) : internal (i) {}
  bool operator() (const lit_implication &a,
                   const lit_implication &b) const {
    const auto s = litpair_rank (internal) (a);
    const auto t = litpair_rank (internal) (b);
    return s < t;
  }
};

lit_implications::const_iterator
Closure::find_lit_implication_second_literal (
    int lit, lit_implications::const_iterator begin,
    lit_implications::const_iterator end) {
  LOG ("searching for %d in", lit);
  for (auto it = begin; it != end; ++it)
    LOG ("%d [%d]", it->first, it->second);
  lit_implications::const_iterator found = std::lower_bound (
      begin, end, lit_implication{lit, lit},
      [] (const lit_implication &a, const lit_implication &b) {
        return a.second < b.second;
      });
#ifndef CADICAL_NDEBUG
  auto found2 = std::binary_search (
      begin, end, lit_implication{lit, lit},
      [] (const lit_implication &a, const lit_implication &b) {
        return a.second < b.second;
      });
#endif

  if (found < end && found->second == lit) {
    CADICAL_assert (found2 == (found < end));
    return found;
  }
  return end;
}

void Closure::search_condeq (int lit, int pos_lit,
                             lit_implications::const_iterator pos_begin,
                             lit_implications::const_iterator pos_end,
                             int neg_lit,
                             lit_implications::const_iterator neg_begin,
                             lit_implications::const_iterator neg_end,
                             lit_equivalences &condeq) {
  CADICAL_assert (neg_lit == -pos_lit);
  CADICAL_assert (pos_begin < pos_end);
  CADICAL_assert (neg_begin < neg_end);
  CADICAL_assert (pos_begin->first == pos_lit);
  CADICAL_assert (neg_begin->first == neg_lit);
  CADICAL_assert (pos_end <= neg_begin || neg_end <= pos_begin);
  for (lit_implications::const_iterator p = pos_begin; p != pos_end; p++) {
    const int other = p->second;
    const int not_other = -other;
    const lit_implications::const_iterator other_clause =
        find_lit_implication_second_literal (not_other, neg_begin, neg_end);
    CADICAL_assert (std::find (begin (*p->clause), end (*p->clause), lit) !=
            end (*p->clause));
    if (other_clause != neg_end) {
      CADICAL_assert (std::find (begin (*other_clause->clause),
                         end (*other_clause->clause),
                         neg_lit) != end (*other_clause->clause));
      CADICAL_assert (std::find (begin (*p->clause), end (*p->clause), other) !=
              end (*p->clause));
      lit_equivalence equivalence (neg_lit, other_clause->clause, other,
                                   p->clause);
      if (pos_lit > 0) {
        equivalence.negate_both ();
      }
      if (internal->lrat)
        equivalence.check_invariant ();
      LOG ("found conditional %d equivalence %d = %d", lit,
           equivalence.first, equivalence.second);
      CADICAL_assert (equivalence.first > 0);
      CADICAL_assert (internal->vlit (equivalence.first) <
              internal->vlit (equivalence.second));
      check_ternary (lit, neg_lit, -other);
      check_ternary (lit, -neg_lit, other);
      condeq.push_back (equivalence);
      if (equivalence.second < 0) {
        lit_equivalence inverse_equivalence =
            equivalence.swap ().negate_both ();
        condeq.push_back (inverse_equivalence);
        if (internal->lrat)
          inverse_equivalence.check_invariant ();
      } else {
        lit_equivalence inverse_equivalence = equivalence.swap ();
        condeq.push_back (inverse_equivalence);
        if (internal->lrat)
          inverse_equivalence.check_invariant ();
      }
    }
  }
#ifndef LOGGING
  (void) lit;
#endif
}

void Closure::extract_condeq_pairs (int lit, lit_implications &condbin,
                                    lit_equivalences &condeq) {
  const lit_implications::const_iterator begin = condbin.cbegin ();
  const lit_implications::const_iterator end = condbin.cend ();
  lit_implications::const_iterator pos_begin = begin;
  int next_lit = 0;

#ifdef LOGGING
  for (const auto &pair : condbin)
    LOG ("unsorted conditional %d equivalence %d = %d", lit, pair.first,
         pair.second);
#endif
  LOG ("searching for first positive literal for lit %d", lit);
  for (;;) {
    if (pos_begin == end)
      return;
    next_lit = pos_begin->first;
    LOG ("checking %d", next_lit);
    if (next_lit > 0)
      break;
    pos_begin++;
  }

  for (;;) {
    CADICAL_assert (pos_begin != end);
    CADICAL_assert (next_lit == pos_begin->first);
    CADICAL_assert (next_lit > 0);
    const int pos_lit = next_lit;
    lit_implications::const_iterator pos_end = pos_begin + 1;
    LOG ("searching for first other literal after finding lit %d",
         next_lit);
    for (;;) {
      if (pos_end == end)
        return;
      next_lit = pos_end->first;
      if (next_lit != pos_lit)
        break;
      pos_end++;
    }
    CADICAL_assert (pos_end != end);
    CADICAL_assert (next_lit == pos_end->first);
    const int neg_lit = -pos_lit;
    if (next_lit != neg_lit) {
      if (next_lit < 0) {
        pos_begin = pos_end + 1;
        LOG ("next_lit %d < 0", next_lit);
        for (;;) {
          if (pos_begin == end)
            return;
          next_lit = pos_begin->first;
          if (next_lit > 0)
            break;
          pos_begin++;
        }
      } else
        pos_begin = pos_end;
      continue;
    }
    const lit_implications::const_iterator neg_begin = pos_end;
    lit_implications::const_iterator neg_end = neg_begin + 1;
    while (neg_end != end) {
      next_lit = neg_end->first;
      if (next_lit != neg_lit)
        break;
      neg_end++;
    }
#ifdef LOGGING
    for (lit_implications::const_iterator p = pos_begin; p != pos_end; p++)
      LOG ("conditional %d binary clause %d %d with positive %d", (lit),
           (p->first), (p->second), (pos_lit));
    for (lit_implications::const_iterator p = neg_begin; p != neg_end; p++)
      LOG ("conditional %d binary clause %d %d with negative %d", (lit),
           (p->first), (p->second), (neg_lit));
#endif
    const size_t pos_size = pos_end - pos_begin;
    const size_t neg_size = neg_end - neg_begin;
    if (pos_size <= neg_size) {
      LOG ("searching negation of %zu conditional binary clauses "
           "with positive %d in %zu conditional binary clauses with %d",
           pos_size, (pos_lit), neg_size, (neg_lit));
      search_condeq (lit, pos_lit, pos_begin, pos_end, neg_lit, neg_begin,
                     neg_end, condeq);
    } else {
      LOG ("searching negation of %zu conditional binary clauses "
           "with negative %d in %zu conditional binary clauses with %d",
           neg_size, (neg_lit), pos_size, (pos_lit));
      search_condeq (lit, neg_lit, neg_begin, neg_end, pos_lit, pos_begin,
                     pos_end, condeq);
    }
    if (neg_end == end)
      return;
    CADICAL_assert (next_lit == neg_end->first);
    if (next_lit < 0) {
      pos_begin = neg_end + 1;
      for (;;) {
        if (pos_begin == end)
          return;
        next_lit = pos_begin->first;
        if (next_lit > 0)
          break;
        pos_begin++;
      }
    } else
      pos_begin = neg_end;
  }
}

void Closure::find_conditional_equivalences (int lit,
                                             lit_implications &condbin,
                                             lit_equivalences &condeq) {
  CADICAL_assert (condbin.empty ());
  CADICAL_assert (condeq.empty ());
  CADICAL_assert (internal->occs (lit).size () > 1);
  copy_conditional_equivalences (lit, condbin);
  MSORT (internal->opts.radixsortlim, begin (condbin), end (condbin),
         litpair_rank (this->internal), litpair_smaller (this->internal));

  extract_condeq_pairs (lit, condbin, condeq);
  MSORT (internal->opts.radixsortlim, begin (condbin), end (condbin),
         litpair_rank (this->internal), litpair_smaller (this->internal));

#ifdef LOGGING
  for (auto pair : condeq)
    LOG ("sorted conditional %d equivalence %d = %d", lit, pair.first,
         pair.second);
  LOG ("found %zu conditional %d equivalences", condeq.size (), lit);

#endif
}

void Closure::merge_condeq (int cond, lit_equivalences &condeq,
                            lit_equivalences &not_condeq) {
  LOG ("merging cond for literal %d", cond);
  auto q = begin (not_condeq);
  const auto end_not_condeq = end (not_condeq);
  for (auto p : condeq) {
    const int lhs = p.first;
    const int then_lit = p.second;
    if (internal->lrat)
      p.check_invariant ();
    CADICAL_assert (lhs > 0);
    while (q != end_not_condeq && q->first < lhs)
      ++q;
    while (q != end_not_condeq && q->first == lhs) {
      LOG ("looking when %d at p= %d %d", cond, p.first, p.second);
      LOG ("looking when %d at %d %d", -cond, q->first, q->second);
      lit_equivalence not_cond_pair = *q++;
      const int else_lit = not_cond_pair.second;
      std::vector<LitClausePair> clauses;
      if (internal->lrat) {
        // The then/else literal is the second of the pair, hence the swap
        // of the reasons
        CADICAL_assert (p.first_clause && p.second_clause);
        CADICAL_assert (not_cond_pair.first_clause && not_cond_pair.second_clause);
        LOG (p.second_clause, "pairing %d", then_lit);
        LOG (p.first_clause, "pairing %d", -then_lit);
        LOG (not_cond_pair.second_clause, "pairing %d", else_lit);
        LOG (not_cond_pair.first_clause, "pairing %d", -else_lit);
        clauses.push_back (LitClausePair (then_lit, p.second_clause));
        clauses.push_back (LitClausePair (-then_lit, p.first_clause));
        clauses.push_back (
            LitClausePair (else_lit, not_cond_pair.second_clause));
        clauses.push_back (
            LitClausePair (-else_lit, not_cond_pair.first_clause));
        if (internal->lrat)
          not_cond_pair.check_invariant ();
      }
      new_ite_gate (lhs, cond, then_lit, else_lit, std::move (clauses));
      if (internal->unsat)
        return;
    }
  }
}

void Closure::extract_ite_gates_of_literal (int lit) {
  LOG ("search for ITE for literal %d ", lit);
  find_conditional_equivalences (lit, condbin[0], condeq[0]);
  if (!condeq[0].empty ()) {
    find_conditional_equivalences (-lit, condbin[1], condeq[1]);
    if (!condeq[1].empty ()) {
      if (lit < 0)
        merge_condeq (-lit, condeq[0], condeq[1]);
      else
        merge_condeq (lit, condeq[1], condeq[0]);
    }
  }

  condbin[0].clear ();
  condbin[1].clear ();
  condeq[0].clear ();
  condeq[1].clear ();
}

void Closure::extract_ite_gates_of_variable (int idx) {
  const int lit = idx;
  const int not_lit = -idx;

  auto lit_watches = internal->occs (lit);
  auto not_lit_watches = internal->occs (not_lit);
  const size_t size_lit_watches = lit_watches.size ();
  const size_t size_not_lit_watches = not_lit_watches.size ();
  if (size_lit_watches <= size_not_lit_watches) {
    if (size_lit_watches > 1)
      extract_ite_gates_of_literal (lit);
  } else {
    if (size_not_lit_watches > 1)
      extract_ite_gates_of_literal (not_lit);
  }
}

void Closure::extract_ite_gates () {
  CADICAL_assert (!full_watching);
  if (!internal->opts.congruenceite)
    return;
  START (extractites);
  std::vector<ClauseSize> candidates;

  init_ite_gate_extraction (candidates);

  for (auto idx : internal->vars) {
    if (internal->flags (idx).active ()) {
      extract_ite_gates_of_variable (idx);
      if (internal->unsat)
        break;
    }
  }
  // Kissat has an alternative version MERGE_CONDITIONAL_EQUIVALENCES
  reset_ite_gate_extraction ();
  STOP (extractites);
}

/*------------------------------------------------------------------------*/
void Closure::extract_gates () {
  START (extract);
  extract_and_gates ();
  CADICAL_assert (internal->unsat || lrat_chain.empty ());
  CADICAL_assert (internal->unsat || chain.empty ());
  if (internal->unsat || internal->terminated_asynchronously ()) {
    STOP (extract);
    return;
  }
  extract_xor_gates ();
  CADICAL_assert (internal->unsat || lrat_chain.empty ());
  CADICAL_assert (internal->unsat || chain.empty ());

  if (internal->unsat || internal->terminated_asynchronously ()) {
    STOP (extract);
    return;
  }
  extract_ite_gates ();
  STOP (extract);
}

/*------------------------------------------------------------------------*/
// top level function to extract gate
bool Internal::extract_gates () {
  if (unsat)
    return false;
  if (!opts.congruence)
    return false;
  if (level)
    backtrack ();
  if (!propagate ()) {
    learn_empty_clause ();
    return false;
  }
  if (congruence_delay.bumpreasons.limit) {
    LOG ("delaying congruence %" PRId64 " more times",
         congruence_delay.bumpreasons.limit);
    congruence_delay.bumpreasons.limit--;
    return false;
  }

  // to remove false literals from clauses
  // It makes the technique stronger as long clauses
  // can become binary / ternary
  //  garbage_collection ();

  const int64_t old = stats.congruence.congruent;
  const int old_merged = stats.congruence.congruent;

  // congruencebinary is already doing it (and more actually)
  if (!internal->opts.congruencebinaries) {
    const bool dedup = opts.deduplicate;
    opts.deduplicate = true;
    mark_duplicated_binary_clauses_as_garbage ();
    opts.deduplicate = dedup;
  }
  ++stats.congruence.rounds;
  clear_watches ();
  //  connect_binary_watches ();

  START_SIMPLIFIER (congruence, CONGRUENCE);
  Closure closure (this);

  closure.init_closure ();
  CADICAL_assert (unsat || closure.chain.empty ());
  CADICAL_assert (unsat || lrat_chain.empty ());
  closure.extract_binaries ();
  CADICAL_assert (unsat || closure.chain.empty ());
  CADICAL_assert (unsat || lrat_chain.empty ());
  closure.extract_gates ();
  CADICAL_assert (unsat || closure.chain.empty ());
  CADICAL_assert (unsat || lrat_chain.empty ());
  internal->clear_watches ();
  internal->connect_watches ();
  closure.reset_extraction ();

  if (!unsat) {
    closure.find_units ();
    CADICAL_assert (unsat || closure.chain.empty ());
    CADICAL_assert (unsat || lrat_chain.empty ());
    if (!internal->unsat) {
      closure.find_equivalences ();
      CADICAL_assert (unsat || closure.chain.empty ());
      CADICAL_assert (unsat || lrat_chain.empty ());

      if (!unsat) {
        const int propagated = closure.propagate_units_and_equivalences ();
        CADICAL_assert (unsat || closure.chain.empty ());
        if (!unsat && propagated)
          closure.forward_subsume_matching_clauses ();
      }
    }
  }

  closure.reset_closure ();
  internal->clear_watches ();
  internal->connect_watches ();
  if (!internal->unsat) {
    propagated2 = propagated = 0;
    propagate ();
  }
  CADICAL_assert (closure.new_unwatched_binary_clauses.empty ());
  internal->reset_occs ();
  internal->reset_noccs ();
  CADICAL_assert (!internal->occurring ());
  CADICAL_assert (lrat_chain.empty ());

  const int64_t new_merged = stats.congruence.congruent;

#ifndef CADICAL_QUIET
  phase ("congruence-phase", stats.congruence.rounds, "merged %ld literals",
         new_merged - old_merged);
#endif
  if (!unsat && !internal->propagate ())
    unsat = true;

  STOP_SIMPLIFIER (congruence, CONGRUENCE);
  report ('c', !opts.reportall && !(stats.congruence.congruent - old));
#ifndef CADICAL_NDEBUG
  size_t watched = 0;
  for (auto v : vars) {
    for (auto sgn = -1; sgn <= 1; sgn += 2) {
      const int lit = v * sgn;
      for (auto w : watches (lit)) {
        if (w.binary ())
          CADICAL_assert (!w.clause->garbage);
        if (w.clause->garbage)
          continue;
        ++watched;
        LOG (w.clause, "watched");
      }
    }
  }
  LOG ("and now the clauses:");
  size_t nb_clauses = 0;
  for (auto c : clauses) {
    if (c->garbage)
      continue;
    LOG (c, "watched");
    ++nb_clauses;
  }
  CADICAL_assert (watched == nb_clauses * 2);
#endif
  CADICAL_assert (!internal->occurring ());

  if (new_merged == old_merged) {
    congruence_delay.bumpreasons.interval++;
  } else {
    congruence_delay.bumpreasons.interval /= 2;
  }

  LOG ("delay congruence internal %" PRId64,
       congruence_delay.bumpreasons.interval);
  congruence_delay.bumpreasons.limit =
      congruence_delay.bumpreasons.interval;

  return new_merged != old_merged;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
