#include "congruence.h"
#include "dense.h"
#include "fifo.h"
#include "inline.h"
#include "inlinevector.h"
#include "internal.h"
#include "logging.h"
#include "print.h"
#include "proprobe.h"
#include "rank.h"
#include "reference.h"
#include "report.h"
#include "sort.h"
#include "terminate.h"
#include "trail.h"
#include "utilities.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

ABC_NAMESPACE_IMPL_START

// #define INDEX_LARGE_CLAUSES
// #define INDEX_BINARY_CLAUSES
#define MERGE_CONDITIONAL_EQUIVALENCES

#define AND_GATE 0
#define XOR_GATE 1
#define ITE_GATE 2

#define LD_MAX_ARITY 26
#define MAX_ARITY ((1 << LD_MAX_ARITY) - 1)

struct gate {
#if defined(LOGGING) || !defined(KISSAT_NDEBUG)
  size_t id;
#endif
  unsigned lhs;
  unsigned hash;
  unsigned tag : 2;
  bool garbage : 1;
  bool indexed : 1;
  bool marked : 1;
  bool shrunken : 1;
  unsigned arity : LD_MAX_ARITY;
  unsigned rhs[];
};

typedef struct gate gate;
typedef STACK (gate *) gates;

struct gate_hash_table {
  gate **table;
  size_t size;
  size_t entries;
};

typedef struct gate_hash_table gate_hash_table;

#define REMOVED ((gate *) (~(uintptr_t) 0))

#define BEGIN_RHS(G) ((G)->rhs)
#define END_RHS(G) (BEGIN_RHS (G) + (G)->arity)

#define all_rhs_literals_in_gate(LIT, G) \
  unsigned LIT, \
      *LIT##_PTR = BEGIN_RHS (G), *const LIT##_END = END_RHS (G); \
  LIT##_PTR != LIT##_END && ((LIT = *LIT##_PTR), true); \
  ++LIT##_PTR

#ifdef INDEX_BINARY_CLAUSES

struct binary_clause {
  unsigned lits[2];
};

typedef struct binary_clause binary_clause;

struct binary_hash_table {
  binary_clause *table;
  size_t size, size2, count;
};

typedef struct binary_hash_table binary_hash_table;

#endif

struct hash_ref {
  unsigned hash;
  reference ref;
};

#ifdef INDEX_LARGE_CLAUSES

typedef struct hash_ref hash_ref;

struct large_clause_hash_table {
  hash_ref *table;
  size_t size, size2, count;
};

typedef struct large_clause_hash_table large_clause_hash_table;

#endif

#define SIZE_NONCES 16

struct closure {
  kissat *solver;
  bool *scheduled;
  gates *occurrences;
  gates garbage;
  unsigneds lits;
  unsigneds rhs;
  unsigneds unsimplified;
  litpairs binaries;
  unsigned_fifo schedule;
  unsigned *repr;
  gate_hash_table hash;
  uint64_t nonces[SIZE_NONCES];
  unsigned *units;
  unsigned *equivalences;
  unsigned *negbincount;
  unsigned *largecount;
  litpairs condbin[2];
  litpairs condeq[2];
#ifdef INDEX_BINARY_CLAUSES
  binary_hash_table bintab;
#endif
#ifdef INDEX_LARGE_CLAUSES
  large_clause_hash_table clauses;
#endif
#ifdef CHECKING_OR_PROVING
  unsigneds chain;
#endif
#if defined(LOGGING) || !defined(KISSAT_NDEBUG)
  size_t gates_added;
#endif
#ifndef KISSAT_NDEBUG
  unsigneds implied;
#endif
};

typedef struct closure closure;

static void init_closure (kissat *solver, closure *closure) {
  closure->solver = solver;
  CALLOC (bool, closure->scheduled, VARS);
  CALLOC (gates, closure->occurrences, LITS);
  INIT_STACK (closure->garbage);
  INIT_STACK (closure->lits);
  INIT_STACK (closure->rhs);
  INIT_STACK (closure->unsimplified);
  INIT_STACK (closure->binaries);
  INIT_FIFO (closure->schedule);

  NALLOC (unsigned, closure->repr, LITS);
  for (all_literals (lit))
    closure->repr[lit] = lit;

  closure->hash.table = 0;
  closure->hash.size = closure->hash.entries = 0;

  generator random = solver->random;
  for (size_t i = 0; i != SIZE_NONCES; i++)
    closure->nonces[i] = 1 | kissat_next_random64 (&random);

#ifdef CHECKING_OR_PROVING
  INIT_STACK (closure->chain);
#endif
#if defined(LOGGING) || !defined(KISSAT_NDEBUG)
  closure->gates_added = 0;
#endif
#ifndef KISSAT_NDEBUG
  INIT_STACK (closure->implied);
#endif
}

static size_t bytes_gate (size_t arity) {
  return sizeof (gate) + arity * sizeof (unsigned);
}

static unsigned actual_gate_arity (gate *g) {
  unsigned res = g->arity;
  if (!g->shrunken)
    return res;
  while (g->rhs[res++] != INVALID_LIT)
    ;
  return res;
}

#define CLOGANDGATE(G, ...) \
  do { \
    KISSAT_assert ((G)->tag == AND_GATE); \
    LOGANDGATE ((G)->id, closure->repr, (G)->lhs, (G)->arity, (G)->rhs, \
                __VA_ARGS__); \
  } while (0)

#define CLOGXORGATE(G, ...) \
  do { \
    KISSAT_assert ((G)->tag == XOR_GATE); \
    LOGXORGATE ((G)->id, closure->repr, (G)->lhs, (G)->arity, (G)->rhs, \
                __VA_ARGS__); \
  } while (0)

#define CLOGITEGATE(G, ...) \
  do { \
    KISSAT_assert ((G)->tag == ITE_GATE); \
    LOGITEGATE ((G)->id, closure->repr, (G)->lhs, (G)->rhs[0], \
                (G)->rhs[1], (G)->rhs[2], __VA_ARGS__); \
  } while (0)

#define CLOGREPR(L) LOGREPR ((L), closure->repr)

#define LOGATE(G, ...) \
  do { \
    if ((G)->tag == AND_GATE) \
      CLOGANDGATE (G, __VA_ARGS__); \
    else if ((G)->tag == XOR_GATE) \
      CLOGXORGATE (G, __VA_ARGS__); \
    else { \
      KISSAT_assert ((G)->tag == ITE_GATE); \
      CLOGITEGATE (G, __VA_ARGS__); \
    } \
  } while (0)

static void delete_gate (closure *closure, gate *g) {
  kissat *const solver = closure->solver;
  LOGATE (g, "delete");
  unsigned actual_arity = actual_gate_arity (g);
  size_t actual_bytes = bytes_gate (actual_arity);
  kissat_free (solver, g, actual_bytes);
}

void reset_gate_hash_table (closure *closure) {
  kissat *const solver = closure->solver;
  gate **table = closure->hash.table;
  for (size_t pos = 0; pos != closure->hash.size; pos++) {
    gate *g = table[pos];
    if (g && g != REMOVED && !g->garbage)
      delete_gate (closure, g);
  }
  DEALLOC (table, closure->hash.size);
}

static void reset_closure (closure *closure) {
  kissat *const solver = closure->solver;

  gates *occurrences = closure->occurrences;
  for (all_literals (lit))
    RELEASE_STACK (occurrences[lit]);
  DEALLOC (occurrences, LITS);

  reset_gate_hash_table (closure);
  for (all_pointers (gate, g, closure->garbage))
    delete_gate (closure, g);

  RELEASE_STACK (closure->garbage);
  RELEASE_STACK (closure->binaries);
  DEALLOC (closure->scheduled, VARS);
  RELEASE_STACK (closure->lits);
  RELEASE_STACK (closure->rhs);
  RELEASE_STACK (closure->unsimplified);
  RELEASE_FIFO (closure->schedule);
#ifdef CHECKING_OR_PROVING
  RELEASE_STACK (closure->chain);
#endif
#ifndef KISSAT_NDEBUG
  RELEASE_STACK (closure->implied);
#endif

  if (!solver->inconsistent && solver->unflushed)
    kissat_flush_trail (solver);
}

static unsigned reset_repr (closure *closure) {
  kissat *const solver = closure->solver;
  unsigned res = 0, *repr = closure->repr;
  for (all_variables (idx)) {
    unsigned lit = LIT (idx);
    if (!VALUE (lit) && repr[lit] != lit)
      res++;
  }
  DEALLOC (repr, LITS);
  return res;
}

#ifndef KISSAT_NDEBUG

static void check_lits_sorted (size_t size, const unsigned *lits) {
  unsigned prev = INVALID_LIT;
  const unsigned *const end_lits = lits + size;
  for (const unsigned *p = lits; p != end_lits; p++) {
    const unsigned lit = *p;
    if (prev != INVALID_LIT) {
      KISSAT_assert (prev != lit);
      const unsigned not_lit = lit ^ 1;
      KISSAT_assert (prev != not_lit);
      KISSAT_assert (prev < lit);
    }
    prev = lit;
  }
}

static void check_and_lits_normalized (size_t arity, const unsigned *lits) {
  KISSAT_assert (arity > 1);
  check_lits_sorted (arity, lits);
}

static void check_xor_lits_normalized (const unsigned arity,
                                       const unsigned *lits) {
  KISSAT_assert (arity > 1);
  check_lits_sorted (arity, lits);
  for (size_t i = 1; i != arity; i++)
    KISSAT_assert (lits[i - 1] < lits[i]);
}

static void check_ite_lits_normalized (kissat *solver,
                                       const unsigned *lits) {
  KISSAT_assert (!NEGATED (lits[0]));
  KISSAT_assert (!NEGATED (lits[1]));
  KISSAT_assert (lits[0] != lits[1]);
  KISSAT_assert (lits[0] != lits[2]);
  KISSAT_assert (lits[1] != lits[2]);
  KISSAT_assert (lits[0] != NOT (lits[1]));
  KISSAT_assert (lits[0] != NOT (lits[2]));
  KISSAT_assert (lits[1] != NOT (lits[2]));
}

#else

#define check_lits_sorted(...) \
  do { \
  } while (0)

#define check_and_lits_normalized check_lits_sorted
#define check_xor_lits_normalized check_lits_sorted
#define check_ite_lits_normalized check_lits_sorted

#endif

#define LESS_LIT(A, B) ((A) < (B))

static void sort_lits (kissat *solver, size_t arity, unsigned *lits) {
  SORT (unsigned, arity, lits, LESS_LIT);
  check_lits_sorted (arity, lits);
}

static unsigned hash_lits (closure *closure, unsigned tag, size_t arity,
                           const unsigned *lits) {
#ifndef KISSAT_NDEBUG
  if (tag == AND_GATE)
    check_and_lits_normalized (arity, lits);
  else if (tag == XOR_GATE)
    check_xor_lits_normalized (arity, lits);
  else {
    KISSAT_assert (tag == ITE_GATE);
    check_ite_lits_normalized (closure->solver, lits);
  }
#endif
  const unsigned *end_lits = lits + arity;
  const uint64_t *const nonces = closure->nonces;
  const uint64_t *const end_nonces = nonces + SIZE_NONCES;
  const uint64_t *n = nonces + tag;
  uint64_t hash = 0;
  KISSAT_assert (n < end_nonces);
  for (const unsigned *l = lits; l != end_lits; l++) {
    hash += *l;
    hash *= *n++;
    hash = (hash << 4) | (hash >> 60);
    if (n == end_nonces)
      n = nonces;
  }
  hash ^= hash >> 32;
  return hash;
}

#ifndef KISSAT_NDEBUG
static bool is_power_of_two (size_t n) { return n && ~(n & (n - 1)); }
#endif

static size_t reduce_hash (unsigned hash, size_t size, size_t size2) {
  KISSAT_assert (size <= size2);
  KISSAT_assert (size2 <= 2 * size);
  KISSAT_assert (is_power_of_two (size2));
  unsigned res = hash;
  res &= size2 - 1;
  if (res >= size)
    res -= size;
  KISSAT_assert (res < size);
  return res;
}

#define MAX_HASH_TABLE_SIZE ((size_t) 1 << 32)

static bool closure_hash_table_is_full (closure *closure) {
  if (closure->hash.size == MAX_HASH_TABLE_SIZE)
    return false;
  if (2 * closure->hash.entries < closure->hash.size)
    return false;
  return true;
}

static bool match_lits (gate *g, unsigned tag, unsigned hash, size_t size,
                        const unsigned *lits) {
  KISSAT_assert (!g->garbage);
  if (g->tag != tag)
    return false;
  if (g->hash != hash)
    return false;
  if (g->arity != size)
    return false;
  const unsigned *p = lits;
  for (all_rhs_literals_in_gate (lit, g))
    if (lit != *p++)
      return false;
  return true;
}

static void resize_gate_hash_table (closure *closure) {
  kissat *solver = closure->solver;
  gate_hash_table *hash = &closure->hash;
  const size_t old_size = hash->size;
  const size_t new_size = old_size ? 2 * old_size : 1;
  const size_t old_entries = hash->entries;
  kissat_extremely_verbose (
      solver,
      "resizing gate table of size %zu filled with %zu entries %.0f%%",
      old_size, old_entries, kissat_percent (old_entries, old_size));
  gate **old_table = hash->table, **new_table;
  CALLOC (gate*, new_table, new_size);
  size_t flushed = 0;
  for (size_t old_pos = 0; old_pos != old_size; old_pos++) {
    gate *g = old_table[old_pos];
    if (!g)
      continue;
    if (g == REMOVED) {
      flushed++;
      continue;
    }
    size_t new_pos = reduce_hash (g->hash, new_size, new_size);
    while (new_table[new_pos]) {
      KISSAT_assert (new_table[new_pos] != REMOVED);
      if (++new_pos == new_size)
        new_pos = 0;
    }
    new_table[new_pos] = g;
  }
  kissat_extremely_verbose (
      solver, "flushed %zu entries %.0f%% resizing table of size %zu",
      flushed, kissat_percent (flushed, old_size), old_size);
  DEALLOC (old_table, old_size);
  KISSAT_assert (flushed <= old_entries);
  const size_t new_entries = old_entries - flushed;
  hash->table = new_table;
  hash->size = new_size;
  hash->entries = new_entries;
  kissat_very_verbose (
      solver, "resized gate table to %zu with %zu entries %.0f%%", new_size,
      new_entries, kissat_percent (new_entries, new_size));
}

static bool remove_gate (closure *closure, gate *g) {
  if (!g->indexed)
    return false;
  kissat *solver = closure->solver;
  KISSAT_assert (!solver->inconsistent);
  const size_t hash_size = closure->hash.size;
  size_t pos = reduce_hash (g->hash, hash_size, hash_size);
  gate **table = closure->hash.table;
  INC (congruent_lookups);
  INC (congruent_lookups_removed);
  unsigned collisions = 0;
  while (table[pos] != g) {
    collisions++;
    if (++pos == hash_size)
      pos = 0;
  }
  ADD (congruent_collisions_removed, collisions);
  ADD (congruent_collisions, collisions);
  table[pos] = REMOVED;
  LOGATE (g, "removing from hash table");
  g->indexed = false;
  return true;
}

static gate *find_gate (closure *closure, unsigned tag, unsigned hash,
                        size_t size, const unsigned *lits, gate *except) {
  KISSAT_assert (!except || !except->garbage);
  if (!closure->hash.entries)
    return 0;
  kissat *solver = closure->solver;
  KISSAT_assert (!solver->inconsistent);
  KISSAT_assert (hash == hash_lits (closure, tag, size, lits));
  const size_t hash_size = closure->hash.size;
  size_t start_pos = reduce_hash (hash, hash_size, hash_size);
  gate **table = closure->hash.table, *g;
  INC (congruent_lookups);
  INC (congruent_lookups_find);
  size_t pos = start_pos;
  unsigned collisions = 0;
  gate *res = 0;
  while ((g = table[pos])) {
    if (g == REMOVED)
      ;
    else if (g->garbage) {
      KISSAT_assert (g->indexed);
      g->indexed = false;
      table[pos] = REMOVED;
    } else if (g != except && match_lits (g, tag, hash, size, lits)) {
      INC (congruent_matched);
      res = g;
      break;
    }
    collisions++;
    if (++pos == hash_size)
      pos = 0;
    if (pos == start_pos)
      break;
  }
  ADD (congruent_collisions_find, collisions);
  ADD (congruent_collisions, collisions);
  return res;
}

static void index_gate (closure *closure, gate *g) {
  KISSAT_assert (!g->indexed);
  kissat *solver = closure->solver;
  KISSAT_assert (!solver->inconsistent);
  KISSAT_assert (g->arity > 1);
  if (closure_hash_table_is_full (closure))
    resize_gate_hash_table (closure);
  LOGATE (g, "adding to hash table");
  INC (congruent_indexed);
  KISSAT_assert (g->hash == hash_lits (closure, g->tag, g->arity, g->rhs));
  const size_t hash_size = closure->hash.size;
  size_t pos = reduce_hash (g->hash, hash_size, hash_size);
  gate **table = closure->hash.table, *h;
  unsigned collisions = 0;
  while ((h = table[pos]) && h != REMOVED) {
    collisions++;
    if (++pos == hash_size)
      pos = 0;
  }
  ADD (congruent_collisions_index, collisions);
  ADD (congruent_collisions, collisions);
  table[pos] = g;
  closure->hash.entries++;
  g->indexed = true;
}

static unsigned parity_lits (kissat *solver, unsigneds *lits) {
  unsigned res = 0;
  for (all_stack (unsigned, lit, *lits))
    res ^= NEGATED (lit);
#ifdef KISSAT_NDEBUG
  (void) solver;
#endif
  return res;
}

static void inc_lits (kissat *solver, unsigneds *lits) {
  unsigned *p = BEGIN_STACK (*lits);
  unsigned *end = END_STACK (*lits);
  unsigned carry = 1;
  while (carry && p != end) {
    unsigned lit = *p;
    unsigned not_lit = NOT (lit);
    carry = !NEGATED (not_lit);
    *p++ = not_lit;
  }
#ifdef KISSAT_NDEBUG
  (void) solver;
#endif
}

#ifndef KISSAT_NDEBUG

#define LESS_LITERAL(A, B) ((A) < (B))

static void check_implied (closure *closure) {
  kissat *const solver = closure->solver;
  unsigneds *implied = &closure->implied;
  SORT_STACK (unsigned, *implied, LESS_LITERAL);
  unsigned *q = BEGIN_STACK (*implied);
  const unsigned *const end = END_STACK (*implied);
  unsigned prev = INVALID_LIT;
  bool tautological = false;
  for (const unsigned *p = q; p != end; p++) {
    const unsigned lit = *p;
    if (prev == lit)
      continue;
    const unsigned not_lit = NOT (lit);
    if (prev == not_lit) {
      tautological = true;
      break;
    }
    *q++ = prev = lit;
  }
  if (!tautological) {
    SET_END_OF_STACK (*implied, q);
    CHECK_AND_ADD_STACK (*implied);
    REMOVE_CHECKER_STACK (*implied);
  }
  CLEAR_STACK (*implied);
}

static void check_clause (closure *closure) {
  kissat *const solver = closure->solver;
  unsigneds *implied = &closure->implied;
  unsigneds *clause = &solver->clause;
  for (all_stack (unsigned, lit, *clause))
    PUSH_STACK (*implied, lit);
  check_implied (closure);
}

static void check_binary_implied (closure *closure, unsigned a,
                                  unsigned b) {
  kissat *const solver = closure->solver;
  unsigneds *implied = &closure->implied;
  KISSAT_assert (EMPTY_STACK (*implied));
  PUSH_STACK (*implied, a);
  PUSH_STACK (*implied, b);
  check_implied (closure);
}

static void check_and_gate_implied (closure *closure, gate *g) {
  KISSAT_assert (g->tag == AND_GATE);
  kissat *const solver = closure->solver;
  if (GET_OPTION (check) < 2)
    return;
  CLOGANDGATE (g, "checking implied");
  const unsigned lhs = g->lhs;
  const unsigned not_lhs = NOT (lhs);
  for (all_rhs_literals_in_gate (other, g))
    check_binary_implied (closure, not_lhs, other);
  unsigneds *implied = &closure->implied;
  KISSAT_assert (EMPTY_STACK (*implied));
  PUSH_STACK (*implied, lhs);
  for (all_rhs_literals_in_gate (other, g)) {
    const unsigned not_other = NOT (other);
    PUSH_STACK (*implied, not_other);
  }
  check_implied (closure);
}

static void check_xor_gate_implied (closure *closure, gate *g) {
  KISSAT_assert (g->tag == XOR_GATE);
  kissat *const solver = closure->solver;
  if (GET_OPTION (check) < 2)
    return;
  CLOGXORGATE (g, "checking implied");
  const unsigned lhs = g->lhs;
  const unsigned not_lhs = NOT (lhs);
  unsigneds *clause = &solver->clause;
  KISSAT_assert (EMPTY_STACK (*clause));
  PUSH_STACK (*clause, not_lhs);
  for (all_rhs_literals_in_gate (other, g)) {
    KISSAT_assert (!NEGATED (other));
    PUSH_STACK (*clause, other);
  }
  unsigned arity = g->arity;
  unsigned end = 1u << arity;
  unsigned parity = NEGATED (not_lhs);
  KISSAT_assert (parity == parity_lits (solver, clause));
  for (unsigned i = 0; i != end; i++) {
    while (i && parity_lits (solver, clause) != parity)
      inc_lits (solver, clause);
    check_clause (closure);
    inc_lits (solver, clause);
  }
  CLEAR_STACK (*clause);
}

static void check_ternary (closure *closure, unsigned a, unsigned b,
                           unsigned c) {
  kissat *const solver = closure->solver;
  if (GET_OPTION (check) < 2)
    return;
  unsigneds *implied = &closure->implied;
  PUSH_STACK (*implied, a);
  PUSH_STACK (*implied, b);
  PUSH_STACK (*implied, c);
  check_implied (closure);
}

static void check_ite_implied (closure *closure, unsigned lhs,
                               unsigned cond, unsigned then_lit,
                               unsigned else_lit) {
  kissat *const solver = closure->solver;
  if (GET_OPTION (check) < 2)
    return;
  LOG ("checking implied ITE gate %s := %s ? %s : %s", LOGLIT (lhs),
       LOGLIT (cond), LOGLIT (then_lit), LOGLIT (else_lit));
  const unsigned not_lhs = NOT (lhs);
  const unsigned not_cond = NOT (cond);
  const unsigned not_then_lit = NOT (then_lit);
  const unsigned not_else_lit = NOT (else_lit);
  check_ternary (closure, cond, not_else_lit, lhs);
  check_ternary (closure, cond, else_lit, not_lhs);
  check_ternary (closure, not_cond, not_then_lit, lhs);
  check_ternary (closure, not_cond, then_lit, not_lhs);
}

static void check_ite_gate_implied (closure *closure, gate *g) {
  KISSAT_assert (g->tag == ITE_GATE);
  KISSAT_assert (g->arity == 3);
#ifndef KISSAT_NOPTIONS
  kissat *const solver = closure->solver;
#endif
  if (GET_OPTION (check) < 2)
    return;
  const unsigned lhs = g->lhs;
  const unsigned cond = g->rhs[0];
  const unsigned then_lit = g->rhs[1];
  const unsigned else_lit = g->rhs[2];
  check_ite_implied (closure, lhs, cond, then_lit, else_lit);
}

#else

#define check_and_gate_implied(...) \
  do { \
  } while (0)

#define check_xor_gate_implied check_and_gate_implied
#define check_ternary check_and_gate_implied
#define check_ite_implied check_and_gate_implied
#define check_ite_gate_implied check_and_gate_implied

#endif

static inline unsigned find_repr (closure *closure, unsigned lit) {
  const unsigned *const repr = closure->repr;
  unsigned res = lit, next = repr[res];
  while (res != next)
    res = next, next = repr[res];
  return res;
}

#ifndef MERGE_CONDITIONAL_EQUIVALENCES

static clause *find_other_two (kissat *solver, watches *watches, unsigned a,
                               unsigned b, unsigned ignore) {
  KISSAT_assert (!solver->watching);
  const value *const values = solver->values;
  const watch *const begin_watches = BEGIN_WATCHES (*watches);
  const watch *const end_watches = END_WATCHES (*watches);
  const watch *p = begin_watches;
  while (p != end_watches) {
    const watch watch = *p++;
    KISSAT_assert (!watch.type.binary);
    const reference ref = watch.large.ref;
    clause *c = kissat_dereference_clause (solver, ref);
    KISSAT_assert (!c->garbage);
    unsigned found = 0;
    for (all_literals_in_clause (lit, c)) {
      if (values[lit])
        continue;
      if (lit == ignore)
        continue;
      if (lit == a || lit == b) {
        found++;
        continue;
      }
      goto CONTINUE_WITH_NEXT_WATCH;
    }
    KISSAT_assert (found <= 2);
    if (found == 2)
      return c;
  CONTINUE_WITH_NEXT_WATCH:;
  }
  return 0;
}

static clause *find_ternary_clause (kissat *solver, unsigned a, unsigned b,
                                    unsigned c) {
  KISSAT_assert (!solver->watching);
  watches *const a_watches = &WATCHES (a);
  watches *const b_watches = &WATCHES (b);
  watches *const c_watches = &WATCHES (c);
  const size_t size_a = SIZE_WATCHES (*a_watches);
  const size_t size_b = SIZE_WATCHES (*b_watches);
  const size_t size_c = SIZE_WATCHES (*c_watches);
  if (size_a <= size_b && size_a <= size_b)
    return find_other_two (solver, a_watches, b, c, a);
  if (size_b <= size_a && size_b <= size_c)
    return find_other_two (solver, b_watches, a, c, b);
  KISSAT_assert (size_c <= size_a && size_c <= size_b);
  return find_other_two (solver, c_watches, a, b, c);
}

#endif

static bool learn_congruence_unit (closure *closure, unsigned unit) {
  kissat *const solver = closure->solver;
  KISSAT_assert (!solver->inconsistent);
  const value value = solver->values[unit];
  if (value > 0)
    return true;
  INC (congruent_units);
  if (value < 0) {
    solver->inconsistent = 1;
    LOG ("inconsistent congruence unit %s", LOGLIT (unit));
    CHECK_AND_ADD_EMPTY ();
    ADD_EMPTY_TO_PROOF ();
    return false;
  }
  LOG ("learning congruence unit %s", LOGLIT (unit));
  kissat_learned_unit (solver, unit);
  clause *conflict = kissat_probing_propagate (solver, 0, false);
  if (!conflict)
    return true;
  KISSAT_assert (solver->inconsistent);
  LOG ("propagating congruence unit %s yields conflict", LOGLIT (unit));
  return false;
}

static void add_binary_clause (closure *closure, unsigned a, unsigned b) {
  kissat *const solver = closure->solver;
  if (solver->inconsistent)
    return;
  if (a == NOT (b))
    return;
  value a_value = VALUE (a);
  if (a_value > 0)
    return;
  value b_value = VALUE (b);
  if (b_value > 0)
    return;
  unsigned unit = INVALID_LIT;
  if (a == b)
    unit = a;
  else if (a_value < 0 && !b_value)
    unit = b;
  else if (!a_value && b_value < 0)
    unit = a;
  if (unit != INVALID_LIT) {
    (void) !learn_congruence_unit (closure, unit);
    return;
  }
  KISSAT_assert (!a_value), KISSAT_assert (!b_value);
  LOGBINARY (a, b, "adding representative");
  if (solver->watching)
    kissat_new_binary_clause (solver, a, b);
  else {
    kissat_new_unwatched_binary_clause (solver, a, b);
    litpair litpair = {.lits = {a < b ? a : b, a < b ? b : a}};
    PUSH_STACK (closure->binaries, litpair);
  }
}

static void schedule_literal (closure *closure, unsigned lit) {
  kissat *const solver = closure->solver;
  unsigned idx = IDX (lit);
  bool *scheduled = closure->scheduled + idx;
  if (*scheduled)
    return;
  *scheduled = true;
  ENQUEUE_FIFO (unsigned, closure->schedule, lit);
  LOG ("scheduled propagation of merged %s", CLOGREPR (lit));
}

static unsigned dequeue_next_scheduled_literal (closure *closure) {
  unsigned res;
  DEQUEUE_FIFO (closure->schedule, res);
#if defined(LOGGING) || !defined(KISSAT_NDEBUG)
  kissat *const solver = closure->solver;
#endif
  unsigned idx = IDX (res);
  bool *scheduled = closure->scheduled + idx;
  KISSAT_assert (*scheduled);
  *scheduled = false;
  LOG ("dequeued from schedule %s", CLOGREPR (res));
  return res;
}

static bool merge_literals (closure *closure, unsigned lit,
                            unsigned other) {
  kissat *const solver = closure->solver;
  KISSAT_assert (!solver->inconsistent);
  unsigned repr_lit = find_repr (closure, lit);
  unsigned repr_other = find_repr (closure, other);
  unsigned *const repr = closure->repr;
  if (repr_lit == repr_other) {
    LOG ("already merged %s and %s", LOGREPR (lit, repr),
         LOGREPR (other, repr));
    return false;
  }
  const value *const values = solver->values;
  const value lit_value = values[lit];
  const value other_value = values[other];
  KISSAT_assert (lit_value == values[repr_lit]);
  KISSAT_assert (other_value == values[repr_other]);
  if (lit_value) {
    if (lit_value == other_value) {
      LOG ("not merging %s and %s assigned to the same value",
           LOGREPR (lit, repr), LOGREPR (other, repr));
      return false;
    }
    if (lit_value == -other_value) {
      LOG ("merging inconsistently assigned %s and %s", LOGREPR (lit, repr),
           LOGREPR (other, repr));
      solver->inconsistent = true;
      CHECK_AND_ADD_EMPTY ();
      ADD_EMPTY_TO_PROOF ();
      return false;
    }
    KISSAT_assert (!other_value);
    LOG ("merging assigned %s and unassigned %s", LOGREPR (lit, repr),
         LOGREPR (other, repr));
    const unsigned unit = (lit_value < 0) ? NOT (other) : other;
    (void) learn_congruence_unit (closure, unit);
    return false;
  }
  if (!lit_value && other_value) {
    LOG ("merging unassigned %s and assigned %s", LOGREPR (lit, repr),
         LOGREPR (other, repr));
    const unsigned unit = (other_value < 0) ? NOT (lit) : lit;
    (void) learn_congruence_unit (closure, unit);
    return false;
  }
  unsigned smaller = repr_lit;
  unsigned larger = repr_other;
  if (smaller > larger)
    SWAP (unsigned, smaller, larger);
  KISSAT_assert (repr[smaller] == smaller);
  KISSAT_assert (repr[larger] > smaller);
  if (repr_lit == NOT (repr_other)) {
    LOG ("merging clashing %s and %s", LOGREPR (lit, repr),
         LOGREPR (other, repr));
    kissat_learned_unit (solver, smaller);
    solver->inconsistent = true;
    CHECK_AND_ADD_EMPTY ();
    ADD_EMPTY_TO_PROOF ();
    return false;
  }
  LOG ("merging %s and %s", LOGREPR (lit, repr), LOGREPR (other, repr));
  const unsigned not_smaller = NOT (smaller);
  const unsigned not_larger = NOT (larger);
  repr[larger] = smaller;
  repr[not_larger] = not_smaller;
  LOG ("congruence repr[%s] = %s", LOGLIT (larger), LOGLIT (smaller));
  LOG ("congruence repr[%s] = %s", LOGLIT (not_larger),
       LOGLIT (not_smaller));
  add_binary_clause (closure, not_larger, smaller);
  add_binary_clause (closure, larger, not_smaller);
  schedule_literal (closure, larger);
  INC (congruent);
  return true;
}

static void connect_occurrence (closure *closure, unsigned lit, gate *g) {
  gates *const occurrences = closure->occurrences;
  kissat *const solver = closure->solver;
  PUSH_STACK (occurrences[lit], g);
  LOG ("connected %s to gate[%zu]", LOGLIT (lit), g->id);
}

static gate *new_gate (closure *closure, unsigned tag, unsigned hash,
                       unsigned lhs, unsigned arity, const unsigned *lits) {
  kissat *const solver = closure->solver;
  const size_t bytes = bytes_gate (arity);
  gate *g = (gate*)kissat_malloc (solver, bytes);
#if defined(LOGGING) || !defined(KISSAT_NDEBUG)
  g->id = closure->gates_added++;
#endif
  g->tag = tag;
  g->hash = hash;
  g->lhs = lhs;
  g->arity = arity;
  g->garbage = false;
  g->indexed = false;
  g->marked = false;
  g->shrunken = false;
  memcpy (g->rhs, lits, arity * sizeof *lits);
  for (all_rhs_literals_in_gate (lit, g))
    connect_occurrence (closure, lit, g);
  LOGATE (g, "new");
  index_gate (closure, g);
  ADD (congruent_arity, arity);
  INC (congruent_gates);
  return g;
}

static gate *find_and_lits (closure *closure, unsigned *hash_ptr,
                            unsigned arity, unsigned *lits, gate *except) {
  kissat *const solver = closure->solver;
  sort_lits (solver, arity, lits);
  const unsigned hash = hash_lits (closure, AND_GATE, arity, lits);
  gate *g = find_gate (closure, AND_GATE, hash, arity, lits, except);
  *hash_ptr = hash;
  if (g) {
    CLOGANDGATE (g, "found matching");
    INC (congruent_matched_ands);
  } else
    LOGANDGATE (INVALID_GATE_ID, closure->repr, INVALID_LIT, arity, lits,
                "could not find matching");
  return g;
}

static gate *find_and_gate (closure *closure, unsigned *h, gate *g) {
  return find_and_lits (closure, h, g->arity, g->rhs, g);
}

static gate *new_and_gate (closure *closure, unsigned lhs) {
  kissat *const solver = closure->solver;
  unsigneds *all_lits = &closure->lits;
  unsigneds *rhs_stack = &closure->rhs;
  CLEAR_STACK (*rhs_stack);
  for (all_stack (unsigned, lit, *all_lits))
    if (lhs != lit) {
      unsigned not_lit = NOT (lit);
      KISSAT_assert (lhs != not_lit);
      PUSH_STACK (*rhs_stack, not_lit);
    }
  const unsigned arity = SIZE_STACK (*rhs_stack);
  unsigned *rhs_lits = BEGIN_STACK (*rhs_stack);
  KISSAT_assert (arity + 1 == SIZE_STACK (*all_lits));
  unsigned hash;
  gate *g = find_and_lits (closure, &hash, arity, rhs_lits, 0);
  if (g) {
    if (merge_literals (closure, g->lhs, lhs))
      INC (congruent_ands);
    return 0;
  }
  g = new_gate (closure, AND_GATE, hash, lhs, arity, rhs_lits);
  check_and_gate_implied (closure, g);
  ADD (congruent_arity_ands, arity);
  INC (congruent_gates_ands);
  return g;
}

#ifdef CHECKING_OR_PROVING

static void copy_literals (kissat *solver, unsigneds *dst,
                           const unsigneds *src) {
  for (all_stack (unsigned, lit, *src))
    PUSH_STACK (*dst, lit);
  PUSH_STACK (*dst, INVALID_LIT);
}

static void simplify_and_add_to_proof_chain (kissat *solver, mark *marks,
                                             unsigneds *unsimplified,
                                             unsigneds *clause,
                                             unsigneds *chain) {
  KISSAT_assert (EMPTY_STACK (*clause));
#ifndef KISSAT_NDEBUG
  for (all_stack (unsigned, lit, *unsimplified))
    KISSAT_assert (!(marks[lit] & 4));
#endif
  bool trivial = false;
  for (all_stack (unsigned, lit, *unsimplified)) {
    mark lit_mark = marks[lit];
    if (lit_mark & 4)
      continue;
    const unsigned not_lit = NOT (lit);
    const mark not_lit_mark = marks[not_lit];
    if (not_lit_mark & 4) {
      trivial = true;
      break;
    }
    lit_mark |= 4;
    marks[lit] = lit_mark;
    PUSH_STACK (*clause, lit);
  }
  for (all_stack (unsigned, lit, *clause)) {
    mark mark = marks[lit];
    KISSAT_assert (mark & 4);
    mark &= ~4u;
    marks[lit] = mark;
  }
  if (!trivial) {
    CHECK_AND_ADD_STACK (*clause);
    ADD_STACK_TO_PROOF (*clause);
    copy_literals (solver, chain, clause);
  }
  CLEAR_STACK (*clause);
}

#define SIMPLIFY_AND_ADD_TO_PROOF_CHAIN() \
  simplify_and_add_to_proof_chain (solver, marks, unsimplified, clause, \
                                   chain)

static void add_xor_matching_proof_chain (closure *closure, gate *g,
                                          unsigned lhs1, unsigned lhs2) {
  if (lhs1 == lhs2)
    return;
  kissat *const solver = closure->solver;
  if (!kissat_checking_or_proving (solver))
    return;
  LOG ("starting XOR matching proof chain");
  unsigneds *const unsimplified = &closure->unsimplified;
  unsigneds *const clause = &solver->clause;
  unsigneds *const chain = &closure->chain;
  mark *const marks = solver->marks;
  KISSAT_assert (EMPTY_STACK (*unsimplified));
  KISSAT_assert (EMPTY_STACK (*chain));
  KISSAT_assert (g->arity > 1);
  const unsigned reduced_arity = g->arity - 1;
  for (unsigned i = 0; i != reduced_arity; i++)
    PUSH_STACK (*unsimplified, g->rhs[i]);
  const unsigned not_lhs1 = NOT (lhs1);
  const unsigned not_lhs2 = NOT (lhs2);
  do {
    const size_t size = SIZE_STACK (*unsimplified);
    KISSAT_assert (size < 32);
    for (unsigned i = 0; i != 1u << size; i++) {
      PUSH_STACK (*unsimplified, not_lhs1);
      PUSH_STACK (*unsimplified, lhs2);
      SIMPLIFY_AND_ADD_TO_PROOF_CHAIN ();
      unsimplified->end -= 2;
      PUSH_STACK (*unsimplified, lhs1);
      PUSH_STACK (*unsimplified, not_lhs2);
      SIMPLIFY_AND_ADD_TO_PROOF_CHAIN ();
      unsimplified->end -= 2;
      inc_lits (solver, unsimplified);
    }
    KISSAT_assert (!EMPTY_STACK (*unsimplified));
    unsimplified->end--;
  } while (!EMPTY_STACK (*unsimplified));
  LOG ("finished XOR matching proof chain");
}

static void delete_proof_chain (closure *closure) {
  kissat *const solver = closure->solver;
  unsigneds *chain = &closure->chain;
  if (!kissat_checking_or_proving (solver)) {
    KISSAT_assert (EMPTY_STACK (*chain));
    return;
  }
  if (EMPTY_STACK (*chain))
    return;
  LOG ("starting deletion of proof chain");
  unsigneds *clause = &solver->clause;
  KISSAT_assert (EMPTY_STACK (*clause));
  const unsigned *start = BEGIN_STACK (*chain);
  const unsigned *end = END_STACK (*chain);
  const unsigned *p = start;
  while (p != end) {
    const unsigned lit = *p;
    if (lit == INVALID_LIT) {
      while (start != p) {
        const unsigned other = *start++;
        PUSH_STACK (*clause, other);
      }
      REMOVE_CHECKER_STACK (*clause);
      DELETE_STACK_FROM_PROOF (*clause);
      CLEAR_STACK (*clause);
      start++;
    }
    p++;
  }
  KISSAT_assert (EMPTY_STACK (*clause));
  KISSAT_assert (start == end);
  CLEAR_STACK (*chain);
  LOG ("finished deletion of proof chain");
}

#else

#define add_xor_matching_proof_chain(...) \
  do { \
  } while (0)

#define delete_proof_chain(...) \
  do { \
  } while (0)

#endif

static gate *find_xor_lits (closure *closure, unsigned *hash_ptr,
                            unsigned arity, unsigned *lits, gate *except) {
  kissat *const solver = closure->solver;
  sort_lits (solver, arity, lits);
  const unsigned hash = hash_lits (closure, XOR_GATE, arity, lits);
  gate *g = find_gate (closure, XOR_GATE, hash, arity, lits, except);
  *hash_ptr = hash;
  if (g) {
    CLOGXORGATE (g, "found matching");
    INC (congruent_matched_xors);
  } else
    LOGXORGATE (INVALID_GATE_ID, closure->repr, INVALID_LIT, arity, lits,
                "tried but did not find matching");
  return g;
}

static gate *find_xor_gate (closure *closure, unsigned *h, gate *g) {
  return find_xor_lits (closure, h, g->arity, g->rhs, g);
}

static gate *new_xor_gate (closure *closure, unsigned lhs) {
  kissat *const solver = closure->solver;
  unsigneds *all_lits = &closure->lits;
  unsigneds *rhs_stack = &closure->rhs;
  CLEAR_STACK (*rhs_stack);
  const unsigned not_lhs = NOT (lhs);
  for (all_stack (unsigned, lit, *all_lits))
    if (lit != lhs && lit != not_lhs) {
      KISSAT_assert (!NEGATED (lit));
      PUSH_STACK (*rhs_stack, lit);
    }
  const unsigned arity = SIZE_STACK (*rhs_stack);
  unsigned *rhs_lits = BEGIN_STACK (*rhs_stack);
  KISSAT_assert (arity + 1 == SIZE_STACK (*all_lits));
  unsigned hash;
  gate *g = find_xor_lits (closure, &hash, arity, rhs_lits, 0);
  if (g) {
    add_xor_matching_proof_chain (closure, g, g->lhs, lhs);
    if (merge_literals (closure, g->lhs, lhs))
      INC (congruent_xors);
    if (!solver->inconsistent)
      delete_proof_chain (closure);
    return 0;
  }
  g = new_gate (closure, XOR_GATE, hash, lhs, arity, rhs_lits);
  check_xor_gate_implied (closure, g);
  ADD (congruent_arity_xors, arity);
  INC (congruent_gates_xors);
  return g;
}

#ifdef CHECKING_OR_PROVING

static void add_ite_matching_proof_chain (closure *closure, gate *g,
                                          unsigned lhs1, unsigned lhs2) {
  if (lhs1 == lhs2)
    return;
  kissat *const solver = closure->solver;
  if (!kissat_checking_or_proving (solver))
    return;
  LOG ("starting ITE matching proof chain");
  unsigneds *const unsimplified = &closure->unsimplified;
  unsigneds *clause = &solver->clause;
  mark *const marks = solver->marks;
  unsigneds *chain = &closure->chain;
  KISSAT_assert (EMPTY_STACK (*clause));
  KISSAT_assert (EMPTY_STACK (*chain));
  const unsigned *rhs = g->rhs;
  const unsigned cond = rhs[0];
  const unsigned not_cond = NOT (cond);
  const unsigned not_lhs1 = NOT (lhs1);
  const unsigned not_lhs2 = NOT (lhs2);
  PUSH_STACK (*unsimplified, lhs1);
  PUSH_STACK (*unsimplified, not_lhs2);
  PUSH_STACK (*unsimplified, cond);
  SIMPLIFY_AND_ADD_TO_PROOF_CHAIN ();
  unsimplified->end--;
  PUSH_STACK (*unsimplified, not_cond);
  SIMPLIFY_AND_ADD_TO_PROOF_CHAIN ();
  unsimplified->end--;
  CHECK_AND_ADD_STACK (*unsimplified);
  ADD_STACK_TO_PROOF (*unsimplified);
  copy_literals (solver, chain, unsimplified);
  CLEAR_STACK (*unsimplified);
  PUSH_STACK (*unsimplified, not_lhs1);
  PUSH_STACK (*unsimplified, lhs2);
  PUSH_STACK (*unsimplified, cond);
  SIMPLIFY_AND_ADD_TO_PROOF_CHAIN ();
  unsimplified->end--;
  PUSH_STACK (*unsimplified, not_cond);
  SIMPLIFY_AND_ADD_TO_PROOF_CHAIN ();
  unsimplified->end--;
  SIMPLIFY_AND_ADD_TO_PROOF_CHAIN ();
  CLEAR_STACK (*unsimplified);
  LOG ("finished ITE matching proof chain");
}

static void add_ite_turned_and_binary_clauses (closure *closure, gate *g) {
  kissat *const solver = closure->solver;
  if (!kissat_checking_or_proving (solver))
    return;
  LOG ("starting ITE turned AND supporting binary clauses");
  unsigneds *const unsimplified = &closure->unsimplified;
  unsigneds *clause = &solver->clause;
  unsigneds *chain = &closure->chain;
  mark *const marks = solver->marks;
  KISSAT_assert (EMPTY_STACK (*unsimplified));
  KISSAT_assert (EMPTY_STACK (*chain));
  const unsigned not_lhs = NOT (g->lhs);
  const unsigned *rhs = g->rhs;
  PUSH_STACK (*unsimplified, not_lhs);
  PUSH_STACK (*unsimplified, rhs[0]);
  SIMPLIFY_AND_ADD_TO_PROOF_CHAIN ();
  unsimplified->end--;
  PUSH_STACK (*unsimplified, rhs[1]);
  SIMPLIFY_AND_ADD_TO_PROOF_CHAIN ();
  CLEAR_STACK (*unsimplified);
}

#else

#define add_ite_matching_proof_chain(...) \
  do { \
  } while (0)

#define add_ite_turned_and_binary_clauses add_ite_matching_proof_chain

#endif

static bool normalize_ite_lits (kissat *solver, unsigned *lits) {
#ifdef KISSAT_NDEBUG
  (void) solver;
#endif
  if (NEGATED (lits[0])) {
    lits[0] = NOT (lits[0]);
    SWAP (unsigned, lits[1], lits[2]);
  }
  if (!NEGATED (lits[1]))
    return false;
  lits[1] = NOT (lits[1]);
  lits[2] = NOT (lits[2]);
  return true;
}

static gate *find_ite_lits (closure *closure, unsigned *hash_ptr,
                            bool *negate_lhs_ptr, unsigned arity,
                            unsigned *lits, gate *except) {
  kissat *const solver = closure->solver;
  KISSAT_assert (arity == 3);
  LOGITEGATE (INVALID_GATE_ID, closure->repr, INVALID_LIT, lits[0], lits[1],
              lits[2], "finding not yet normalized");
  bool negate_lhs = normalize_ite_lits (solver, lits);
#ifdef LOGGING
  if (negate_lhs)
    LOG ("normalization forces negation of LHS");
  LOGITEGATE (INVALID_GATE_ID, closure->repr, INVALID_LIT, lits[0], lits[1],
              lits[2], "normalized");
#endif
  *negate_lhs_ptr = negate_lhs;
  const unsigned hash = hash_lits (closure, ITE_GATE, arity, lits);
  gate *g = find_gate (closure, ITE_GATE, hash, arity, lits, except);
  *hash_ptr = hash;
  if (g) {
    CLOGITEGATE (g, "found matching");
    INC (congruent_matched_ites);
  } else
    LOGITEGATE (INVALID_GATE_ID, closure->repr, INVALID_LIT, lits[0],
                lits[1], lits[2], "tried but did not find matching");
  return g;
}

static gate *find_ite_gate (closure *closure, unsigned *h,
                            bool *negate_lhs_ptr, gate *g) {
  return find_ite_lits (closure, h, negate_lhs_ptr, g->arity, g->rhs, g);
}

static gate *new_ite_gate (closure *closure, unsigned lhs, unsigned cond,
                           unsigned then_lit, unsigned else_lit) {
  kissat *const solver = closure->solver;
  const unsigned not_then_lit = NOT (then_lit);
  if (else_lit == not_then_lit) {
#ifdef LOGGING
    if (NEGATED (then_lit))
      LOG ("skipping ternary XOR gate %s := %s ^ %s", LOGLIT (lhs),
           LOGLIT (cond), LOGLIT (not_then_lit));
    else
      LOG ("skipping ternary XOR gate %s := %s ^ %s", LOGLIT (NOT (lhs)),
           LOGLIT (cond), LOGLIT (then_lit));
#endif
    return 0;
  }
  if (else_lit == then_lit) {
    LOG ("found trivial ITE gate %s := %s ? %s : %s", LOGLIT (lhs),
         LOGLIT (cond), LOGLIT (then_lit), LOGLIT (else_lit));
    if (merge_literals (closure, lhs, then_lit))
      INC (congruent_trivial_ite);
    return 0;
  }
  unsigneds *rhs_stack = &closure->rhs;
  CLEAR_STACK (*rhs_stack);
  PUSH_STACK (*rhs_stack, cond);
  PUSH_STACK (*rhs_stack, then_lit);
  PUSH_STACK (*rhs_stack, else_lit);
  KISSAT_assert (SIZE_STACK (*rhs_stack) == 3);
  const unsigned arity = 3;
  unsigned *rhs_lits = BEGIN_STACK (*rhs_stack);
  bool negate_lhs;
  unsigned hash;
  gate *g = find_ite_lits (closure, &hash, &negate_lhs, arity, rhs_lits, 0);
  if (g) {
    if (negate_lhs)
      lhs = NOT (lhs);
    add_ite_matching_proof_chain (closure, g, g->lhs, lhs);
    if (merge_literals (closure, g->lhs, lhs))
      INC (congruent_ites);
    if (!solver->inconsistent)
      delete_proof_chain (closure);
    return 0;
  }
  if (negate_lhs)
    lhs = NOT (lhs);
  g = new_gate (closure, ITE_GATE, hash, lhs, arity, rhs_lits);
  check_ite_gate_implied (closure, g);
  INC (congruent_gates_ites);
  return g;
}

static void mark_gate_as_garbage (closure *closure, gate *g) {
  kissat *const solver = closure->solver;
  KISSAT_assert (!g->garbage);
  g->garbage = true;
  LOGATE (g, "marked as garbage");
  PUSH_STACK (closure->garbage, g);
}

static void shrink_gate (closure *closure, gate *g,
                         const unsigned *new_end_rhs) {
  unsigned *const rhs = g->rhs;
  const unsigned old_arity = g->arity;
  unsigned *const old_end_rhs = rhs + old_arity;
  KISSAT_assert (rhs <= new_end_rhs);
  KISSAT_assert (new_end_rhs <= old_end_rhs);
  if (new_end_rhs == old_end_rhs)
    return;
  const unsigned new_arity = new_end_rhs - rhs;
  if (!g->shrunken) {
    KISSAT_assert (old_end_rhs[-1] != INVALID_LIT);
    old_end_rhs[-1] = INVALID_LIT;
    g->shrunken = true;
  }
  g->arity = new_arity;
#ifdef LOGGING
  kissat *const solver = closure->solver;
  LOGATE (g, "shrunken");
#else
  (void) closure;
#endif
}

static bool skip_and_gate (closure *closure, gate *g) {
  KISSAT_assert (g->tag == AND_GATE);
  if (g->garbage)
    return true;
  kissat *const solver = closure->solver;
  const value *const values = solver->values;
  const unsigned lhs = g->lhs;
  const value value_lhs = values[lhs];
  if (value_lhs > 0) {
    mark_gate_as_garbage (closure, g);
    return true;
  }
  KISSAT_assert (g->arity > 1);
  return false;
}

static bool gate_contains (gate *g, unsigned lit) {
  for (all_rhs_literals_in_gate (other, g))
    if (lit == other)
      return true;
  return false;
}

static bool rewriting_lhs (closure *closure, gate *g, unsigned dst) {
#ifndef KISSAT_NDEBUG
  kissat *const solver = closure->solver;
#endif
  if (dst != g->lhs && dst != NOT (g->lhs))
    return false;
  mark_gate_as_garbage (closure, g);
  return true;
}

static void shrink_and_gate (closure *closure, gate *g,
                             unsigned *new_end_rhs, unsigned falsifies,
                             unsigned clashing) {
  KISSAT_assert (g->tag == AND_GATE);
#ifndef KISSAT_NDEBUG
  kissat *const solver = closure->solver;
#endif
  if (falsifies != INVALID_LIT) {
    KISSAT_assert (g->arity);
    g->rhs[0] = falsifies;
    new_end_rhs = g->rhs + 1;
  } else if (clashing != INVALID_LIT) {
    KISSAT_assert (1 < g->arity);
    g->rhs[0] = clashing;
    g->rhs[1] = NOT (clashing);
    new_end_rhs = g->rhs + 2;
  }
  shrink_gate (closure, g, new_end_rhs);
}

static void update_and_gate (closure *closure, gate *g, unsigned falsifies,
                             unsigned clashing) {
  KISSAT_assert (g->tag == AND_GATE);
  bool garbage = true;
  kissat *const solver = closure->solver;
  if (falsifies != INVALID_LIT || clashing != INVALID_LIT)
    (void) learn_congruence_unit (closure, NOT (g->lhs));
  else if (g->arity == 1) {
    const value value_lhs = VALUE (g->lhs);
    if (value_lhs > 0)
      (void) learn_congruence_unit (closure, g->rhs[0]);
    else if (value_lhs < 0)
      (void) learn_congruence_unit (closure, NOT (g->rhs[0]));
    else if (merge_literals (closure, g->lhs, g->rhs[0])) {
      INC (congruent_unary_ands);
      INC (congruent_unary);
    }
  } else {
    unsigned hash;
    gate *h = find_and_gate (closure, &hash, g);
    if (h) {
      KISSAT_assert (garbage);
      if (merge_literals (closure, g->lhs, h->lhs))
        INC (congruent_ands);
    } else {
      remove_gate (closure, g);
      g->hash = hash;
      index_gate (closure, g);
      garbage = false;
    }
  }
  if (garbage && !solver->inconsistent)
    mark_gate_as_garbage (closure, g);
}

static void simplify_and_gate (closure *closure, gate *g) {
  if (skip_and_gate (closure, g))
    return;
  kissat *const solver = closure->solver;
  const value *const values = solver->values;
  CLOGANDGATE (g, "simplifying");
  const unsigned old_arity = g->arity;
  unsigned *const rhs = g->rhs;
  unsigned *const end_of_rhs = rhs + old_arity;
  const unsigned *p = rhs;
  unsigned *q = rhs;
  unsigned falsifies = INVALID_LIT;
  while (p != end_of_rhs) {
    const unsigned lit = *p++;
    const value value = values[lit];
    if (value > 0)
      continue;
    if (value < 0) {
      LOG ("found falsifying literal %s", LOGLIT (lit));
      falsifies = lit;
      continue;
    }
    *q++ = lit;
  }
  shrink_and_gate (closure, g, q, falsifies, INVALID_LIT);
  CLOGANDGATE (g, "simplified");
  check_and_gate_implied (closure, g);
  update_and_gate (closure, g, falsifies, INVALID_LIT);
  INC (congruent_simplified);
  INC (congruent_simplified_ands);
}

static void rewrite_and_gate (closure *closure, gate *g, unsigned dst,
                              unsigned src) {
  if (skip_and_gate (closure, g))
    return;
  if (!gate_contains (g, src))
    return;
  KISSAT_assert (src != INVALID_LIT);
  KISSAT_assert (dst != INVALID_LIT);
  kissat *const solver = closure->solver;
  const value *const values = solver->values;
  KISSAT_assert (values[src] == values[dst]);
  CLOGANDGATE (g, "rewriting %s by %s in", CLOGREPR (src), CLOGREPR (dst));
  const unsigned old_arity = g->arity;
  const unsigned not_lhs = NOT (g->lhs);
  unsigned *const rhs = g->rhs;
  unsigned *const end_of_rhs = rhs + old_arity;
  const unsigned *p = rhs;
  unsigned *q = rhs;
  unsigned falsifies = INVALID_LIT;
  unsigned clashing = INVALID_LIT;
  const unsigned not_dst = NOT (dst);
  unsigned dst_count = 0, not_dst_count = 0;
  while (p != end_of_rhs) {
    unsigned lit = *p++;
    if (lit == src)
      lit = dst;
    if (lit == not_lhs) {
      LOG ("found negated LHS literal %s", LOGLIT (lit));
      clashing = lit;
      break;
    }
    const value value = values[lit];
    if (value > 0)
      continue;
    if (value < 0) {
      LOG ("found falsifying literal %s", LOGLIT (lit));
      falsifies = lit;
      break;
    }
    if (lit == dst) {
      if (not_dst_count) {
        LOG ("clashing literals %s and %s", LOGLIT (not_dst), LOGLIT (dst));
        clashing = not_dst;
        break;
      }
      if (dst_count++)
        continue;
    }
    if (lit == not_dst) {
      if (dst_count) {
        KISSAT_assert (!not_dst_count);
        LOG ("clashing literals %s and %s", LOGLIT (dst), LOGLIT (not_dst));
        clashing = dst;
        break;
      }
      KISSAT_assert (!not_dst_count);
      not_dst_count++;
    }
    *q++ = lit;
  }
  KISSAT_assert (dst_count <= 2);
  KISSAT_assert (not_dst_count <= 1);
  shrink_and_gate (closure, g, q, falsifies, clashing);
  CLOGANDGATE (g, "rewritten");
  check_and_gate_implied (closure, g);
  update_and_gate (closure, g, falsifies, clashing);
  INC (congruent_rewritten);
  INC (congruent_rewritten_ands);
}

static bool skip_xor_gate (gate *g) {
  KISSAT_assert (g->tag == XOR_GATE);
  if (g->garbage)
    return true;
  KISSAT_assert (g->arity > 1);
  return false;
}

#ifdef CHECKING_OR_PROVING

static void add_xor_shrinking_proof_chain (closure *closure, gate *g,
                                           unsigned pivot) {
  kissat *const solver = closure->solver;
  if (!kissat_checking_or_proving (solver))
    return;
  LOG ("starting XOR shrinking proof chain");
  unsigneds *clause = &solver->clause;
  KISSAT_assert (EMPTY_STACK (*clause));
  for (unsigned i = 0; i != g->arity; i++) {
    unsigned lit = g->rhs[i];
    PUSH_STACK (*clause, lit);
  }
  const unsigned lhs = g->lhs;
  const unsigned not_lhs = NOT (lhs);
  PUSH_STACK (*clause, not_lhs);
  const unsigned parity = NEGATED (not_lhs);
  KISSAT_assert (parity == parity_lits (solver, clause));
  const unsigned not_pivot = NOT (pivot);
  const size_t size = SIZE_STACK (*clause);
  KISSAT_assert (size < 32);
  const unsigned end = 1u << size;
  for (unsigned i = 0; i != end; i++) {
    while (i && parity != parity_lits (solver, clause))
      inc_lits (solver, clause);
    PUSH_STACK (*clause, pivot);
    CHECK_AND_ADD_STACK (*clause);
    ADD_STACK_TO_PROOF (*clause);
    clause->end--;
    PUSH_STACK (*clause, not_pivot);
    CHECK_AND_ADD_STACK (*clause);
    ADD_STACK_TO_PROOF (*clause);
    clause->end--;
    CHECK_AND_ADD_STACK (*clause);
    ADD_STACK_TO_PROOF (*clause);
    PUSH_STACK (*clause, pivot);
    REMOVE_CHECKER_STACK (*clause);
    DELETE_STACK_FROM_PROOF (*clause);
    clause->end--;
    PUSH_STACK (*clause, not_pivot);
    REMOVE_CHECKER_STACK (*clause);
    DELETE_STACK_FROM_PROOF (*clause);
    clause->end--;
    inc_lits (solver, clause);
  }
  CLEAR_STACK (*clause);
  LOG ("finished XOR shrinking proof chain");
}

#else

#define add_xor_shrinking_proof_chain(...) \
  do { \
  } while (0)
#endif

static void shrink_xor_gate (closure *closure, gate *g,
                             unsigned *new_end_rhs) {
  KISSAT_assert (g->tag == XOR_GATE);
  shrink_gate (closure, g, new_end_rhs);
}

static void update_xor_gate (closure *closure, gate *g) {
  KISSAT_assert (g->tag == XOR_GATE);
  kissat *const solver = closure->solver;
  bool garbage = true;
  if (g->arity == 0)
    (void) learn_congruence_unit (closure, NOT (g->lhs));
  else if (g->arity == 1) {
    const value value_lhs = VALUE (g->lhs);
    if (value_lhs > 0)
      (void) learn_congruence_unit (closure, g->rhs[0]);
    else if (value_lhs < 0)
      (void) learn_congruence_unit (closure, NOT (g->rhs[0]));
    else if (merge_literals (closure, g->lhs, g->rhs[0])) {
      INC (congruent_unary_xors);
      INC (congruent_unary);
    }
  } else {
    KISSAT_assert (g->arity > 1);
    unsigned hash;
    gate *h = find_xor_gate (closure, &hash, g);
    if (h) {
      KISSAT_assert (garbage);
      add_xor_matching_proof_chain (closure, g, g->lhs, h->lhs);
      if (merge_literals (closure, g->lhs, h->lhs))
        INC (congruent_xors);
      if (!solver->inconsistent)
        delete_proof_chain (closure);
    } else {
      remove_gate (closure, g);
      g->hash = hash;
      index_gate (closure, g);
      garbage = false;
    }
  }
  if (garbage && !solver->inconsistent)
    mark_gate_as_garbage (closure, g);
}

static void simplify_xor_gate (closure *closure, gate *g) {
  if (skip_xor_gate (g))
    return;
  kissat *const solver = closure->solver;
  const value *const values = solver->values;
  CLOGXORGATE (g, "simplifying");
  unsigned *q = g->rhs, *const end_of_rhs = q + g->arity;
  unsigned negate = 0;
  for (const unsigned *p = q; p != end_of_rhs; p++) {
    const unsigned lit = *p;
    KISSAT_assert (!NEGATED (lit));
    const value value = values[lit];
    if (value > 0)
      negate ^= 1;
    if (!value)
      *q++ = lit;
  }
  if (negate) {
    LOG ("flipping LHS literal %s", LOGLIT (g->lhs));
    g->lhs = NOT (g->lhs);
  }
  shrink_xor_gate (closure, g, q);
  update_xor_gate (closure, g);
  CLOGXORGATE (g, "simplified");
  check_xor_gate_implied (closure, g);
  INC (congruent_simplified);
  INC (congruent_simplified_xors);
}

static void rewrite_xor_gate (closure *closure, gate *g, unsigned dst,
                              unsigned src) {
  if (skip_xor_gate (g))
    return;
  if (rewriting_lhs (closure, g, dst))
    return;
  if (!gate_contains (g, src))
    return;
  kissat *const solver = closure->solver;
  CLOGXORGATE (g, "rewriting %s by %s in", CLOGREPR (src), CLOGREPR (dst));
  const value *const values = solver->values;
  unsigned *q = g->rhs, *end_of_rhs = q + g->arity;
  unsigned original_dst_negated = NEGATED (dst);
  unsigned negate = original_dst_negated;
  unsigned dst_count = 0;
  dst = STRIP (dst);
  for (const unsigned *p = q; p != end_of_rhs; p++) {
    unsigned lit = *p;
    KISSAT_assert (!NEGATED (lit));
    if (lit == src)
      lit = dst;
    const value value = values[lit];
    if (value > 0)
      negate ^= 1;
    if (value)
      continue;
    if (lit == dst)
      dst_count++;
    *q++ = lit;
  }
  if (negate) {
    LOG ("flipping LHS literal %s", LOGLIT (g->lhs));
    g->lhs = NOT (g->lhs);
  }
  KISSAT_assert (dst_count <= 2);
  if (dst_count == 2) {
    CLOGXORGATE (g, "literals %s and %s were both in", LOGLIT (src),
                 LOGLIT (dst));
    end_of_rhs = q;
    q = g->rhs;
    for (const unsigned *p = q; p != end_of_rhs; p++) {
      const unsigned lit = *p;
      if (lit != dst)
        *q++ = lit;
    }
    KISSAT_assert (q + 2 == end_of_rhs);
  }
  shrink_xor_gate (closure, g, q);
  CLOGXORGATE (g, "rewritten");
  if (dst_count > 1)
    add_xor_shrinking_proof_chain (closure, g, src);
  update_xor_gate (closure, g);
  if (!g->garbage && !solver->inconsistent && original_dst_negated &&
      dst_count == 1) {
    KISSAT_assert (!NEGATED (dst));
    connect_occurrence (closure, dst, g);
  }
  check_xor_gate_implied (closure, g);
  INC (congruent_rewritten);
  INC (congruent_rewritten_xors);
}

static bool skip_ite_gate (gate *g) {
  KISSAT_assert (g->tag == ITE_GATE);
  if (g->garbage)
    return true;
  return false;
}

static void simplify_ite_gate (closure *closure, gate *g) {
  if (skip_ite_gate (g))
    return;
  kissat *const solver = closure->solver;
  const value *const values = solver->values;
  CLOGITEGATE (g, "simplifying");
  KISSAT_assert (g->arity == 3);
  bool garbage = true;
  const unsigned lhs = g->lhs;
  unsigned *const rhs = g->rhs;
  const unsigned cond = rhs[0];
  const unsigned then_lit = rhs[1];
  const unsigned else_lit = rhs[2];
  const value cond_value = values[cond];
  if (cond_value > 0) {
    if (merge_literals (closure, lhs, then_lit)) {
      INC (congruent_unary_ites);
      INC (congruent_unary);
    }
  } else if (cond_value < 0) {
    if (merge_literals (closure, lhs, else_lit)) {
      INC (congruent_unary_ites);
      INC (congruent_unary);
    }
  } else {
    const value then_value = values[then_lit];
    const value else_value = values[else_lit];
    const unsigned not_lhs = NOT (lhs);
    KISSAT_assert (then_value || else_value);
    if (then_value > 0 && else_value > 0)
      learn_congruence_unit (closure, lhs);
    else if (then_value < 0 && else_value < 0)
      learn_congruence_unit (closure, not_lhs);
    else if (then_value > 0 && else_value < 0) {
      if (merge_literals (closure, lhs, cond)) {
        INC (congruent_unary_ites);
        INC (congruent_unary);
      }
    } else if (then_value < 0 && else_value > 0) {
      const unsigned not_cond = NOT (cond);
      if (merge_literals (closure, lhs, not_cond)) {
        INC (congruent_unary_ites);
        INC (congruent_unary);
      }
    } else {
      KISSAT_assert (!!else_value + !!then_value == 1);
      if (then_value > 0) {
        KISSAT_assert (!else_value);
        g->lhs = not_lhs;
        rhs[0] = NOT (cond);
        rhs[1] = NOT (else_lit);
      } else if (then_value < 0) {
        KISSAT_assert (!else_value);
        rhs[0] = NOT (cond);
        rhs[1] = else_lit;
      } else if (else_value > 0) {
        KISSAT_assert (!then_value);
        g->lhs = not_lhs;
        rhs[0] = NOT (then_lit);
        rhs[1] = cond;
      } else {
        KISSAT_assert (else_value < 0);
        KISSAT_assert (!then_value);
        rhs[0] = cond;
        rhs[1] = then_lit;
      }
      if (rhs[0] > rhs[1])
        SWAP (unsigned, rhs[0], rhs[1]);
      KISSAT_assert (!g->shrunken);
      g->shrunken = true;
      rhs[2] = INVALID_LIT;
      g->arity = 2;
      g->tag = AND_GATE;
      KISSAT_assert (rhs[0] < rhs[1]);
      KISSAT_assert (rhs[0] != NOT (rhs[1]));
      CLOGANDGATE (g, "simplified");
      check_and_gate_implied (closure, g);
      unsigned hash;
      gate *h = find_and_gate (closure, &hash, g);
      if (h) {
        KISSAT_assert (garbage);
        if (merge_literals (closure, g->lhs, h->lhs))
          INC (congruent_ands);
      } else {
        remove_gate (closure, g);
        g->hash = hash;
        index_gate (closure, g);
        garbage = false;
        for (all_rhs_literals_in_gate (lit, g))
          if (lit != cond && lit != then_lit && lit != else_lit)
            connect_occurrence (closure, lit, g);
      }
    }
  }
  if (garbage && !solver->inconsistent)
    mark_gate_as_garbage (closure, g);
  INC (congruent_simplified);
  INC (congruent_simplified_ites);
}

static void rewrite_ite_gate (closure *closure, gate *g, unsigned dst,
                              unsigned src) {
  if (skip_ite_gate (g))
    return;
  if (!gate_contains (g, src))
    return;
  kissat *const solver = closure->solver;
  CLOGITEGATE (g, "rewriting %s by %s in", CLOGREPR (src), CLOGREPR (dst));
  unsigned *const rhs = g->rhs;
  KISSAT_assert (g->arity == 3);
  const unsigned lhs = g->lhs;
  const unsigned cond = rhs[0];
  const unsigned then_lit = rhs[1];
  const unsigned else_lit = rhs[2];
  const unsigned not_lhs = NOT (lhs);
  const unsigned not_dst = NOT (dst);
  const unsigned not_cond = NOT (cond);
  const unsigned not_then_lit = NOT (then_lit);
  const unsigned not_else_lit = NOT (else_lit);
  unsigned new_tag = AND_GATE;
  bool garbage = false;
  bool shrink = true;
  if (src == cond) {
    if (dst == then_lit) {
      // then_lit ? then_lit : else_lit
      // then_lit & then_lit | !then_lit & else_lit
      // then_lit | !then_lit & else_lit
      // then_lit | else_lit
      // !(!then_lit & !else_lit)
      g->lhs = not_lhs;
      rhs[0] = not_then_lit;
      rhs[1] = not_else_lit;
    } else if (not_dst == then_lit) {
      // !then_lit ? then_lit : else_lit
      // !then_lit & then_lit | then_lit & else_lit
      // then_lit & else_lit
      rhs[0] = else_lit;
      KISSAT_assert (rhs[1] == then_lit);
    } else if (dst == else_lit) {
      // else_list ? then_lit : else_lit
      // else_list & then_lit | !else_list & else_lit
      // else_list & then_lit
      rhs[0] = else_lit;
      KISSAT_assert (rhs[1] == then_lit);
    } else if (not_dst == else_lit) {
      // !else_list ? then_lit : else_lit
      // !else_list & then_lit | else_lit & else_lit
      // !else_list & then_lit | else_lit
      // then_lit | else_lit
      // !(!then_lit & !else_lit)
      g->lhs = not_lhs;
      rhs[0] = not_then_lit;
      rhs[1] = not_else_lit;
    } else {
      shrink = false;
      rhs[0] = dst;
    }
  } else if (src == then_lit) {
    if (dst == cond) {
      // cond ? cond : else_lit
      // cond & cond | !cond & else_lit
      // cond | !cond & else_lit
      // cond | else_lit
      // !(!cond & !else_lit)
      g->lhs = not_lhs;
      rhs[0] = not_cond;
      rhs[1] = not_else_lit;
    } else if (not_dst == cond) {
      // cond ? !cond : else_lit
      // cond & !cond | !cond & else_lit
      // !cond & else_lit
      rhs[0] = not_cond;
      rhs[1] = else_lit;
    } else if (dst == else_lit) {
      // cond ? else_lit : else_lit
      // else_lit
      if (merge_literals (closure, lhs, else_lit)) {
        INC (congruent_unary_ites);
        INC (congruent_unary);
      }
      garbage = true;
    } else if (not_dst == else_lit) {
      // cond ? !else_lit : else_lit
      // cond & !else_lit | !cond & else_lit
      // cond ^ else_lit
      new_tag = XOR_GATE;
      KISSAT_assert (rhs[0] == cond);
      rhs[1] = else_lit;
    } else {
      shrink = false;
      rhs[1] = dst;
    }
  } else {
    KISSAT_assert (src == else_lit);
    if (dst == cond) {
      // cond ? then_lit : cond
      // cond & then_lit | !cond & cond
      // cond & then_lit
      KISSAT_assert (rhs[0] == cond);
      KISSAT_assert (rhs[1] == then_lit);
    } else if (not_dst == cond) {
      // cond ? then_lit : !cond
      // cond & then_lit | !cond & !cond
      // cond & then_lit | !cond
      // then_lit | !cond
      // !(!then_lit & cond)
      g->lhs = not_lhs;
      KISSAT_assert (rhs[0] == cond);
      rhs[1] = not_then_lit;
    } else if (dst == then_lit) {
      // cond ? then_lit : then_lit
      // then_lit
      if (merge_literals (closure, lhs, then_lit)) {
        INC (congruent_unary_ites);
        INC (congruent_unary);
      }
      garbage = true;
    } else if (not_dst == then_lit) {
      // cond ? then_lit : !then_lit
      // cond & then_lit | !cond & !then_lit
      // !(cond ^ then_lit)
      new_tag = XOR_GATE;
      g->lhs = not_lhs;
      KISSAT_assert (rhs[0] == cond);
      KISSAT_assert (rhs[1] == then_lit);
    } else {
      shrink = false;
      rhs[2] = dst;
    }
  }
  if (!garbage) {
    if (shrink) {
      if (rhs[0] > rhs[1])
        SWAP (unsigned, rhs[0], rhs[1]);
      if (new_tag == XOR_GATE) {
        bool negate_lhs = false;
        if (NEGATED (rhs[0])) {
          rhs[0] = NOT (rhs[0]);
          negate_lhs = !negate_lhs;
        }
        if (NEGATED (rhs[1])) {
          rhs[1] = NOT (rhs[1]);
          negate_lhs = !negate_lhs;
        }
        if (negate_lhs)
          g->lhs = NOT (g->lhs);
      }
      KISSAT_assert (!g->shrunken);
      g->shrunken = true;
      rhs[2] = INVALID_LIT;
      g->arity = 2;
      g->tag = new_tag;
      KISSAT_assert (rhs[0] < rhs[1]);
      KISSAT_assert (rhs[0] != NOT (rhs[1]));
      LOGATE (g, "rewritten");
      gate *h;
      unsigned hash;
      if (new_tag == AND_GATE) {
        check_and_gate_implied (closure, g);
        h = find_and_gate (closure, &hash, g);
      } else {
        KISSAT_assert (new_tag == XOR_GATE);
        check_xor_gate_implied (closure, g);
        h = find_xor_gate (closure, &hash, g);
      }
      if (h) {
        garbage = true;
        if (new_tag == XOR_GATE)
          add_xor_matching_proof_chain (closure, g, g->lhs, h->lhs);
        else
          add_ite_turned_and_binary_clauses (closure, g);
        if (merge_literals (closure, g->lhs, h->lhs))
          INC (congruent_ands);
        if (!solver->inconsistent)
          delete_proof_chain (closure);
      } else {
        garbage = false;
        remove_gate (closure, g);
        g->hash = hash;
        index_gate (closure, g);
        KISSAT_assert (g->arity == 2);
        for (all_rhs_literals_in_gate (lit, g))
          if (lit != dst)
            if (lit != cond && lit != then_lit && lit != else_lit)
              connect_occurrence (closure, lit, g);
        if (g->tag == AND_GATE)
          for (all_rhs_literals_in_gate (lit, g))
            add_binary_clause (closure, NOT (g->lhs), lit);
      }
    } else {
      CLOGITEGATE (g, "rewritten");
      KISSAT_assert (rhs[0] != rhs[1]);
      KISSAT_assert (rhs[0] != rhs[2]);
      KISSAT_assert (rhs[1] != rhs[2]);
      KISSAT_assert (rhs[0] != NOT (rhs[1]));
      KISSAT_assert (rhs[0] != NOT (rhs[2]));
      KISSAT_assert (rhs[1] != NOT (rhs[2]));
      check_ite_gate_implied (closure, g);
      unsigned hash;
      bool negate_lhs;
      gate *h = find_ite_gate (closure, &hash, &negate_lhs, g);
      KISSAT_assert (lhs == g->lhs);
      KISSAT_assert (not_lhs == NOT (g->lhs));
      if (h) {
        garbage = true;
        unsigned normalized_lhs = negate_lhs ? not_lhs : lhs;
        add_ite_matching_proof_chain (closure, h, h->lhs, normalized_lhs);
        if (merge_literals (closure, h->lhs, normalized_lhs))
          INC (congruent_ites);
        if (!solver->inconsistent)
          delete_proof_chain (closure);
      } else {
        garbage = false;
        remove_gate (closure, g);
        if (negate_lhs)
          g->lhs = not_lhs;
        CLOGITEGATE (g, "normalized");
        g->hash = hash;
        index_gate (closure, g);
        KISSAT_assert (g->arity == 3);
        for (all_rhs_literals_in_gate (lit, g))
          if (lit != dst)
            if (lit != cond && lit != then_lit && lit != else_lit)
              connect_occurrence (closure, lit, g);
      }
    }
  }
  if (garbage && !solver->inconsistent)
    mark_gate_as_garbage (closure, g);
  INC (congruent_rewritten);
  INC (congruent_rewritten_ites);
}

static bool simplify_gate (closure *closure, gate *g) {
  if (g->tag == AND_GATE)
    simplify_and_gate (closure, g);
  else if (g->tag == XOR_GATE)
    simplify_xor_gate (closure, g);
  else
    simplify_ite_gate (closure, g);
  return !closure->solver->inconsistent;
}

static bool rewrite_gate (closure *closure, gate *g, unsigned dst,
                          unsigned src) {
  if (g->tag == AND_GATE)
    rewrite_and_gate (closure, g, dst, src);
  else if (g->tag == XOR_GATE)
    rewrite_xor_gate (closure, g, dst, src);
  else
    rewrite_ite_gate (closure, g, dst, src);
  return !closure->solver->inconsistent;
}

struct offsetsize {
  unsigned offset, size;
};

typedef struct offsetsize offsetsize;

#define RANK_OTHER(A) ((A).lits[1])
#define LESS_OTHER(A, B) (RANK_OTHER (A) < RANK_OTHER (B))

static bool find_binary (kissat *solver, litpair *binaries,
                         offsetsize *offsetsize, unsigned lit,
                         unsigned other) {
  KISSAT_assert (lit != other);
  if (lit > other)
    SWAP (unsigned, lit, other);
  size_t l = offsetsize[lit].offset;
  size_t r = l + offsetsize[lit].size;
  while (l < r) {
    const size_t m = (l + r) / 2;
    const unsigned tmp = binaries[m].lits[1];
    if (tmp < other)
      l = m + 1;
    else if (tmp > other)
      r = m;
    else {
      KISSAT_assert (binaries[m].lits[0] == lit);
      KISSAT_assert (binaries[m].lits[1] == other);
#ifdef LOGGING
      LOGBINARY (lit, other, "found");
#else
      (void) solver;
#endif
      return true;
    }
  }
  return false;
}

static uint64_t rank_litpair (litpair p) {
  uint64_t res = p.lits[0];
  res <<= 32;
  res += p.lits[1];
  return res;
}

static void extract_binaries (closure *closure) {
  kissat *const solver = closure->solver;
  if (!GET_OPTION (congruencebinaries))
    return;
  START (extractbinaries);
  litpair *binaries = BEGIN_STACK (closure->binaries);
  offsetsize *offsetsize;
  CALLOC (struct offsetsize, offsetsize, LITS);
  {
    litpair *end = END_STACK (closure->binaries);
    litpair *p = binaries;
    while (p != end) {
      litpair *q = p + 1;
      const unsigned lit = p->lits[0];
      while (q != end && q->lits[0] == lit)
        q++;
      const size_t size = q - p;
      KISSAT_assert (size), KISSAT_assert (size <= UINT_MAX);
      const size_t offset = p - binaries;
      if (size < 32)
        SORT (litpair, size, p, LESS_OTHER);
      else
        RADIX_SORT (litpair, unsigned, size, p, RANK_OTHER);
      offsetsize[lit].offset = offset;
      offsetsize[lit].size = size;
      p = q;
    }
  }
  clause *last_irredundant = kissat_last_irredundant_clause (solver);
  const size_t before = SIZE_STACK (closure->binaries);
  size_t extracted = 0, duplicated = 0;
  const value *const values = solver->values;
  for (all_clauses (d)) {
    if (d->garbage)
      continue;
    if (last_irredundant && last_irredundant < d)
      break;
    if (d->redundant)
      continue;
    if (d->size != 3)
      continue;
    const unsigned *lits = d->lits;
    const unsigned a = lits[0];
    if (values[a])
      continue;
    const unsigned b = lits[1];
    if (values[b])
      continue;
    const unsigned c = lits[2];
    if (values[c])
      continue;
    const unsigned not_a = NOT (a);
    const unsigned not_b = NOT (b);
    const unsigned not_c = NOT (c);
    unsigned l = INVALID_LIT, k = INVALID_LIT;
    if (find_binary (solver, binaries, offsetsize, not_a, b) ||
        find_binary (solver, binaries, offsetsize, not_a, c))
      l = b, k = c;
    else if (find_binary (solver, binaries, offsetsize, not_b, a) ||
             find_binary (solver, binaries, offsetsize, not_b, c))
      l = a, k = c;
    else if (find_binary (solver, binaries, offsetsize, not_c, a) ||
             find_binary (solver, binaries, offsetsize, not_c, b))
      l = a, k = b;
    else
      continue;
    LOGCLS (d, "strengthening");
    if (!find_binary (solver, binaries, offsetsize, l, k)) {
      LOGBINARY (l, k, "strengthened");
      add_binary_clause (closure, l, k);
      binaries = BEGIN_STACK (closure->binaries);
      extracted++;
    }
  }
  DEALLOC (offsetsize, LITS);
  {
    litpair *end = END_STACK (closure->binaries);
    litpair *added = binaries + before;
#ifndef KISSAT_NDEBUG
    const size_t after = end - binaries;
    KISSAT_assert (after - before == extracted);
#endif
    RADIX_SORT (litpair, uint64_t, extracted, added, rank_litpair);
    litpair *q = added;
    unsigned prev_lit = INVALID_LIT;
    unsigned prev_other = INVALID_LIT;
    for (const litpair *p = q; p != end; p++) {
      const litpair pair = *p;
      const unsigned lit = pair.lits[0];
      const unsigned other = pair.lits[1];
      if (p == added || lit != prev_lit || other != prev_other) {
        q->lits[0] = lit;
        q->lits[1] = other;
        prev_lit = lit;
        prev_other = other;
        q++;
      } else {
        duplicated++;
        LOGBINARY (lit, other, "removing duplicated");
        kissat_delete_binary (solver, lit, other);
      }
    }
    SET_END_OF_STACK (closure->binaries, q);
  }
  ADD (congruent_binaries, extracted - duplicated);
  kissat_verbose (solver, "extracted %zu binaries (plus %zu duplicated)",
                  extracted, duplicated);
  STOP (extractbinaries);
}

#ifndef INDEX_BINARY_CLAUSES

static gate *find_first_and_gate (closure *closure, unsigned lhs,
                                  unsigneds *lits) {
  kissat *const solver = closure->solver;
  KISSAT_assert (!solver->watching);
  mark *const marks = solver->marks;

  const unsigned not_lhs = NOT (lhs);
  LOG ("trying to find AND gate with first LHS %s", LOGLIT (lhs));
  LOG ("negated LHS %s occurs in %u binary clauses", LOGLIT (not_lhs),
       closure->negbincount[lhs]);

  unsigneds *const marked = &solver->analyzed;
  KISSAT_assert (EMPTY_STACK (*marked));

  const unsigned arity = SIZE_STACK (*lits) - 1;
  unsigned matched = 0;
  KISSAT_assert (1 < arity);

  watches *watches = &WATCHES (not_lhs);
  const watch *const end = END_WATCHES (*watches);
  const watch *p = BEGIN_WATCHES (*watches);

  while (p != end) {
    const watch watch = *p++;
    KISSAT_assert (watch.type.binary);
    const unsigned other = watch.binary.lit;
    const mark tmp = marks[other];
    if (tmp) {
      matched++;
      KISSAT_assert (~(tmp & 2));
      marks[other] |= 2;
      PUSH_STACK (*marked, other);
    }
  }

  LOG ("found %zu initial LHS candidates", SIZE_STACK (*marked));
  if (matched < arity)
    return 0;

  return new_and_gate (closure, lhs);
}

static gate *find_remaining_and_gate (closure *closure, unsigned lhs,
                                      unsigneds *lits) {
  kissat *const solver = closure->solver;
  KISSAT_assert (!solver->watching);
  mark *const marks = solver->marks;
  const unsigned not_lhs = NOT (lhs);

  if (marks[not_lhs] < 2) {
    LOG ("skipping no-candidate LHS %s", LOGLIT (lhs));
    return NULL;
  }

  LOG ("trying to find AND gate with remaining LHS %s", LOGLIT (lhs));
  LOG ("negated LHS %s occurs times in %u binary clauses", LOGLIT (not_lhs),
       closure->negbincount[lhs]);

  const unsigned arity = SIZE_STACK (*lits) - 1;
  unsigned matched = 0;
  KISSAT_assert (1 < arity);

  {
    watches *watches = &WATCHES (not_lhs);
    const watch *const end_watches = END_WATCHES (*watches);
    const watch *p = BEGIN_WATCHES (*watches);
    while (p != end_watches) {
      const watch watch = *p++;
      KISSAT_assert (watch.type.binary);
      const unsigned other = watch.binary.lit;
      mark mark = marks[other];
      if (!mark)
        continue;
      matched++;
      if (!(mark & 2))
        continue;
      KISSAT_assert (!(mark & 4));
      marks[other] = mark | 4;
    }
  }

  {
    unsigneds *const marked = &solver->analyzed;
    KISSAT_assert (!EMPTY_STACK (*marked));
    unsigned *const begin_marked = BEGIN_STACK (*marked);
    const unsigned *const end_marked = END_STACK (*marked);
    unsigned *q = begin_marked;
    const unsigned *p = q;
    KISSAT_assert (marks[not_lhs] == 3);
    while (p != end_marked) {
      const unsigned lit = *p++;
      if (lit == not_lhs) {
        marks[not_lhs] = 1;
        continue;
      }
      mark mark = marks[lit];
      KISSAT_assert ((mark & 3) == 3);
      if (mark & 4) {
        mark = 3;
        *q++ = lit;
        LOG2 ("keeping LHS candidate %s", LOGLIT (NOT (lit)));
      } else {
        LOG2 ("dropping LHS candidate %s", LOGLIT (NOT (lit)));
        mark = 1;
      }
      marks[lit] = mark;
    }
    KISSAT_assert (q != end_marked);
    KISSAT_assert (marks[not_lhs] == 1);
    SET_END_OF_STACK (*marked, q);
    LOG ("after filtering %zu LHS candidates remain", SIZE_STACK (*marked));
  }

  if (matched < arity)
    return 0;

  return new_and_gate (closure, lhs);
}

#endif

static inline bool smaller_negated_bin_count (const unsigned *negbincount,
                                              unsigned a, unsigned b) {
  unsigned c = negbincount[a];
  unsigned d = negbincount[b];
  if (c < d)
    return true;
  if (c > d)
    return false;
  return a < b;
}

#define SMALLER_NEGATED_BIN_COUNT(A, B) \
  smaller_negated_bin_count (negbincount, A, B)

static void sort_lits_by_negbincount (closure *closure, size_t size,
                                      unsigned *lits) {
  const unsigned *const negbincount = closure->negbincount;
  kissat *const solver = closure->solver;
  SORT (unsigned, size, lits, SMALLER_NEGATED_BIN_COUNT);
}

#ifdef INDEX_BINARY_CLAUSES

static unsigned hash_binary (closure *closure, binary_clause *binary) {
  return hash_lits (closure, 0, 2, binary->lits);
}

static bool indexed_binary (closure *closure, unsigned lit,
                            unsigned other) {
  KISSAT_assert (lit != other);
#ifdef LOGGING
  kissat *const solver = closure->solver;
#endif
  binary_hash_table *bintab = &closure->bintab;
  if (!bintab->count) {
    LOG ("did not find binary %s %s", LOGLIT (lit), LOGLIT (other));
    return false;
  }
  KISSAT_assert (bintab->size);
  SWAP (unsigned, lit, other);
  if (lit > other)
    SWAP (unsigned, lit, other);
  binary_clause binary = {.lits = {lit, other}};
  const unsigned hash = hash_binary (closure, &binary);
  const size_t size = bintab->size;
  const size_t size2 = bintab->size2;
  size_t pos = reduce_hash (hash, size, size2);
  binary_clause *table = bintab->table;
  unsigned lit0, lit1;
  while ((lit1 = table[pos].lits[1])) {
    if (lit1 == other) {
      lit0 = table[pos].lits[0];
      KISSAT_assert (lit0 < other);
      if (lit0 == lit) {
        LOG ("found binary %s %s", LOGLIT (lit), LOGLIT (other));
        return true;
      }
    }
    if (++pos == size)
      pos = 0;
  }
  LOG ("did not find binary %s %s", LOGLIT (lit), LOGLIT (other));
  return false;
}

#endif

static void extract_and_gates_with_base_clause (closure *closure,
                                                clause *c) {
  KISSAT_assert (!c->garbage);
  kissat *const solver = closure->solver;
  KISSAT_assert (!solver->inconsistent);
  value *values = solver->values;
  unsigned arity_limit = MIN (GET_OPTION (congruenceandarity), MAX_ARITY);
  const unsigned size_limit = arity_limit + 1;
  const unsigned *const negbincount = closure->negbincount;
  unsigneds *lits = &closure->lits;
  unsigned size = 0, max_negbincount = 0;
  CLEAR_STACK (*lits);
  for (all_literals_in_clause (lit, c)) {
    value value = values[lit];
    if (value < 0)
      continue;
    if (value > 0) {
      KISSAT_assert (!solver->level);
      LOGCLS (c, "found satisfied %s in", LOGLIT (lit));
      kissat_mark_clause_as_garbage (solver, c);
      return;
    }
    if (++size > size_limit) {
      LOGCLS (c, "too large actual size %u thus skipping", size);
      return;
    }
    const unsigned count = negbincount[lit];
    if (!count) {
      LOGCLS (c,
              "%s negated does not occur in any binary clause "
              "thus skipping",
              LOGLIT (lit));
      return;
    }
    if (count > max_negbincount)
      max_negbincount = count;
    PUSH_STACK (*lits, lit);
  }
  if (size < 3) {
    LOGCLS (c, "actual size %u too small thus skipping", size);
    return;
  }
  const unsigned arity = size - 1;
  if (max_negbincount < arity) {
    LOGCLS (c,
            "all literals have less than %u negated occurrences "
            "thus skipping",
            arity);
    return;
  }
  unsigned *begin_lits = BEGIN_STACK (*lits), *reduced_lits = begin_lits;
  LOGCOUNTEDLITS (size, begin_lits, negbincount,
                  "counted candidate arity %u AND gate base clause", arity);
  const unsigned *const end_lits = END_STACK (*lits);
#ifndef INDEX_BINARY_CLAUSES
  mark *const marks = solver->marks;
  unsigneds *marked = &solver->analyzed;
  KISSAT_assert (EMPTY_STACK (*marked));
#endif
  for (unsigned *p = begin_lits; p != end_lits; p++) {
    const unsigned lit = *p, count = negbincount[lit];
#ifndef INDEX_BINARY_CLAUSES
    const unsigned not_lit = NOT (lit);
    marks[not_lit] = 1;
#endif
    if (count < arity) {
      if (reduced_lits < p)
        *p = *reduced_lits, *reduced_lits++ = lit;
      else if (reduced_lits == p)
        reduced_lits++;
    }
  }
  KISSAT_assert (reduced_lits < end_lits);
  const size_t reduced_size = end_lits - reduced_lits;
  KISSAT_assert (reduced_size);
  LOGCLS (c, "trying as base arity %u AND gate", arity);
  sort_lits_by_negbincount (closure, reduced_size, reduced_lits);
#ifdef LOGGING
  if (begin_lits < reduced_lits) {
    LOGCOUNTEDLITS (reduced_lits - begin_lits, begin_lits, negbincount,
                    "skipping low occurrence");
    LOGCOUNTEDLITS (reduced_size, reduced_lits, negbincount,
                    "remaining LHS candidate");
  } else
    LOGCOUNTEDLITS (reduced_size, reduced_lits, negbincount,
                    "all remain LHS candidate");
#endif
#ifdef LOGGING
  unsigned extracted = 0;
#endif
#ifndef INDEX_BINARY_CLAUSES
  bool first = true;
#endif
  for (unsigned *p = reduced_lits; p != end_lits; p++) {
    if (solver->inconsistent)
      break;
    if (c->garbage)
      break;
    const unsigned lhs = *p;
    LOG ("trying LHS candidate literal %s with %u negated occurrences",
         LOGLIT (lhs), negbincount[lhs]);
    KISSAT_assert (arity <= negbincount[lhs]);
#ifdef INDEX_BINARY_CLAUSES
    const unsigned not_lhs = NOT (lhs);
    for (const unsigned *q = begin_lits; q != end_lits; q++)
      if (p != q) {
        const unsigned rhs = *q, not_rhs = NOT (rhs);
        if (!indexed_binary (closure, not_lhs, not_rhs))
          goto CONTINUE_WITH_NEXT_LHS;
      }
    (void) new_and_gate (closure, lhs);
#ifdef LOGGING
    extracted++;
#endif
  CONTINUE_WITH_NEXT_LHS:;
#else
    if (first) {
      first = false;
      KISSAT_assert (EMPTY_STACK (*marked));
      if (find_first_and_gate (closure, lhs, lits)) {
#ifdef LOGGING
        extracted++;
#endif
      }
    } else if (EMPTY_STACK (*marked)) {
      LOG ("early abort AND gate search");
      break;
    } else if (find_remaining_and_gate (closure, lhs, lits)) {
#ifdef LOGGING
      extracted++;
#endif
    }
#endif
  }
#ifndef INDEX_BINARY_CLAUSES
  for (const unsigned *p = begin_lits; p != end_lits; p++) {
    const unsigned lit = *p, not_lit = NOT (lit);
    marks[not_lit] = 0;
  }
  CLEAR_STACK (*marked);
#endif
#ifdef LOGGING
  if (extracted)
    LOGCLS (c, "extracted %u with arity %u AND base", extracted, arity);
#endif
}

#ifdef INDEX_LARGE_CLAUSES

static bool valid_large_clause (hash_ref *clause) {
  return clause->hash || clause->ref;
}

static clause *find_indexed_large_clause (closure *closure,
                                          unsigneds *lits) {
  kissat *const solver = closure->solver;
  size_t size_lits = SIZE_STACK (*lits);
  KISSAT_assert (size_lits > 2);
#ifdef LOGGING
  {
    unsigned *begin = BEGIN_STACK (*lits);
    unsigned arity = size_lits - 1;
    LOGCOUNTEDLITS (size_lits, begin, closure->largecount,
                    "trying to find arity %u XOR side clause", arity);
  }
#endif
  large_clause_hash_table *clauses = &closure->clauses;
  if (!clauses->count)
    return 0;
  const value *const values = solver->values;
  mark *const marks = solver->marks;
  unsigneds *sorted = &solver->clause;
  KISSAT_assert (EMPTY_STACK (*sorted));
  for (all_stack (unsigned, lit, *lits)) {
    KISSAT_assert (!values[lit]);
    PUSH_STACK (*sorted, lit);
    marks[lit] = 1;
  }
  KISSAT_assert (size_lits == SIZE_STACK (*sorted));
  unsigned *begin_sorted = BEGIN_STACK (*sorted);
  sort_lits (solver, size_lits, begin_sorted);
  const unsigned hash = hash_lits (closure, 0, size_lits, begin_sorted);
  const size_t hash_size = clauses->size;
  const size_t hash_size2 = clauses->size2;
  size_t pos = reduce_hash (hash, hash_size, hash_size2);
  hash_ref *table = clauses->table, *hash_ref;
  clause *res = 0;
  while (valid_large_clause (hash_ref = table + pos)) {
    if (hash_ref->hash == hash) {
      reference ref = hash_ref->ref;
      if (ref == INVALID_REF) {
        KISSAT_assert (!hash);
        ref = 0;
      }
      clause *c = kissat_dereference_clause (solver, ref);
      for (all_literals_in_clause (other, c))
        if (!values[other] && !marks[other])
          goto CONTINUE_WITH_NEXT_HASH_BUCKET;
      res = c;
      break;
    }
  CONTINUE_WITH_NEXT_HASH_BUCKET:
    if (++pos == hash_size)
      pos = 0;
  }
  while (!EMPTY_STACK (*sorted)) {
    const unsigned lit = POP_STACK (*sorted);
    marks[lit] = 0;
  }
  if (res)
    LOGCLS (res, "found indexed matching XOR side clause");
  else
    LOG ("no matching XOR side clause found");
  return res;
}

#else

static clause *find_large_xor_side_clause (closure *closure,
                                           unsigneds *lits) {
  kissat *const solver = closure->solver;
  KISSAT_assert (!solver->watching);
  const unsigned *const largecount = closure->largecount;
  unsigned least_occurring_literal = INVALID_LIT;
  unsigned count_least_occurring = UINT_MAX;
  mark *marks = solver->marks;
  const size_t size_lits = SIZE_STACK (*lits);
#if defined(LOGGING) || !defined(KISSAT_NDEBUG)
  const unsigned arity = size_lits - 1;
#endif
#ifndef KISSAT_NDEBUG
  const unsigned count_limit = 1u << (arity - 1);
#endif
  const value *const values = solver->values;
  LOGCOUNTEDLITS (size_lits, BEGIN_STACK (*lits), largecount,
                  "trying to find arity %u XOR side clause", arity);
  for (all_stack (unsigned, lit, *lits)) {
    KISSAT_assert (!values[lit]);
    marks[lit] = 1;
    unsigned count = largecount[lit];
    KISSAT_assert (count_limit <= count);
    if (count >= count_least_occurring)
      continue;
    count_least_occurring = count;
    least_occurring_literal = lit;
  }
  clause *res = 0;
  KISSAT_assert (least_occurring_literal != INVALID_LIT);
  LOG ("searching XOR side clause watched by %s#%u",
       LOGLIT (least_occurring_literal), count_least_occurring);
  watches *const watches = &WATCHES (least_occurring_literal);
  watch *p = BEGIN_WATCHES (*watches);
  const watch *const end = END_WATCHES (*watches);
  while (p != end) {
    const watch watch = *p++;
    if (watch.type.binary)
      break;
    const reference ref = watch.large.ref;
    clause *const c = kissat_dereference_clause (solver, ref);
    if (c->garbage)
      continue;
    if (c->size < size_lits)
      continue;
    size_t found = 0;
    for (all_literals_in_clause (other, c)) {
      const value value = values[other];
      if (value < 0)
        continue;
      if (value > 0) {
        LOGCLS (c, "found satisfied %s in", LOGLIT (other));
        kissat_mark_clause_as_garbage (solver, c);
        KISSAT_assert (c->garbage);
        break;
      }
      if (marks[other])
        found++;
      else {
        found = UINT_MAX;
        break;
      }
    }
    if (found < UINT_MAX && !c->garbage) {
      res = c;
      break;
    }
  }
  for (all_stack (unsigned, lit, *lits))
    marks[lit] = 0;
  if (res)
    LOGCLS (res, "found matching XOR side");
  else
    LOG ("no matching XOR side clause found");
  return res;
}

#endif

static void extract_xor_gates_with_base_clause (closure *closure,
                                                clause *c) {
  KISSAT_assert (!c->garbage);
  kissat *const solver = closure->solver;
  KISSAT_assert (!solver->inconsistent);
  const value *const values = solver->values;
  unsigned smallest = INVALID_LIT, largest = INVALID_LIT;
  const unsigned arity_limit =
      MIN (GET_OPTION (congruencexorarity), MAX_ARITY);
  const unsigned size_limit = arity_limit + 1;
  unsigned negated = 0, size = 0;
  unsigneds *lits = &closure->lits;
  CLEAR_STACK (*lits);
  bool first = true;
  for (all_literals_in_clause (lit, c)) {
    const value value = values[lit];
    if (value < 0)
      continue;
    if (value > 0) {
      LOGCLS (c, "found satisfied %s in", LOGLIT (lit));
      kissat_mark_clause_as_garbage (solver, c);
      return;
    }
    if (size == size_limit) {
      LOGCLS (c, "size limit %u for XOR base clause exceeded in",
              size_limit);
      return;
    }
    if (first) {
      largest = smallest = lit;
      first = false;
    } else {
      KISSAT_assert (smallest != INVALID_LIT);
      KISSAT_assert (largest != INVALID_LIT);
      if (lit < smallest)
        smallest = lit;
      if (lit > largest) {
        if (NEGATED (largest)) {
          LOGCLS (c, "not largest literal %s occurs negated in XOR base",
                  LOGLIT (largest));
          return;
        }
        largest = lit;
      }
    }
    if (NEGATED (lit) && lit < largest) {
      LOGCLS (c, "negated literal %s not largest in XOR base",
              LOGLIT (lit));
      return;
    }
    if (NEGATED (lit) && negated++) {
      LOGCLS (c, "more than one negated literal in XOR base");
      return;
    }
    PUSH_STACK (*lits, lit);
    size++;
  }
  KISSAT_assert (size == SIZE_STACK (*lits));
  if (size < 3) {
    LOGCLS (c, "short XOR base clause");
    return;
  }
  const unsigned arity = size - 1;
  const unsigned needed_clauses = 1u << (arity - 1);
  const unsigned *const largecount = closure->largecount;
  for (all_stack (unsigned, lit, *lits))
    for (unsigned sign = 0; sign != 2; sign++, lit = NOT (lit)) {
      unsigned count = largecount[lit];
      if (count >= needed_clauses)
        continue;
      LOGCLS (c,
              "literal %s in XOR base clause only occurs "
              "%u times in large clauses thus skipping",
              LOGLIT (lit), count);
      return;
    }
  LOGCLS (c, "trying arity %u XOR base", arity);
  KISSAT_assert (smallest != INVALID_LIT);
  KISSAT_assert (largest != INVALID_LIT);
  const unsigned end = 1u << arity;
  KISSAT_assert (negated == parity_lits (solver, lits));
#if !defined(KISSAT_NDEBUG) || defined(LOGGING)
  unsigned found = 0;
#endif
  for (unsigned i = 0; i != end; i++) {
    while (i && parity_lits (solver, lits) != negated)
      inc_lits (solver, lits);
    if (i) {
#ifdef INDEX_LARGE_CLAUSES
      clause *d = find_indexed_large_clause (closure, lits);
#else
      clause *d = find_large_xor_side_clause (closure, lits);
#endif
      if (!d)
        return;
      KISSAT_assert (!d->redundant);
    } else
      KISSAT_assert (!c->redundant);
    inc_lits (solver, lits);
#if !defined(KISSAT_NDEBUG) || defined(LOGGING)
    found++;
#endif
  }
  while (parity_lits (solver, lits) != negated)
    inc_lits (solver, lits);
  LOGUNSIGNEDS2 (size, BEGIN_STACK (*lits), "back to original");
  LOG ("found all needed %u matching clauses:", found);
  KISSAT_assert (found == 1u << arity);
  if (negated) {
    unsigned *p = BEGIN_STACK (*lits), lit;
    while (!NEGATED (lit = *p))
      p++;
    LOG ("flipping RHS literal %s", LOGLIT (lit));
    *p = NOT (lit);
  }
  unsigned extracted = 0;
  for (all_stack (unsigned, lhs, *lits)) {
    if (!negated)
      lhs = NOT (lhs);
    gate *g = new_xor_gate (closure, lhs);
    if (g)
      extracted++;
    if (solver->inconsistent)
      break;
  }
  if (!extracted)
    LOG ("no arity %u XOR gate extracted", arity);
}

#ifdef INDEX_BINARY_CLAUSES

static void init_bintab (closure *closure) {
  kissat *const solver = closure->solver;
  size_t limit = BINARY_CLAUSES;
  size_t size = 2 * limit;
  size_t size2 = 1;
  while (size > size2)
    size2 *= 2;
  KISSAT_assert (!limit || size2 <= 2 * size);
  binary_hash_table *bintab = &closure->bintab;
  CALLOC (binary_hash_table, bintab->table, size);
  bintab->count = 0;
  bintab->size = size;
  bintab->size2 = size2;
  kissat_very_verbose (
      solver, "allocated binary clause hash table of size %zu", size);
}

#ifndef KISSAT_NDEBUG

static bool binaries_hash_table_is_full (binary_hash_table *bintab) {
  if (bintab->size == MAX_HASH_TABLE_SIZE)
    return false;
  if (2 * bintab->count < bintab->size)
    return false;
  return true;
}

#endif

static void index_binary (closure *closure, unsigned lit, unsigned other) {
  KISSAT_assert (lit < other);
  binary_hash_table *bintab = &closure->bintab;
  KISSAT_assert (!binaries_hash_table_is_full (bintab));
  binary_clause binary = {.lits = {lit, other}};
  const unsigned hash = hash_binary (closure, &binary);
  const size_t size = bintab->size;
  const size_t size2 = bintab->size2;
  size_t pos = reduce_hash (hash, size, size2);
  binary_clause *table = bintab->table;
  while (table[pos].lits[1])
    if (++pos == size)
      pos = 0;
  table[pos] = binary;
  bintab->count++;
#ifdef LOGGING
  kissat *const solver = closure->solver;
  LOG ("indexed binary %s %s", LOGLIT (lit), LOGLIT (other));
#endif
}

static void reset_bintab (closure *closure) {
  kissat *const solver = closure->solver;
  binary_hash_table *bintab = &closure->bintab;
  DEALLOC (bintab->table, bintab->size);
}

#endif

static void init_and_gate_extraction (closure *closure) {
  kissat *const solver = closure->solver;
  KISSAT_assert (!solver->watching);
  unsigned *negbincount;
  CALLOC (unsigned, negbincount, LITS);
  litpairs *binaries = &closure->binaries;
#ifdef INDEX_BINARY_CLAUSES
  init_bintab (closure);
#endif
  for (all_stack (litpair, pair, *binaries)) {
    const unsigned lit = pair.lits[0], other = pair.lits[1];
    const unsigned not_lit = NOT (lit), not_other = NOT (other);
    negbincount[not_lit]++, negbincount[not_other]++;
    kissat_watch_binary (solver, lit, other);
#ifdef INDEX_BINARY_CLAUSES
    index_binary (closure, lit, other);
#endif
  }
#ifndef KISSAT_QUIET
  size_t connected = SIZE_STACK (*binaries);
  kissat_very_verbose (solver, "connected %zu binary clauses", connected);
#endif
  closure->negbincount = negbincount;
}

static void reset_and_gate_extraction (closure *closure) {
  kissat *const solver = closure->solver;
  DEALLOC (closure->negbincount, LITS);
  kissat_flush_all_connected (solver);
#ifdef INDEX_BINARY_CLAUSES
  reset_bintab (closure);
#endif
}

#ifdef INDEX_LARGE_CLAUSES

static void init_large_clauses (closure *closure, size_t expected) {
  kissat *const solver = closure->solver;
  size_t size = 2 * expected;
  size_t size2 = 1;
  while (size > size2)
    size2 *= 2;
  KISSAT_assert (!expected || size2 <= 2 * size);
  large_clause_hash_table *clauses = &closure->clauses;
  CALLOC (large_clause_hash_table, clauses->table, size);
  clauses->count = 0;
  clauses->size = size;
  clauses->size2 = size2;
  kissat_very_verbose (
      solver, "allocated large clause hash table of size %zu", size);
}

#ifndef KISSAT_NDEBUG

static bool large_clause_hash_table_is_full (closure *closure) {
  if (closure->clauses.size == MAX_HASH_TABLE_SIZE)
    return false;
  if (2 * closure->clauses.count < closure->clauses.size)
    return false;
  return true;
}

#endif

static void index_large_clause (closure *closure, reference ref) {
  KISSAT_assert (!large_clause_hash_table_is_full (closure));
  kissat *const solver = closure->solver;
  clause *c = kissat_dereference_clause (solver, ref);
  const value *const values = solver->values;
  unsigneds *lits = &closure->lits;
  CLEAR_STACK (*lits);
  for (all_literals_in_clause (lit, c))
    if (!values[lit])
      PUSH_STACK (*lits, lit);
  const size_t size_lits = SIZE_STACK (*lits);
  unsigned *begin_lits = BEGIN_STACK (*lits);
  KISSAT_assert (3 <= size_lits);
  sort_lits (solver, size_lits, begin_lits);
  const unsigned hash = hash_lits (closure, 0, size_lits, begin_lits);
  large_clause_hash_table *clauses = &closure->clauses;
  if (!hash && !ref) {
    ref = INVALID_REF;
    KISSAT_assert (ref);
  }
  const size_t hash_size = clauses->size;
  const size_t hash_size2 = clauses->size2;
  size_t pos = reduce_hash (hash, hash_size, hash_size2);
  hash_ref *table = clauses->table, *clause;
  while (valid_large_clause (clause = table + pos))
    if (++pos == hash_size)
      pos = 0;
  clause->hash = hash;
  clause->ref = ref;
  KISSAT_assert (valid_large_clause (clause));
  clauses->count++;
  LOGCLS (c, "indexed");
}

static void reset_large_clauses (closure *closure) {
  large_clause_hash_table *clauses = &closure->clauses;
  kissat *const solver = closure->solver;
  DEALLOC (clauses->table, clauses->size);
}

#endif

static void init_xor_gate_extraction (closure *closure,
                                      references *candidates) {
  KISSAT_assert (EMPTY_STACK (*candidates));
  kissat *const solver = closure->solver;
  KISSAT_assert (!solver->watching);
  const unsigned arity_limit = GET_OPTION (congruencexorarity);
  const unsigned size_limit = arity_limit + 1;
  clause *last_irredundant = kissat_last_irredundant_clause (solver);
  const value *const values = solver->values;
  unsigned *largecount;
  CALLOC (unsigned, largecount, LITS);
  for (all_clauses (c)) {
    if (c->garbage)
      continue;
    if (last_irredundant && last_irredundant < c)
      break;
    if (c->redundant)
      continue;
    unsigned size = 0;
    int continue_counting_next_clause = 0;
    for (all_literals_in_clause (lit, c)) {
      const value value = values[lit];
      if (value < 0)
        continue;
      if (value > 0) {
        LOGCLS (c, "satisfied %s in", LOGLIT (lit));
        kissat_mark_clause_as_garbage (solver, c);
        continue_counting_next_clause = 1;
        break;
      }
      if (size == size_limit) {
        continue_counting_next_clause = 1;
        break;
      }
      size++;
    }
    if(continue_counting_next_clause) {
      continue;
    }
    if (size < 3)
      continue;
    for (all_literals_in_clause (lit, c))
      if (!values[lit])
        largecount[lit]++;
    reference ref = kissat_reference_clause (solver, c);
    PUSH_STACK (*candidates, ref);
  }
#ifndef KISSAT_QUIET
  size_t considered_clauses = IRREDUNDANT_CLAUSES;
  size_t original_candidates = SIZE_STACK (*candidates);
  kissat_very_verbose (
      solver,
      "%zu original candidate XOR base clauses "
      "(%.0f%% of %zu irredundant clauses)",
      original_candidates,
      kissat_percent (original_candidates, considered_clauses),
      considered_clauses);
#endif
  const unsigned counting_rounds = GET_OPTION (congruencexorcounts);
  for (unsigned round = 1; round <= counting_rounds; round++) {
    size_t removed = 0;
    unsigned *new_largecount;
    CALLOC (unsigned, new_largecount, LITS);
    const reference *const end_candidates = END_STACK (*candidates);
    reference *q = BEGIN_STACK (*candidates), *p = q;
    while (p != end_candidates) {
      const reference ref = *p++;
      clause *c = kissat_dereference_clause (solver, ref);
      unsigned size = 0;
      for (all_literals_in_clause (lit, c))
        if (!values[lit])
          size++;
      KISSAT_assert (3 <= size);
      KISSAT_assert (size <= size_limit);
      const unsigned arity = size - 1;
      const unsigned needed_clauses = 1u << (arity - 1);
      for (all_literals_in_clause (lit, c))
        if (largecount[lit] < needed_clauses) {
          removed++;
          goto CONTINUE_WITH_NEXT_CANDIDATE_CLAUSE;
        }
      for (all_literals_in_clause (lit, c))
        if (!values[lit])
          new_largecount[lit]++;
      *q++ = ref;
    CONTINUE_WITH_NEXT_CANDIDATE_CLAUSE:;
    }
    DEALLOC (largecount, LITS);
    largecount = new_largecount;
    SET_END_OF_STACK (*candidates, q);
    if (!removed)
      break;
#ifndef KISSAT_QUIET
    size_t remaining_candidates = SIZE_STACK (*candidates);
    const char *how_often;
    char buffer[64];
    if (round == 1)
      how_often = "once";
    else if (round == 2)
      how_often = "twice";
    else {
      sprintf (buffer, "%u times", round);
      how_often = buffer;
    }
    kissat_very_verbose (
        solver,
        "%zu XOR base clause candidates remain (%.0f%% "
        "original candidates)"
        " after counting %s",
        remaining_candidates,
        kissat_percent (remaining_candidates, original_candidates),
        how_often);
#endif
  }
  closure->largecount = largecount;
#ifdef INDEX_LARGE_CLAUSES
  init_large_clauses (closure, SIZE_STACK (*candidates));
#endif
  for (all_stack (reference, ref, *candidates)) {
    kissat_connect_referenced (solver, ref);
#ifdef INDEX_LARGE_CLAUSES
    index_large_clause (closure, ref);
#endif
  }
#ifndef KISSAT_QUIET
  size_t connected = SIZE_STACK (*candidates);
  kissat_very_verbose (solver, "connected %zu large clauses %.0f%%",
                       connected,
                       kissat_percent (connected, IRREDUNDANT_CLAUSES));
#endif
}

static void reset_xor_gate_extraction (closure *closure) {
  kissat *const solver = closure->solver;
  DEALLOC (closure->largecount, LITS);
  kissat_flush_all_connected (solver);
#ifdef INDEX_LARGE_CLAUSES
  reset_large_clauses (closure);
#endif
}

static void init_ite_gate_extraction (closure *closure,
                                      references *candidates) {
  KISSAT_assert (EMPTY_STACK (*candidates));
  kissat *const solver = closure->solver;
  clause *last_irredundant = kissat_last_irredundant_clause (solver);
  const value *const values = solver->values;
  unsigned *largecount;
  CALLOC (unsigned, largecount, LITS);
  references ternary;
  INIT_STACK (ternary);
  for (all_clauses (c)) {
    if (c->garbage)
      continue;
    if (last_irredundant && last_irredundant < c)
      break;
    if (c->redundant)
      continue;
    unsigned size = 0;
    int continue_counting_next_clause = 0;
    for (all_literals_in_clause (lit, c)) {
      const value value = values[lit];
      if (value < 0)
        continue;
      if (value > 0) {
        LOGCLS (c, "satisfied %s in", LOGLIT (lit));
        kissat_mark_clause_as_garbage (solver, c);
        continue_counting_next_clause = 1;
        break;
      }
      if (size == 3) {
        continue_counting_next_clause = 1;
        break;
      }
      size++;
    }
    if(continue_counting_next_clause) {
      continue;
    }
    if (size < 3)
      continue;
    KISSAT_assert (size == 3);
    const reference ref = kissat_reference_clause (solver, c);
    PUSH_STACK (ternary, ref);
    LOGCLS (c, "counting original ITE gate base");
    for (all_literals_in_clause (lit, c))
      if (!values[lit])
        largecount[lit]++;
  }
#ifndef KISSAT_QUIET
  size_t counted = SIZE_STACK (ternary);
  kissat_very_verbose (solver,
                       "counted %zu ternary ITE clauses "
                       "(%.0f%% of %" PRIu64 " irredundant clauses)",
                       counted,
                       kissat_percent (counted, IRREDUNDANT_CLAUSES),
                       IRREDUNDANT_CLAUSES);
  size_t connected = 0;
#endif
  for (all_stack (reference, ref, ternary)) {
    clause *c = kissat_dereference_clause (solver, ref);
    KISSAT_assert (!c->garbage);
    unsigned positive = 0, negative = 0, twice = 0;
    for (all_literals_in_clause (lit, c)) {
      if (values[lit])
        continue;
      const unsigned not_lit = NOT (lit);
      const unsigned count_not_lit = largecount[not_lit];
      if (!count_not_lit)
        goto CONTINUE_WITH_NEXT_TERNARY_CLAUSE;
      const unsigned count_lit = largecount[lit];
      KISSAT_assert (count_lit);
      if (count_lit > 1 && count_not_lit > 1)
        twice++;
      if (NEGATED (lit))
        negative++;
      else
        positive++;
    }
    if (twice < 2)
      goto CONTINUE_WITH_NEXT_TERNARY_CLAUSE;
#ifndef KISSAT_QUIET
    connected++;
#endif
    kissat_connect_clause (solver, c);
    if (positive && negative)
      PUSH_STACK (*candidates, ref);
  CONTINUE_WITH_NEXT_TERNARY_CLAUSE:;
  }
  RELEASE_STACK (ternary);
#ifndef KISSAT_QUIET
  kissat_very_verbose (solver,
                       "connected %zu ITE clauses "
                       "(%.0f%% of %" PRIu64 " counted clauses)",
                       connected, kissat_percent (connected, counted),
                       IRREDUNDANT_CLAUSES);
  size_t size_candidates = SIZE_STACK (*candidates);
  kissat_very_verbose (solver,
                       "%zu candidates ITE base clauses "
                       "(%.0f%% of %zu connected)",
                       size_candidates,
                       kissat_percent (size_candidates, connected),
                       connected);
#endif
  closure->largecount = largecount;
  INIT_STACK (closure->condbin[0]);
  INIT_STACK (closure->condbin[1]);
  INIT_STACK (closure->condeq[0]);
  INIT_STACK (closure->condeq[1]);
}

static void reset_ite_gate_extraction (closure *closure) {
  kissat *const solver = closure->solver;
  RELEASE_STACK (closure->condbin[0]);
  RELEASE_STACK (closure->condbin[1]);
  RELEASE_STACK (closure->condeq[0]);
  RELEASE_STACK (closure->condeq[1]);
  DEALLOC (closure->largecount, LITS);
  kissat_flush_all_connected (solver);
}

static void unmark_all (unsigneds *marked, signed char *marks) {
  for (all_stack (unsigned, lit, *marked))
    marks[lit] = 0;
  CLEAR_STACK (*marked);
}

#ifdef MERGE_CONDITIONAL_EQUIVALENCES

static void copy_conditional_equivalences (kissat *solver, unsigned lit,
                                           watches *watches,
                                           litpairs *condbin) {
  KISSAT_assert (EMPTY_STACK (*condbin));
  const value *const values = solver->values;
  const watch *const begin_watches = BEGIN_WATCHES (*watches);
  const watch *const end_watches = END_WATCHES (*watches);
  for (const watch *p = begin_watches; p != end_watches; p++) {
    const watch watch = *p;
    if (watch.type.binary)
      break;
    const unsigned ref = watch.large.ref;
    clause *c = kissat_dereference_clause (solver, ref);
    unsigned first = INVALID_LIT, second = INVALID_LIT;
    for (all_literals_in_clause (other, c)) {
      if (values[other])
        continue;
      if (other == lit)
        continue;
      if (first == INVALID_LIT)
        first = other;
      else {
        KISSAT_assert (second == INVALID_LIT);
        second = other;
      }
    }
    KISSAT_assert (first != INVALID_LIT);
    KISSAT_assert (second != INVALID_LIT);
    litpair pair;
    if (first < second)
      pair = (litpair){.lits = {first, second}};
    else {
      KISSAT_assert (second < first);
      pair = (litpair){.lits = {second, first}};
    }
    LOG ("literal %s conditional binary clause %s %s", LOGLIT (lit),
         LOGLIT (first), LOGLIT (second));
    PUSH_STACK (*condbin, pair);
  }
}

static bool less_litpair (litpair p, litpair q) {
  const unsigned a = p.lits[0], b = q.lits[0];
  if (a < b)
    return true;
  if (a > b)
    return false;
  const unsigned c = p.lits[1], d = q.lits[1];
  return c < d;
}

#define RADIDX_SORT_PAIR_LIMIT 32

static void sort_pairs (kissat *solver, litpairs *pairs) {
  const size_t size = SIZE_STACK (*pairs);
  if (size < 32)
    SORT_STACK (litpair, *pairs, less_litpair);
  else
    for (int i = 1; i >= 0; i--)
      RADIX_STACK (litpair, uint64_t, *pairs, rank_litpair);
}

static bool find_litpair_second_literal (unsigned lit, const litpair *begin,
                                         const litpair *end) {
  const litpair *l = begin, *r = end;
  while (l != r) {
    const litpair *m = l + (r - l) / 2;
    KISSAT_assert (begin <= m), KISSAT_assert (m < end);
    unsigned other = m->lits[1];
    if (other < lit)
      l = m + 1;
    else if (other > lit)
      r = m;
    else
      return true;
  }
  return false;
}

static void search_condeq (closure *closure, unsigned lit, unsigned pos_lit,
                           const litpair *pos_begin, const litpair *pos_end,
                           unsigned neg_lit, const litpair *neg_begin,
                           const litpair *neg_end, litpairs *condeq) {
  kissat *const solver = closure->solver;
  KISSAT_assert (neg_lit == NOT (pos_lit));
  KISSAT_assert (pos_begin < pos_end);
  KISSAT_assert (neg_begin < neg_end);
  KISSAT_assert (pos_begin->lits[0] == pos_lit);
  KISSAT_assert (neg_begin->lits[0] == neg_lit);
  KISSAT_assert (pos_end <= neg_begin || neg_end <= pos_begin);
  for (const litpair *p = pos_begin; p != pos_end; p++) {
    const unsigned other = p->lits[1];
    const unsigned not_other = NOT (other);
    if (find_litpair_second_literal (not_other, neg_begin, neg_end)) {
      unsigned first, second;
      if (NEGATED (pos_lit))
        first = neg_lit, second = other;
      else
        first = pos_lit, second = not_other;
      LOG ("found conditional %s equivalence %s = %s", LOGLIT (lit),
           LOGLIT (first), LOGLIT (second));
      KISSAT_assert (!NEGATED (first));
      KISSAT_assert (first < second);
      check_ternary (closure, lit, first, NOT (second));
      check_ternary (closure, lit, NOT (first), second);
      litpair equivalence = {.lits = {first, second}};
      PUSH_STACK (*condeq, equivalence);
      if (NEGATED (second)) {
        litpair inverse_equivalence = {.lits = {NOT (second), NOT (first)}};
        PUSH_STACK (*condeq, inverse_equivalence);
      } else {
        litpair inverse_equivalence = {.lits = {second, first}};
        PUSH_STACK (*condeq, inverse_equivalence);
      }
    }
  }
#ifndef LOGGING
  (void) lit;
#endif
}

static void extract_condeq_pairs (closure *closure, unsigned lit,
                                  litpairs *condbin, litpairs *condeq) {
#if defined(LOGGING) || !defined(KISSAT_NDEBUG)
  kissat *const solver = closure->solver;
#endif
  const litpair *const begin = BEGIN_STACK (*condbin);
  const litpair *const end = END_STACK (*condbin);
  const litpair *pos_begin = begin;
  unsigned next_lit;
  for (;;) {
    if (pos_begin == end)
      return;
    next_lit = pos_begin->lits[0];
    if (!NEGATED (next_lit))
      break;
    pos_begin++;
  }
  for (;;) {
    KISSAT_assert (pos_begin != end);
    KISSAT_assert (next_lit == pos_begin->lits[0]);
    KISSAT_assert (!NEGATED (next_lit));
    const unsigned pos_lit = next_lit;
    const litpair *pos_end = pos_begin + 1;
    for (;;) {
      if (pos_end == end)
        return;
      next_lit = pos_end->lits[0];
      if (next_lit != pos_lit)
        break;
      pos_end++;
    }
    KISSAT_assert (pos_end != end);
    KISSAT_assert (next_lit == pos_end->lits[0]);
    const unsigned neg_lit = NOT (pos_lit);
    if (next_lit != neg_lit) {
      if (NEGATED (next_lit)) {
        pos_begin = pos_end + 1;
        for (;;) {
          if (pos_begin == end)
            return;
          next_lit = pos_begin->lits[0];
          if (!NEGATED (next_lit))
            break;
          pos_begin++;
        }
      } else
        pos_begin = pos_end;
      continue;
    }
    const litpair *const neg_begin = pos_end;
    const litpair *neg_end = neg_begin + 1;
    while (neg_end != end) {
      next_lit = neg_end->lits[0];
      if (next_lit != neg_lit)
        break;
      neg_end++;
    }
#ifdef LOGGING
    if (kissat_logging (solver)) {
      for (const litpair *p = pos_begin; p != pos_end; p++)
        LOG ("conditional %s binary clause %s %s with positive %s",
             LOGLIT (lit), LOGLIT (p->lits[0]), LOGLIT (p->lits[1]),
             LOGLIT (pos_lit));
      for (const litpair *p = neg_begin; p != neg_end; p++)
        LOG ("conditional %s binary clause %s %s with negative %s",
             LOGLIT (lit), LOGLIT (p->lits[0]), LOGLIT (p->lits[1]),
             LOGLIT (neg_lit));
    }
#endif
    const size_t pos_size = pos_end - pos_begin;
    const size_t neg_size = neg_end - neg_begin;
    if (pos_size <= neg_size) {
      LOG ("searching negation of %zu conditional binary clauses "
           "with positive %s in %zu conditional binary clauses with %s",
           pos_size, LOGLIT (pos_lit), neg_size, LOGLIT (neg_lit));
      search_condeq (closure, lit, pos_lit, pos_begin, pos_end, neg_lit,
                     neg_begin, neg_end, condeq);
    } else {
      LOG ("searching negation of %zu conditional binary clauses "
           "with negative %s in %zu conditional binary clauses with %s",
           neg_size, LOGLIT (neg_lit), pos_size, LOGLIT (pos_lit));
      search_condeq (closure, lit, neg_lit, neg_begin, neg_end, pos_lit,
                     pos_begin, pos_end, condeq);
    }
    if (neg_end == end)
      return;
    KISSAT_assert (next_lit == neg_end->lits[0]);
    if (NEGATED (next_lit)) {
      pos_begin = neg_end + 1;
      for (;;) {
        if (pos_begin == end)
          return;
        next_lit = pos_begin->lits[0];
        if (!NEGATED (next_lit))
          break;
        pos_begin++;
      }
    } else
      pos_begin = neg_end;
  }
}

static void find_conditional_equivalences (closure *closure, unsigned lit,
                                           watches *watches,
                                           litpairs *condbin,
                                           litpairs *condeq) {
  KISSAT_assert (EMPTY_STACK (*condbin));
  KISSAT_assert (EMPTY_STACK (*condeq));
  KISSAT_assert (SIZE_WATCHES (*watches) > 1);
  kissat *const solver = closure->solver;
  copy_conditional_equivalences (solver, lit, watches, condbin);
  sort_pairs (solver, condbin);
#ifdef LOGGING
  if (kissat_logging (solver)) {
    for (all_stack (litpair, pair, *condbin))
      LOG ("sorted conditional %s binary clause %s %s", LOGLIT (lit),
           LOGLIT (pair.lits[0]), LOGLIT (pair.lits[1]));
    LOG ("found %zu conditional %s binary clauses", SIZE_STACK (*condbin),
         LOGLIT (lit));
  }
#endif
  extract_condeq_pairs (closure, lit, condbin, condeq);
  sort_pairs (solver, condeq);
#ifdef LOGGING
  if (kissat_logging (solver)) {
    for (all_stack (litpair, pair, *condeq))
      LOG ("sorted conditional %s equivalence %s = %s", LOGLIT (lit),
           LOGLIT (pair.lits[0]), LOGLIT (pair.lits[1]));
    LOG ("found %zu conditional %s equivalences", SIZE_STACK (*condeq),
         LOGLIT (lit));
  }
#endif
}

static void merge_condeq (closure *closure, unsigned cond, litpairs *condeq,
                          litpairs *not_condeq) {
  kissat *solver = closure->solver;
  KISSAT_assert (!NEGATED (cond));
  const litpair *const begin_condeq = BEGIN_STACK (*condeq);
  const litpair *const end_condeq = END_STACK (*condeq);
  const litpair *const begin_not_condeq = BEGIN_STACK (*not_condeq);
  const litpair *const end_not_condeq = END_STACK (*not_condeq);
  const litpair *p = begin_condeq;
  const litpair *q = begin_not_condeq;
  while (p != end_condeq) {
    litpair cond_pair = *p++;
    const unsigned lhs = cond_pair.lits[0];
    const unsigned then_lit = cond_pair.lits[1];
    KISSAT_assert (!NEGATED (lhs));
    while (q != end_not_condeq && q->lits[0] < lhs)
      q++;
    while (q != end_not_condeq && q->lits[0] == lhs) {
      litpair not_cond_pair = *q++;
      const unsigned else_lit = not_cond_pair.lits[1];
      new_ite_gate (closure, lhs, cond, then_lit, else_lit);
      if (solver->inconsistent)
        return;
    }
  }
}

static void extract_ite_gates_of_literal (closure *closure, unsigned lit,
                                          unsigned not_lit,
                                          watches *lit_watches,
                                          watches *not_lit_watches) {
#ifndef KISSAT_NDEBUG
  kissat *solver = closure->solver;
#endif
  litpairs *condbin = closure->condbin;
  litpairs *condeq = closure->condeq;
  find_conditional_equivalences (closure, lit, lit_watches, condbin + 0,
                                 condeq + 0);
  if (EMPTY_STACK (condeq[0]))
    goto CLEAN_UP;
  find_conditional_equivalences (closure, not_lit, not_lit_watches,
                                 condbin + 1, condeq + 1);
  if (EMPTY_STACK (condeq[1]))
    goto CLEAN_UP;
  if (NEGATED (lit))
    merge_condeq (closure, not_lit, condeq + 0, condeq + 1);
  else
    merge_condeq (closure, lit, condeq + 1, condeq + 0);
CLEAN_UP:
  CLEAR_STACK (condbin[0]);
  CLEAR_STACK (condbin[1]);
  CLEAR_STACK (condeq[0]);
  CLEAR_STACK (condeq[1]);
}

static void extract_ite_gates_of_variable (closure *closure, unsigned idx) {
  kissat *const solver = closure->solver;
  const unsigned lit = LIT (idx);
  const unsigned not_lit = NOT (lit);
  watches *lit_watches = &WATCHES (lit);
  watches *not_lit_watches = &WATCHES (not_lit);
  const size_t size_lit_watches = SIZE_WATCHES (*lit_watches);
  const size_t size_not_lit_watches = SIZE_WATCHES (*not_lit_watches);
  if (size_lit_watches <= size_not_lit_watches) {
    if (size_lit_watches > 1)
      extract_ite_gates_of_literal (closure, lit, not_lit, lit_watches,
                                    not_lit_watches);
  } else {
    if (size_not_lit_watches > 1)
      extract_ite_gates_of_literal (closure, not_lit, lit, not_lit_watches,
                                    lit_watches);
  }
}

#else

static void mark_third_literal_in_ternary_clauses (
    kissat *solver, const value *const values, unsigneds *marked,
    mark *marks, unsigned a, unsigned b) {
  KISSAT_assert (!solver->watching);
  KISSAT_assert (EMPTY_STACK (*marked));
  watches *a_watches = &WATCHES (a);
  watches *b_watches = &WATCHES (b);
  const size_t size_a = SIZE_WATCHES (*a_watches);
  const size_t size_b = SIZE_WATCHES (*b_watches);
  watches *watches = size_a <= size_b ? a_watches : b_watches;
  const watch *const begin = BEGIN_WATCHES (*watches);
  const watch *const end = END_WATCHES (*watches);
  for (const watch *p = begin; p != end; p++) {
    const watch watch = *p;
    KISSAT_assert (!watch.type.binary);
    const reference ref = watch.large.ref;
    clause *c = kissat_dereference_clause (solver, ref);
    KISSAT_assert (!c->garbage);
    unsigned third = INVALID_LIT, found = 0;
    for (all_literals_in_clause (lit, c)) {
      if (values[lit])
        continue;
      if (lit == a || lit == b) {
        found++;
        continue;
      }
      if (third != INVALID_LIT)
        goto NEXT_WATCH;
      third = lit;
    }
    KISSAT_assert (found <= 2);
    if (found < 2)
      goto NEXT_WATCH;
    KISSAT_assert (third != INVALID_LIT);
    if (third == INVALID_LIT)
      goto NEXT_WATCH;
    if (marks[third])
      goto NEXT_WATCH;
    LOGCLS (c, "marking %s as third literal in", LOGLIT (third));
    PUSH_STACK (*marked, third);
    marks[third] = 1;
  NEXT_WATCH:;
  }
}

static void extract_ite_gate (closure *closure, const value *const values,
                              mark *const marks, unsigned lhs,
                              unsigned cond, unsigned then_lit) {
  kissat *const solver = closure->solver;
  KISSAT_assert (!solver->watching);
  unsigned a = NOT (lhs), b = cond;
  watches *a_watches = &WATCHES (a);
  watches *b_watches = &WATCHES (b);
  const size_t size_a = SIZE_WATCHES (*a_watches);
  const size_t size_b = SIZE_WATCHES (*b_watches);
  watches *watches = size_a <= size_b ? a_watches : b_watches;
  const watch *const begin = BEGIN_WATCHES (*watches);
  const watch *const end = END_WATCHES (*watches);
  for (const watch *p = begin; p != end; p++) {
    const watch watch = *p;
    KISSAT_assert (!watch.type.binary);
    const reference ref = watch.large.ref;
    clause *c = kissat_dereference_clause (solver, ref);
    KISSAT_assert (!c->garbage);
    unsigned else_lit = INVALID_LIT, found = 0;
    for (all_literals_in_clause (lit, c)) {
      if (values[lit])
        continue;
      if (lit == a || lit == b) {
        found++;
        continue;
      }
      if (else_lit != INVALID_LIT)
        goto NEXT_WATCH;
      else_lit = lit;
    }
    KISSAT_assert (found <= 2);
    if (found < 2)
      goto NEXT_WATCH;
    KISSAT_assert (else_lit != INVALID_LIT);
    unsigned not_else_lit = NOT (else_lit);
    if (!marks[not_else_lit])
      goto NEXT_WATCH;
    LOGCLS (c, "found fourth matching");
    check_ite_implied (closure, lhs, cond, then_lit, else_lit);
    marks[not_else_lit] = 0;
    new_ite_gate (closure, lhs, cond, then_lit, else_lit);
  NEXT_WATCH:;
  }
  SWAP (unsigned, a, b);
}

static void extract_ite_gates_with_base_clause (closure *closure,
                                                clause *c) {
  KISSAT_assert (!c->garbage);
  kissat *const solver = closure->solver;
  KISSAT_assert (!solver->inconsistent);
  const value *const values = solver->values;
  const unsigned *const largecount = closure->largecount;
  unsigneds *lits = &closure->lits;
  CLEAR_STACK (*lits);
  unsigned sum = 0;
  for (all_literals_in_clause (lit, c)) {
    const value value = values[lit];
    if (value < 0)
      continue;
    if (value > 0) {
      LOGCLS (c, "found satisfied %s in", LOGLIT (lit));
      kissat_mark_clause_as_garbage (solver, c);
      return;
    }
    PUSH_STACK (*lits, lit);
    sum ^= lit;
  }
  const size_t size = SIZE_STACK (*lits);
  KISSAT_assert (size <= 3);
  if (size < 3)
    return;
  mark *const marks = solver->marks;
  unsigneds *marked = &solver->analyzed;
  for (all_stack (unsigned, lhs, *lits)) {
    if (NEGATED (lhs))
      continue;
    if (largecount[lhs] < 2)
      continue;
    const unsigned not_lhs = NOT (lhs);
    if (largecount[not_lhs] < 2)
      continue;
    for (all_stack (unsigned, not_cond, *lits)) {
      if (not_cond == lhs)
        continue;
      if (!NEGATED (not_cond))
        continue;
      if (largecount[not_cond] < 2)
        continue;
      const unsigned cond = NOT (not_cond);
      if (largecount[cond] < 2)
        continue;
      const unsigned not_then_lit = sum ^ lhs ^ not_cond;
      const unsigned then_lit = NOT (not_then_lit);
      if (!largecount[then_lit])
        continue;
      LOGCLS (c, "found first ITE gate '%s := %s ? %s : ...' gate base",
              LOGLIT (lhs), LOGLIT (cond), LOGLIT (then_lit));
      clause *d = find_ternary_clause (solver, not_lhs, not_cond, then_lit);
      if (!d)
        continue;
      LOGCLS (d, "found matching second ITE gate");
      mark_third_literal_in_ternary_clauses (solver, values, marked, marks,
                                             lhs, cond);
      extract_ite_gate (closure, values, marks, lhs, cond, then_lit);
      unmark_all (marked, marks);
    }
  }
}

#endif

static void extract_and_gates (closure *closure) {
  kissat *const solver = closure->solver;
  if (!GET_OPTION (congruenceands))
    return;
  START (extractands);
#ifndef KISSAT_QUIET
  const statistics *s = &solver->statistics_;
  const uint64_t matched_before = s->congruent_matched_ands;
  const uint64_t gates_before = s->congruent_gates_ands;
#endif
  init_and_gate_extraction (closure);
  clause *last_irredundant = kissat_last_irredundant_clause (solver);
  for (all_clauses (c)) {
    if (TERMINATED (congruence_terminated_1))
      break;
    if (solver->inconsistent)
      break;
    if (last_irredundant && last_irredundant < c)
      break;
    if (c->redundant)
      continue;
    if (c->garbage)
      continue;
    extract_and_gates_with_base_clause (closure, c);
  }
  reset_and_gate_extraction (closure);
#ifndef KISSAT_QUIET
  const uint64_t matched = s->congruent_matched_ands - matched_before;
  const uint64_t extracted = s->congruent_gates_ands - gates_before;
  const uint64_t found = matched + extracted;
  kissat_phase (solver, "congruence", GET (closures),
                "found %" PRIu64 " AND gates (%" PRIu64
                " extracted %.0f%% + %" PRIu64 " matched %.0f%%)",
                found, extracted, kissat_percent (extracted, found),
                matched, kissat_percent (matched, found));
#endif
  STOP (extractands);
}

static void extract_xor_gates (closure *closure) {
  kissat *const solver = closure->solver;
  if (!GET_OPTION (congruencexors))
    return;
  START (extractxors);
  references candidates;
  INIT_STACK (candidates);
  init_xor_gate_extraction (closure, &candidates);
  SHRINK_STACK (candidates);
#ifndef KISSAT_QUIET
  const statistics *s = &solver->statistics_;
  const uint64_t matched_before = s->congruent_matched_xors;
  const uint64_t gates_before = s->congruent_gates_xors;
#endif
  for (all_stack (reference, ref, candidates)) {
    if (TERMINATED (congruence_terminated_2))
      break;
    if (solver->inconsistent)
      break;
    clause *c = kissat_dereference_clause (solver, ref);
    if (c->garbage)
      continue;
    extract_xor_gates_with_base_clause (closure, c);
  }
  reset_xor_gate_extraction (closure);
  RELEASE_STACK (candidates);
#ifndef KISSAT_QUIET
  const uint64_t matched = s->congruent_matched_xors - matched_before;
  const uint64_t extracted = s->congruent_gates_xors - gates_before;
  const uint64_t found = matched + extracted;
  kissat_phase (solver, "congruence", GET (closures),
                "found %" PRIu64 " XOR gates (%" PRIu64
                " extracted %.0f%% + %" PRIu64 " matched %.0f%%)",
                found, extracted, kissat_percent (extracted, found),
                matched, kissat_percent (matched, found));
#endif
  STOP (extractxors);
}

static void extract_ite_gates (closure *closure) {
  kissat *const solver = closure->solver;
  if (!GET_OPTION (congruenceites))
    return;
  START (extractites);
  references candidates;
  INIT_STACK (candidates);
  init_ite_gate_extraction (closure, &candidates);
#ifndef KISSAT_QUIET
  const statistics *s = &solver->statistics_;
  const uint64_t matched_before = s->congruent_matched_ites;
  const uint64_t gates_before = s->congruent_gates_ites;
#endif
#ifdef MERGE_CONDITIONAL_EQUIVALENCES
  for (all_variables (idx))
    if (ACTIVE (idx)) {
      extract_ite_gates_of_variable (closure, idx);
      if (solver->inconsistent)
        break;
    }
#else
  for (all_stack (reference, ref, candidates)) {
    if (TERMINATED (congruence_terminated_3))
      break;
    if (solver->inconsistent)
      break;
    clause *c = kissat_dereference_clause (solver, ref);
    if (c->garbage)
      continue;
    extract_ite_gates_with_base_clause (closure, c);
  }
#endif
  reset_ite_gate_extraction (closure);
  RELEASE_STACK (candidates);
#ifndef KISSAT_QUIET
  const uint64_t matched = s->congruent_matched_ites - matched_before;
  const uint64_t extracted = s->congruent_gates_ites - gates_before;
  const uint64_t found = matched + extracted;
  kissat_phase (solver, "congruence", GET (closures),
                "found %" PRIu64 " ITE gates (%" PRIu64
                " extracted %.0f%% + %" PRIu64 " matched %.0f%%)",
                found, extracted, kissat_percent (extracted, found),
                matched, kissat_percent (matched, found));
#endif
  STOP (extractites);
}

static void init_extraction (closure *closure) {
  kissat *const solver = closure->solver;
  kissat_enter_dense_mode (solver, &closure->binaries);
}

static void reset_extraction (closure *closure) {
  kissat *const solver = closure->solver;
  kissat_resume_sparse_mode (solver, false, &closure->binaries);
  RELEASE_STACK (closure->binaries);
}

static void extract_gates (closure *closure) {
  kissat *const solver = closure->solver;
  START (extract);
  KISSAT_assert (!solver->level);
#ifndef KISSAT_QUIET
  const statistics *s = &solver->statistics_;
  const uint64_t before = s->congruent_gates + s->congruent_matched;
#endif
  init_extraction (closure);
  extract_binaries (closure);
  KISSAT_assert (!solver->inconsistent);
  extract_and_gates (closure);
  if (!solver->inconsistent && !TERMINATED (congruence_terminated_4)) {
    extract_xor_gates (closure);
    if (!solver->inconsistent && !TERMINATED (congruence_terminated_5))
      extract_ite_gates (closure);
  }
  reset_extraction (closure);
#ifndef KISSAT_QUIET
  const uint64_t after = s->congruent_gates + s->congruent_matched;
  const uint64_t found = after - before;
  kissat_phase (solver, "congruence", GET (closures),
                "found %" PRIu64 " gates (%.2f%% variables)", found,
                kissat_percent (found, solver->active));
#endif
  STOP (extract);
}

static void find_units (closure *closure) {
  kissat *const solver = closure->solver;
  KISSAT_assert (solver->watching);
  KISSAT_assert (!solver->inconsistent);
  KISSAT_assert (kissat_propagated (solver));
  closure->units = solver->propagate;
  unsigneds *marked = &solver->analyzed;
  mark *const marks = solver->marks;
  size_t units = 0;
  for (all_variables (idx)) {
  RESTART:
    if (!ACTIVE (idx))
      continue;
    unsigned lit = LIT (idx);
    for (unsigned sign = 0; sign != 2; sign++, lit++) {
      watches *const watches = &WATCHES (lit);
      const watch *p = BEGIN_WATCHES (*watches);
      const watch *const end = END_WATCHES (*watches);
      KISSAT_assert (EMPTY_STACK (*marked));
      while (p != end) {
        const watch watch = *p++;
        if (!watch.type.binary)
          break;
        const unsigned other = watch.binary.lit;
        const unsigned not_other = NOT (other);
        if (marks[not_other]) {
          LOG ("binary clauses %s %s and %s %s yield unit %s", LOGLIT (lit),
               LOGLIT (other), LOGLIT (lit), LOGLIT (not_other),
               LOGLIT (lit));
          units++;
          bool failed = !learn_congruence_unit (closure, lit);
          unmark_all (marked, marks);
          if (failed)
            return;
          else
            goto RESTART;
        }
        if (marks[other])
          continue;
        marks[other] = 1;
        PUSH_STACK (*marked, other);
      }
      unmark_all (marked, marks);
    }
  }
  KISSAT_assert (EMPTY_STACK (*marked));
#ifndef KISSAT_QUIET
  kissat_very_verbose (solver, "found %zu units", units);
#else
  (void) units;
#endif
}

static void find_equivalences (closure *closure) {
  kissat *const solver = closure->solver;
  KISSAT_assert (solver->watching);
  KISSAT_assert (!solver->inconsistent);
  unsigneds *const marked = &solver->analyzed;
  mark *const marks = solver->marks;
  KISSAT_assert (EMPTY_STACK (*marked));
  for (all_variables (idx)) {
  RESTART:
    if (!ACTIVE (idx))
      continue;
    const unsigned lit = LIT (idx);
    watches *lit_watches = &WATCHES (lit);
    const watch *p = BEGIN_WATCHES (*lit_watches);
    const watch *const end_lit_watches = END_WATCHES (*lit_watches);
    KISSAT_assert (EMPTY_STACK (*marked));
    while (p != end_lit_watches) {
      const watch watch = *p++;
      if (!watch.type.binary)
        break;
      const unsigned other = watch.binary.lit;
      if (lit > other)
        continue;
      if (marks[other])
        continue;
      marks[other] = 1;
      PUSH_STACK (*marked, other);
    }
    if (EMPTY_STACK (*marked))
      continue;
    const unsigned not_lit = NOT (lit);
    watches *not_lit_watches = &WATCHES (not_lit);
    p = BEGIN_WATCHES (*not_lit_watches);
    const watch *const end_not_lit_watches = END_WATCHES (*not_lit_watches);
    while (p != end_not_lit_watches) {
      const watch watch = *p++;
      if (!watch.type.binary)
        break;
      const unsigned other = watch.binary.lit;
      if (not_lit > other)
        continue;
      if (lit == other)
        continue;
      const unsigned not_other = NOT (other);
      if (marks[not_other]) {
        unsigned lit_repr = find_repr (closure, lit);
        unsigned other_repr = find_repr (closure, other);
        if (lit_repr != other_repr) {
          if (merge_literals (closure, lit, other))
            INC (congruent_equivalences);
          unmark_all (marked, marks);
          if (solver->inconsistent)
            return;
          else
            goto RESTART;
        }
      }
    }
    unmark_all (marked, marks);
  }
  KISSAT_assert (EMPTY_STACK (*marked));
#ifndef KISSAT_QUIET
  size_t found = SIZE_FIFO (closure->schedule);
  kissat_very_verbose (solver, "found %zu equivalences", found);
#endif
}

static bool simplify_gates (closure *closure, unsigned lit) {
  kissat *const solver = closure->solver;
  LOG ("simplifying gates with RHS literal %s", LOGLIT (lit));
  KISSAT_assert (solver->values[lit]);
  gates *lit_occurrences = closure->occurrences + lit;
  for (all_pointers (gate, g, *lit_occurrences))
    if (!simplify_gate (closure, g))
      return false;
  RELEASE_STACK (*lit_occurrences);
  return true;
}

static bool rewrite_gates (closure *closure, unsigned dst, unsigned src) {
  kissat *const solver = closure->solver;
  LOG ("rewriting gates with RHS literal %s", LOGLIT (src));
  gates *occurrences = closure->occurrences;
  gates *src_occurrences = occurrences + src;
  gates *dst_occurrences = occurrences + dst;
  for (all_pointers (gate, g, *src_occurrences))
    if (!rewrite_gate (closure, g, dst, src))
      return false;
    else if (!g->garbage && gate_contains (g, dst))
      PUSH_STACK (*dst_occurrences, g);
  RELEASE_STACK (*src_occurrences);
  return true;
}

static bool propagate_unit (closure *closure, unsigned lit) {
  kissat *const solver = closure->solver;
  LOG ("propagation of congruence unit %s", LOGLIT (lit));
  (void) solver;
  KISSAT_assert (!solver->inconsistent);
  const unsigned not_lit = NOT (lit);
  return simplify_gates (closure, lit) && simplify_gates (closure, not_lit);
}

static bool propagate_equivalence (closure *closure, unsigned lit) {
  kissat *const solver = closure->solver;
  LOG ("propagation of congruence equivalence %s", CLOGREPR (lit));
  KISSAT_assert (!solver->inconsistent);
  if (VALUE (lit))
    return true;
  const unsigned lit_repr = find_repr (closure, lit);
  if (solver->inconsistent)
    return false;
  const unsigned not_lit = NOT (lit);
  const unsigned not_lit_repr = NOT (lit_repr);
  return rewrite_gates (closure, lit_repr, lit) &&
         rewrite_gates (closure, not_lit_repr, not_lit);
}

static bool propagate_units (closure *closure) {
  kissat *const solver = closure->solver;
  KISSAT_assert (!solver->inconsistent);
  const unsigned_array *const trail = &solver->trail;
  while (closure->units != trail->end)
    if (!propagate_unit (closure, *closure->units++))
      return false;
  return true;
}

static size_t propagate_units_and_equivalences (closure *closure) {
  kissat *const solver = closure->solver;
  KISSAT_assert (!solver->inconsistent);
  START (merge);
  unsigned_fifo *schedule = &closure->schedule;
  size_t propagated = 0;
  while (!TERMINATED (congruence_terminated_6) &&
         propagate_units (closure) && !EMPTY_FIFO (*schedule)) {
    propagated++;
    unsigned lit = dequeue_next_scheduled_literal (closure);
    if (!propagate_equivalence (closure, lit))
      break;
  }
#ifndef KISSAT_QUIET
  const size_t units = closure->units - solver->trail.begin;
  kissat_very_verbose (solver, "propagated %zu congruence units", units);
  kissat_very_verbose (solver, "propagated %zu congruence equivalences",
                       propagated);
#endif
  STOP (merge);
  return propagated;
}

#ifndef KISSAT_NDEBUG

static void dump_closure_literal (closure *closure, unsigned ilit) {
  kissat *const solver = closure->solver;
  const int elit = kissat_export_literal (solver, ilit);
  printf ("%u(%d)", ilit, elit);
  unsigned repr_ilit = find_repr (closure, ilit);
  if (repr_ilit != ilit) {
    const int repr_elit = kissat_export_literal (solver, repr_ilit);
    printf ("[%u(%d)]", repr_ilit, repr_elit);
  }
  const int value = VALUE (ilit);
  KISSAT_assert (!solver->level);
  if (value)
    printf ("@0=%d", value);
}

static void dump_units (closure *closure) {
  unsigned_array *trail = &closure->solver->trail;
  size_t i = 0, propagate = closure->units - trail->begin;
  for (all_stack (unsigned, lit, *trail)) {
    printf ("trail[%zu] ", i);
    dump_closure_literal (closure, lit);
    if (i == propagate)
      fputs (" <-- next unit to propagate", stdout);
    fputc ('\n', stdout);
    i++;
  }
}

static void dump_equivalences (closure *closure) {
  kissat *const solver = closure->solver;
  for (all_variables (idx)) {
    unsigned lit = LIT (idx);
    unsigned repr = closure->repr[lit];
    if (repr != lit)
      printf ("repr[%u(%d)] = %u(%d)\n", lit,
              kissat_export_literal (solver, lit), repr,
              kissat_export_literal (solver, repr));
  }
}

static void dump_gate (closure *closure, gate *g) {
  const unsigned tag = g->tag;
  const char *str;
  switch (tag) {
  case AND_GATE:
    str = "AND";
    break;
  case ITE_GATE:
    str = "ITE";
    break;
  case XOR_GATE:
    str = "XOR";
    break;
  default:
    str = "UNKNOWN";
    break;
  }
  printf ("%p %s gate[%zu] ", (void *) g, str, g->id);
  dump_closure_literal (closure, g->lhs);
  fputs (" := ", stdout);
  if (g->tag == ITE_GATE) {
    dump_closure_literal (closure, g->rhs[0]);
    fputs (" ? ", stdout);
    dump_closure_literal (closure, g->rhs[1]);
    fputs (" : ", stdout);
    dump_closure_literal (closure, g->rhs[2]);
  } else {
    bool first = true;
    for (all_rhs_literals_in_gate (rhs, g)) {
      if (first)
        first = false;
      else if (g->tag == AND_GATE)
        fputs (" & ", stdout);
      else
        fputs (" ^ ", stdout);
      dump_closure_literal (closure, rhs);
    }
  }
  fputs (g->indexed ? " removed" : " indexed", stdout);
  if (g->garbage)
    fputs (" garbage", stdout);
  fputc ('\n', stdout);
}

#define LESS_GATE(G, H) ((G)->id < (H)->id)

static void dump_gates (closure *closure) {
  gates gates;
  INIT_STACK (gates);
  kissat *const solver = closure->solver;
  for (unsigned pos = 0; pos != closure->hash.size; pos++) {
    gate *g = closure->hash.table[pos];
    if (!g)
      continue;
    if (g == REMOVED)
      continue;
    PUSH_STACK (gates, g);
  }
  SORT_STACK (gate *, gates, LESS_GATE);
  for (all_pointers (gate, g, gates))
    dump_gate (closure, g);
  RELEASE_STACK (gates);
}

void kissat_dump_closure (closure *closure) {
  dump_units (closure);
  dump_equivalences (closure);
  dump_gates (closure);
}

#endif

static bool find_subsuming_clause (closure *closure, clause *c) {
  KISSAT_assert (!c->garbage);
  kissat *const solver = closure->solver;
  const reference c_ref = kissat_reference_clause (solver, c);
  const value *const values = solver->values;
  mark *marks = solver->marks;
  {
    const unsigned *const end_lits = c->lits + c->size;
    for (const unsigned *p = c->lits; p != end_lits; p++) {
      const unsigned lit = *p;
      KISSAT_assert (values[lit] <= 0);
      const unsigned repr_lit = find_repr (closure, lit);
      const value value_repr_lit = values[repr_lit];
      KISSAT_assert (value_repr_lit <= 0);
      if (value_repr_lit < 0)
        continue;
      if (marks[repr_lit])
        continue;
      KISSAT_assert (!marks[NOT (repr_lit)]);
      marks[repr_lit] = 1;
    }
  }
  unsigned least_occurring_literal = INVALID_LIT;
  unsigned count_least_occurring = UINT_MAX;
  LOGREPRCLS (c, closure->repr, "trying to forward subsume");
  clause *subsuming = 0;
  for (all_literals_in_clause (lit, c)) {
    const unsigned repr_lit = find_repr (closure, lit);
    watches *const watches = &WATCHES (repr_lit);
    const watch *p = BEGIN_WATCHES (*watches);
    const watch *const end = END_WATCHES (*watches);
    const size_t count = end - p;
    KISSAT_assert (count <= UINT_MAX);
    if (count < count_least_occurring) {
      count_least_occurring = count;
      least_occurring_literal = repr_lit;
    }
    while (p != end) {
      const watch watch = *p++;
      KISSAT_assert (!watch.type.binary);
      const reference d_ref = watch.large.ref;
      clause *const d = kissat_dereference_clause (solver, d_ref);
      KISSAT_assert (c != d);
      KISSAT_assert (!d->garbage);
      if (!c->redundant && d->redundant)
        continue;
      for (all_literals_in_clause (other, d)) {
        const value value = values[other];
        if (value < 0)
          continue;
        KISSAT_assert (!value);
        const unsigned repr_other = find_repr (closure, other);
        if (!marks[repr_other])
          goto CONTINUE_WITH_NEXT_CLAUSE;
      }
      subsuming = d;
      goto FOUND_SUBSUMING;
    CONTINUE_WITH_NEXT_CLAUSE:;
    }
  }
FOUND_SUBSUMING:
  for (all_literals_in_clause (lit, c)) {
    const unsigned repr_lit = find_repr (closure, lit);
    const value value = values[repr_lit];
    if (!value)
      marks[repr_lit] = 0;
  }
  if (subsuming) {
    LOGREPRCLS (c, closure->repr, "subsumed");
    LOGREPRCLS (subsuming, closure->repr, "subsuming");
    kissat_mark_clause_as_garbage (solver, c);
    INC (congruent_subsumed);
    return true;
  } else {
    KISSAT_assert (least_occurring_literal != INVALID_LIT);
    KISSAT_assert (count_least_occurring < UINT_MAX);
    LOGCLS (c, "forward subsumption failed of");
    LOG ("connecting %u occurring %s", count_least_occurring,
         LOGLIT (least_occurring_literal));
    kissat_connect_literal (solver, least_occurring_literal, c_ref);
    return false;
  }
}

struct refsize {
  reference ref;
  unsigned size;
};

typedef struct refsize refsize;
typedef STACK (refsize) refsizes;

#define RANKREFSIZE(REFSIZE) ((REFSIZE).size)

static void sort_references_by_clause_size (kissat *solver,
                                            refsizes *candidates) {
  RADIX_STACK (refsize, unsigned, *candidates, RANKREFSIZE);
}

static void forward_subsume_matching_clauses (closure *closure) {
  kissat *const solver = closure->solver;
  START (matching);
  reset_closure (closure);
  litpairs binaries;
  INIT_STACK (binaries);
  kissat_enter_dense_mode (solver, &binaries);
  bool *matchable;
#ifndef KISSAT_QUIET
  unsigned count_matchable = 0;
#endif
  CALLOC (bool, matchable, VARS);
  for (all_variables (idx))
    if (ACTIVE (idx)) {
      const unsigned lit = LIT (idx);
      const unsigned repr = find_repr (closure, lit);
      if (lit == repr)
        continue;
      const unsigned repr_idx = IDX (repr);
      if (!matchable[idx]) {
        LOG ("matchable %s", LOGVAR (idx));
        matchable[idx] = true;
#ifndef KISSAT_QUIET
        count_matchable++;
#endif
      }
      if (!matchable[repr_idx]) {
        LOG ("matchable %s", LOGVAR (repr_idx));
        matchable[repr_idx] = true;
#ifndef KISSAT_QUIET
        count_matchable++;
#endif
      }
    }
  kissat_phase (solver, "congruence", GET (closures),
                "found %u matchable variables %.0f%%", count_matchable,
                kissat_percent (count_matchable, solver->active));
  size_t potential = 0;
  refsizes candidates;
  INIT_STACK (candidates);
  clause *last_irredundant = kissat_last_irredundant_clause (solver);
  const value *const values = solver->values;
  mark *const marks = solver->marks;
  unsigneds *marked = &solver->analyzed;
  for (all_clauses (c)) {
    if (c->garbage)
      continue;
    if (last_irredundant && last_irredundant < c)
      break;
    potential++;
    bool contains_matchable = false;
    KISSAT_assert (EMPTY_STACK (*marked));
    LOGREPRCLS (c, closure->repr, "considering");
    for (all_literals_in_clause (lit, c)) {
      const value value = values[lit];
      if (value < 0)
        continue;
      if (value > 0) {
        LOGCLS (c, "satisfied %s in", LOGLIT (lit));
        kissat_mark_clause_as_garbage (solver, c);
        break;
      }
      if (!contains_matchable) {
        const unsigned lit_idx = IDX (lit);
        if (matchable[lit_idx])
          contains_matchable = true;
      }
      const unsigned repr = find_repr (closure, lit);
      KISSAT_assert (!values[repr]);
      if (marks[repr])
        continue;
      const unsigned not_repr = NOT (repr);
      if (marks[not_repr]) {
        LOGCLS (c, "matches both %s and %s", CLOGREPR (lit),
                LOGLIT (not_repr));
        kissat_mark_clause_as_garbage (solver, c);
        break;
      }
      marks[repr] = 1;
      PUSH_STACK (*marked, repr);
    }
    const size_t size = SIZE_STACK (*marked);
    for (all_stack (unsigned, repr, *marked))
      marks[repr] = 0;
    CLEAR_STACK (*marked);
    if (c->garbage)
      continue;
    if (!contains_matchable) {
      LOGREPRCLS (c, closure->repr, "no matchable variable in");
      continue;
    }
    const reference ref = kissat_reference_clause (solver, c);
    KISSAT_assert (size <= UINT_MAX);
    refsize refsize = {.ref = ref, .size = (unsigned)size};
    PUSH_STACK (candidates, refsize);
  }
  DEALLOC (matchable, VARS);
#ifndef KISSAT_QUIET
  const size_t size_candidates = SIZE_STACK (candidates);
  kissat_very_verbose (
      solver, "considering %zu matchable subsumption candidates %.0f%%",
      size_candidates, kissat_percent (size_candidates, potential));
#else
  (void) potential;
#endif
  sort_references_by_clause_size (solver, &candidates);
#ifndef KISSAT_QUIET
  size_t tried = 0, subsumed = 0;
#endif
  for (all_stack (refsize, refsize, candidates)) {
    if (TERMINATED (congruence_terminated_7))
      break;
#ifndef KISSAT_QUIET
    tried++;
#endif
    const unsigned ref = refsize.ref;
    clause *c = kissat_dereference_clause (solver, ref);
    if (find_subsuming_clause (closure, c)) {
#ifndef KISSAT_QUIET
      subsumed++;
#endif
    }
  }
  kissat_phase (solver, "congruence", GET (closures),
                "subsumed %zu clauses out of %zu tried %.0f%%", subsumed,
                tried, kissat_percent (subsumed, tried));
  kissat_resume_sparse_mode (solver, false, &binaries);
  RELEASE_STACK (candidates);
  RELEASE_STACK (binaries);
  STOP (matching);
}

bool kissat_congruence (kissat *solver) {
  if (solver->inconsistent)
    return false;
  kissat_check_statistics (solver);
  KISSAT_assert (!solver->level);
  KISSAT_assert (solver->probing);
  KISSAT_assert (solver->watching);
  if (!GET_OPTION (congruence))
    return false;
  if (!GET_OPTION (congruenceands) && !GET_OPTION (congruenceites) &&
      !GET_OPTION (congruencexors))
    return false;
  if (GET_OPTION (congruenceonce) && solver->statistics_.closures)
    return false;
  if (TERMINATED (congruence_terminated_8))
    return false;
  if (DELAYING (congruence))
    return false;
  START (congruence);
  INC (closures);
  closure closure;
  init_closure (solver, &closure);
  extract_gates (&closure);
  bool reset = false;
  if (!solver->inconsistent && !TERMINATED (congruence_terminated_9)) {
    find_units (&closure);
    if (!solver->inconsistent && !TERMINATED (congruence_terminated_10)) {
      find_equivalences (&closure);
      if (!solver->inconsistent && !TERMINATED (congruence_terminated_11)) {
        size_t propagated = propagate_units_and_equivalences (&closure);
        if (!solver->inconsistent && propagated &&
            !TERMINATED (congruence_terminated_12)) {
          forward_subsume_matching_clauses (&closure);
          reset = true;
        }
      }
    }
  }
  if (!reset)
    reset_closure (&closure);
  unsigned equivalent = reset_repr (&closure);
  kissat_phase (solver, "congruence", GET (closures),
                "merged %u equivalent variables %.2f%%", equivalent,
                kissat_percent (equivalent, solver->active));
  KISSAT_assert (solver->active >= equivalent);
#ifndef KISSAT_QUIET
  solver->active -= equivalent;
  REPORT (!equivalent, 'c');
  if (!solver->inconsistent)
    solver->active += equivalent;
#endif
  if (kissat_average (equivalent, solver->active) < 0.001)
    BUMP_DELAY (congruence);
  else
    REDUCE_DELAY (congruence);
  STOP (congruence);
  kissat_check_statistics (solver);
  return equivalent;
}

ABC_NAMESPACE_IMPL_END
