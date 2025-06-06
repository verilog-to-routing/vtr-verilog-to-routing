#include "global.h"

#include "kitten.h"
#include "random.h"
#include "stack.h"

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ABC_NAMESPACE_IMPL_START

typedef signed char value;

static void die (const char *fmt, ...) {
  fputs ("cadical_kitten: error: ", stderr);
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  exit (1);
}

static inline void *cadical_kitten_calloc (size_t n, size_t size) {
  void *res = calloc (n, size);
  if (n && size && !res)
    die ("out of memory allocating '%zu * %zu' bytes", n, size);
  return res;
}

#define CALLOC(T, P, N)                          \
  do { \
    (P) = (T*)cadical_kitten_calloc (N, sizeof *(P));   \
  } while (0)
#define DEALLOC(P, N) free (P)

#undef ENLARGE_STACK

#define ENLARGE_STACK(S) \
  do { \
    CADICAL_assert (FULL_STACK (S)); \
    const size_t SIZE = SIZE_STACK (S); \
    const size_t OLD_CAPACITY = CAPACITY_STACK (S); \
    const size_t NEW_CAPACITY = OLD_CAPACITY ? 2 * OLD_CAPACITY : 1; \
    const size_t BYTES = NEW_CAPACITY * sizeof *(S).begin; \
    (S).begin = (unsigned*)realloc ((S).begin, BYTES);              \
    if (!(S).begin) \
      die ("out of memory reallocating '%zu' bytes", BYTES); \
    (S).allocated = (S).begin + NEW_CAPACITY; \
    (S).end = (S).begin + SIZE; \
  } while (0)

// Beside allocators above also use stand alone statistics counters.

#define INC(NAME) \
  do { \
    statistics *statistics = &cadical_kitten->statistics; \
    CADICAL_assert (statistics->NAME < UINT64_MAX); \
    statistics->NAME++; \
  } while (0)

#define ADD(NAME, DELTA) \
  do { \
    statistics *statistics = &cadical_kitten->statistics; \
    CADICAL_assert (statistics->NAME <= UINT64_MAX - (DELTA)); \
    statistics->NAME += (DELTA); \
  } while (0)

#define KITTEN_TICKS (cadical_kitten->statistics.cadical_kitten_ticks)

#define INVALID UINT_MAX
#define MAX_VARS ((1u << 31) - 1)

#define CORE_FLAG (1u)
#define LEARNED_FLAG (2u)

// clang-format off

typedef struct kar kar;
typedef struct kink kink;
typedef struct klause klause;
typedef STACK (unsigned) klauses;
typedef unsigneds katches;

// clang-format on

struct kar {
  unsigned level;
  unsigned reason;
};

struct kink {
  unsigned next;
  unsigned prev;
  uint64_t stamp;
};

struct klause {
  unsigned aux;
  unsigned size;
  unsigned flags;
  unsigned lits[1];
};

typedef struct statistics statistics;

struct statistics {
  uint64_t learned;
  uint64_t original;
  uint64_t cadical_kitten_flip;
  uint64_t cadical_kitten_flipped;
  uint64_t cadical_kitten_sat;
  uint64_t cadical_kitten_solved;
  uint64_t cadical_kitten_conflicts;
  uint64_t cadical_kitten_decisions;
  uint64_t cadical_kitten_propagations;
  uint64_t cadical_kitten_ticks;
  uint64_t cadical_kitten_unknown;
  uint64_t cadical_kitten_unsat;
};

typedef struct kimits kimits;

struct kimits {
  uint64_t ticks;
};

struct cadical_kitten {
  // First zero initialized field in 'clear_cadical_kitten' is 'status'.
  //
  int status;

#if defined(LOGGING)
  bool logging;
#endif
  bool antecedents;
  bool learned;

  unsigned level;
  unsigned propagated;
  unsigned unassigned;
  unsigned inconsistent;
  unsigned failing;

  uint64_t generator;

  size_t lits;
  size_t evars;

  size_t end_original_ref;

  struct {
    unsigned first, last;
    uint64_t stamp;
    unsigned search;
  } queue;

  // The 'size' field below is the first not zero reinitialized field
  // by 'memset' in 'clear_cadical_kitten' (after 'kissat').

  size_t size;
  size_t esize;

  kar *vars;
  kink *links;
  value *marks;
  value *values;
  bool *failed;
  unsigned char *phases;
  unsigned *import;
  katches *watches;

  unsigneds analyzed;
  unsigneds assumptions;
  unsigneds core;
  unsigneds rcore;
  unsigneds eclause;
  unsigneds export_;
  unsigneds klause;
  unsigneds klauses;
  unsigneds resolved;
  unsigneds trail;
  unsigneds units;
  unsigneds prime[2];

  kimits limits;
  int (*terminator) (void *);
  void *terminator_data;
  unsigneds clause;
  uint64_t initialized;
  statistics statistics;
};

/*------------------------------------------------------------------------*/

static inline bool is_core_klause (klause *c) {
  return c->flags & CORE_FLAG;
}

static inline bool is_learned_klause (klause *c) {
  return c->flags & LEARNED_FLAG;
}

static inline void set_core_klause (klause *c) { c->flags |= CORE_FLAG; }

static inline void unset_core_klause (klause *c) { c->flags &= ~CORE_FLAG; }

static inline klause *dereference_klause (cadical_kitten *cadical_kitten, unsigned ref) {
  unsigned *res = BEGIN_STACK (cadical_kitten->klauses) + ref;
  CADICAL_assert (res < END_STACK (cadical_kitten->klauses));
  return (klause *) res;
}

static inline unsigned reference_klause (cadical_kitten *cadical_kitten, const klause *c) {
  const unsigned *const begin = BEGIN_STACK (cadical_kitten->klauses);
  const unsigned *p = (const unsigned *) c;
  CADICAL_assert (begin <= p);
  CADICAL_assert (p < END_STACK (cadical_kitten->klauses));
  const unsigned res = p - begin;
  return res;
}

/*------------------------------------------------------------------------*/

#define KATCHES(KIT) (cadical_kitten->watches[CADICAL_assert ((KIT) < cadical_kitten->lits), (KIT)])

#define all_klauses(C) \
  klause *C = begin_klauses (cadical_kitten), *end_##C = end_klauses (cadical_kitten); \
  (C) != end_##C; \
  (C) = next_klause (cadical_kitten, C)

#define all_original_klauses(C) \
  klause *C = begin_klauses (cadical_kitten), \
         *end_##C = end_original_klauses (cadical_kitten); \
  (C) != end_##C; \
  (C) = next_klause (cadical_kitten, C)

#define all_learned_klauses(C) \
  klause *C = begin_learned_klauses (cadical_kitten), \
         *end_##C = end_klauses (cadical_kitten); \
  (C) != end_##C; \
  (C) = next_klause (cadical_kitten, C)

#define all_kits(KIT) \
  size_t KIT = 0, KIT_##END = cadical_kitten->lits; \
  KIT != KIT_##END; \
  KIT++

#define BEGIN_KLAUSE(C) (C)->lits

#define END_KLAUSE(C) (BEGIN_KLAUSE (C) + (C)->size)

#define all_literals_in_klause(KIT, C) \
  unsigned KIT, *KIT##_PTR = BEGIN_KLAUSE (C), \
                *KIT##_END = END_KLAUSE (C); \
  KIT##_PTR != KIT##_END && ((KIT = *KIT##_PTR), true); \
  ++KIT##_PTR

#define all_antecedents(REF, C) \
  unsigned REF, *REF##_PTR = antecedents (C), \
                *REF##_END = REF##_PTR + (C)->aux; \
  REF##_PTR != REF##_END && ((REF = *REF##_PTR), true); \
  ++REF##_PTR

#ifdef LOGGING

#define logging (cadical_kitten->logging)

static void log_basic (cadical_kitten *, const char *, ...)
    __attribute__ ((format (printf, 2, 3)));

static void log_basic (cadical_kitten *cadical_kitten, const char *fmt, ...) {
  CADICAL_assert (logging);
  printf ("c KITTEN %u ", cadical_kitten->level);
  va_list ap;
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  fputc ('\n', stdout);
  fflush (stdout);
}

static void log_reference (cadical_kitten *, unsigned, const char *, ...)
    __attribute__ ((format (printf, 3, 4)));

static void log_reference (cadical_kitten *cadical_kitten, unsigned ref, const char *fmt,
                           ...) {
  klause *c = dereference_klause (cadical_kitten, ref);
  CADICAL_assert (logging);
  printf ("c KITTEN %u ", cadical_kitten->level);
  va_list ap;
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  if (is_learned_klause (c)) {
    fputs (" learned", stdout);
    if (c->aux)
      printf ("[%u]", c->aux);
  } else {
    fputs (" original", stdout);
    if (c->aux != INVALID)
      printf ("[%u]", c->aux);
  }
  printf (" size %u clause[%u]", c->size, ref);
  value *values = cadical_kitten->values;
  kar *vars = cadical_kitten->vars;
  for (all_literals_in_klause (lit, c)) {
    printf (" %u", lit);
    const value value = values[lit];
    if (value)
      printf ("@%u=%d", vars[lit / 2].level, (int) value);
  }
  fputc ('\n', stdout);
  fflush (stdout);
}

static void log_literals (cadical_kitten *, unsigned *, unsigned, const char *, ...)
    __attribute__ ((format (printf, 4, 5)));

static void log_literals (cadical_kitten *cadical_kitten, unsigned *lits, unsigned size,
                          const char *fmt, ...) {
  CADICAL_assert (logging);
  printf ("c KITTEN %u ", cadical_kitten->level);
  va_list ap;
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  value *values = cadical_kitten->values;
  kar *vars = cadical_kitten->vars;
  for (unsigned i = 0; i < size; i++) {
    const unsigned lit = lits[i];
    printf (" %u", lit);
    const value value = values[lit];
    if (value)
      printf ("@%u=%d", vars[lit / 2].level, (int) value);
  }
  fputc ('\n', stdout);
  fflush (stdout);
}

#define LOG(...) \
  do { \
    if (logging) \
      log_basic (cadical_kitten, __VA_ARGS__); \
  } while (0)

#define ROG(...) \
  do { \
    if (logging) \
      log_reference (cadical_kitten, __VA_ARGS__); \
  } while (0)

#define LOGLITS(...) \
  do { \
    if (logging) \
      log_literals (cadical_kitten, __VA_ARGS__); \
  } while (0)

#else

#define LOG(...) \
  do { \
  } while (0)
#define ROG(...) \
  do { \
  } while (0)
#define LOGLITS(...) \
  do { \
  } while (0)

#endif

static void check_queue (cadical_kitten *cadical_kitten) {
#ifdef CHECK_KITTEN
  const unsigned vars = cadical_kitten->lits / 2;
  unsigned found = 0, prev = INVALID;
  kink *links = cadical_kitten->links;
  uint64_t stamp = 0;
  for (unsigned idx = cadical_kitten->queue.first, next; idx != INVALID;
       idx = next) {
    kink *link = links + idx;
    CADICAL_assert (link->prev == prev);
    CADICAL_assert (!found || stamp < link->stamp);
    CADICAL_assert (link->stamp < cadical_kitten->queue.stamp);
    stamp = link->stamp;
    next = link->next;
    prev = idx;
    found++;
  }
  CADICAL_assert (found == vars);
  unsigned next = INVALID;
  found = 0;
  for (unsigned idx = cadical_kitten->queue.last, prev; idx != INVALID;
       idx = prev) {
    kink *link = links + idx;
    CADICAL_assert (link->next == next);
    prev = link->prev;
    next = idx;
    found++;
  }
  CADICAL_assert (found == vars);
  value *values = cadical_kitten->values;
  bool first = true;
  for (unsigned idx = cadical_kitten->queue.search, next; idx != INVALID;
       idx = next) {
    kink *link = links + idx;
    next = link->next;
    const unsigned lit = 2 * idx;
    CADICAL_assert (first || values[lit]);
    first = false;
  }
#else
  (void) cadical_kitten;
#endif
}

static void update_search (cadical_kitten *cadical_kitten, unsigned idx) {
  if (cadical_kitten->queue.search == idx)
    return;
  cadical_kitten->queue.search = idx;
  LOG ("search updated to %u stamped %" PRIu64, idx,
       cadical_kitten->links[idx].stamp);
}

static void enqueue (cadical_kitten *cadical_kitten, unsigned idx) {
  LOG ("enqueue %u", idx);
  kink *links = cadical_kitten->links;
  kink *l = links + idx;
  const unsigned last = cadical_kitten->queue.last;
  if (last == INVALID)
    cadical_kitten->queue.first = idx;
  else
    links[last].next = idx;
  l->prev = last;
  l->next = INVALID;
  cadical_kitten->queue.last = idx;
  l->stamp = cadical_kitten->queue.stamp++;
  LOG ("stamp %" PRIu64, l->stamp);
}

static void dequeue (cadical_kitten *cadical_kitten, unsigned idx) {
  LOG ("dequeue %u", idx);
  kink *links = cadical_kitten->links;
  kink *l = links + idx;
  const unsigned prev = l->prev;
  const unsigned next = l->next;
  if (prev == INVALID)
    cadical_kitten->queue.first = next;
  else
    links[prev].next = next;
  if (next == INVALID)
    cadical_kitten->queue.last = prev;
  else
    links[next].prev = prev;
}

static void init_queue (cadical_kitten *cadical_kitten, size_t old_vars, size_t new_vars) {
  for (size_t idx = old_vars; idx < new_vars; idx++) {
    CADICAL_assert (!cadical_kitten->values[2 * idx]);
    CADICAL_assert (cadical_kitten->unassigned < UINT_MAX);
    cadical_kitten->unassigned++;
    enqueue (cadical_kitten, idx);
  }
  LOG ("initialized decision queue from %zu to %zu", old_vars, new_vars);
  update_search (cadical_kitten, cadical_kitten->queue.last);
  check_queue (cadical_kitten);
}

static void initialize_cadical_kitten (cadical_kitten *cadical_kitten) {
  cadical_kitten->queue.first = INVALID;
  cadical_kitten->queue.last = INVALID;
  cadical_kitten->inconsistent = INVALID;
  cadical_kitten->failing = INVALID;
  cadical_kitten->queue.search = INVALID;
  cadical_kitten->terminator = 0;
  cadical_kitten->terminator_data = 0;
  cadical_kitten->limits.ticks = UINT64_MAX;
  cadical_kitten->generator = cadical_kitten->initialized++;
}

static void clear_cadical_kitten (cadical_kitten *cadical_kitten) {
  size_t bytes = (char *) &cadical_kitten->size - (char *) &cadical_kitten->status;
  memset (&cadical_kitten->status, 0, bytes);
  memset (&cadical_kitten->statistics, 0, sizeof (statistics));
  initialize_cadical_kitten (cadical_kitten);
}

#define RESIZE1(T, P)                            \
  do { \
    void *OLD_PTR = (P); \
    CALLOC (T, (P), new_size / 2);                \
    const size_t BYTES = old_vars * sizeof *(P); \
    memcpy ((P), OLD_PTR, BYTES); \
    void *NEW_PTR = (P); \
    (P) = (T*)OLD_PTR;           \
    DEALLOC ((P), old_size / 2); \
    (P) = (T*)NEW_PTR;            \
  } while (0)

#define RESIZE2(T, P)                            \
  do { \
    void *OLD_PTR = (P); \
    CALLOC (T, (P), new_size);                    \
    const size_t BYTES = old_lits * sizeof *(P); \
    memcpy ((P), OLD_PTR, BYTES); \
    void *NEW_PTR = (P); \
    (P) = (T*)OLD_PTR;       \
    DEALLOC ((P), old_size); \
    (P) = (T*)NEW_PTR;       \
  } while (0)

static void enlarge_internal (cadical_kitten *cadical_kitten, size_t lit) {
  const size_t new_lits = (lit | 1) + 1;
  const size_t old_lits = cadical_kitten->lits;
  CADICAL_assert (old_lits <= lit);
  CADICAL_assert (old_lits < new_lits);
  CADICAL_assert ((lit ^ 1) < new_lits);
  CADICAL_assert (lit < new_lits);
  const size_t old_size = cadical_kitten->size;
  const unsigned new_vars = new_lits / 2;
  const unsigned old_vars = old_lits / 2;
  if (old_size < new_lits) {
    size_t new_size = old_size ? 2 * old_size : 2;
    while (new_size <= lit)
      new_size *= 2;
    LOG ("internal literals resized to %zu from %zu (requested %zu)",
         new_size, old_size, new_lits);

    RESIZE1 (value, cadical_kitten->marks);
    RESIZE1 (unsigned char, cadical_kitten->phases);
    RESIZE2 (value, cadical_kitten->values);
    RESIZE2 (bool, cadical_kitten->failed);
    RESIZE1 (kar, cadical_kitten->vars);
    RESIZE1 (kink, cadical_kitten->links);
    RESIZE2 (katches, cadical_kitten->watches);

    cadical_kitten->size = new_size;
  }
  cadical_kitten->lits = new_lits;
  init_queue (cadical_kitten, old_vars, new_vars);
  LOG ("internal literals activated until %zu literals", new_lits);
  return;
}

static const char *status_to_string (int status) {
  switch (status) {
  case 10:
    return "formula satisfied";
  case 11:
    return "formula satisfied and prime implicant computed";
  case 20:
    return "formula inconsistent";
  case 21:
    return "formula inconsistent and core computed";
  default:
    CADICAL_assert (!status);
    return "formula unsolved";
  }
}

static void invalid_api_usage (const char *fun, const char *fmt, ...) {
  fprintf (stderr, "cadical_kitten: fatal error: invalid API usage in '%s': ", fun);
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  fflush (stderr);
  abort ();
}

#define INVALID_API_USAGE(...) invalid_api_usage (__func__, __VA_ARGS__)

#define REQUIRE_INITIALIZED() \
  do { \
    if (!cadical_kitten) \
      INVALID_API_USAGE ("solver argument zero"); \
  } while (0)

#define REQUIRE_STATUS(EXPECTED) \
  do { \
    REQUIRE_INITIALIZED (); \
    if (cadical_kitten->status != (EXPECTED)) \
      INVALID_API_USAGE ("invalid status '%s' (expected '%s')", \
                         status_to_string (cadical_kitten->status), \
                         status_to_string (EXPECTED)); \
  } while (0)

#define UPDATE_STATUS(STATUS) \
  do { \
    if (cadical_kitten->status != (STATUS)) \
      LOG ("updating status from '%s' to '%s'", \
           status_to_string (cadical_kitten->status), status_to_string (STATUS)); \
    else \
      LOG ("keeping status at '%s'", status_to_string (STATUS)); \
    cadical_kitten->status = (STATUS); \
  } while (0)

cadical_kitten *cadical_kitten_init (void) {
  cadical_kitten *cadical_kitten;
  CALLOC (struct cadical_kitten, cadical_kitten, 1);
  initialize_cadical_kitten (cadical_kitten);
  return cadical_kitten;
}

#ifdef LOGGING
void cadical_kitten_set_logging (cadical_kitten *cadical_kitten) { logging = true; }
#endif

void cadical_kitten_track_antecedents (cadical_kitten *cadical_kitten) {
  REQUIRE_STATUS (0);

  if (cadical_kitten->learned)
    INVALID_API_USAGE ("can not start tracking antecedents after learning");

  LOG ("enabling antecedents tracking");
  cadical_kitten->antecedents = true;
}

void cadical_kitten_randomize_phases (cadical_kitten *cadical_kitten) {
  REQUIRE_INITIALIZED ();

  LOG ("randomizing phases");

  unsigned char *phases = cadical_kitten->phases;
  const unsigned vars = cadical_kitten->size / 2;

  uint64_t random = kissat_next_random64 (&cadical_kitten->generator);

  unsigned i = 0;
  const unsigned rest = vars & ~63u;

  while (i != rest) {
    uint64_t *p = (uint64_t *) (phases + i);
    p[0] = (random >> 0) & 0x0101010101010101;
    p[1] = (random >> 1) & 0x0101010101010101;
    p[2] = (random >> 2) & 0x0101010101010101;
    p[3] = (random >> 3) & 0x0101010101010101;
    p[4] = (random >> 4) & 0x0101010101010101;
    p[5] = (random >> 5) & 0x0101010101010101;
    p[6] = (random >> 6) & 0x0101010101010101;
    p[7] = (random >> 7) & 0x0101010101010101;
    random = kissat_next_random64 (&cadical_kitten->generator);
    i += 64;
  }

  unsigned shift = 0;
  while (i != vars)
    phases[i++] = (random >> shift++) & 1;
}

void cadical_kitten_flip_phases (cadical_kitten *cadical_kitten) {
  REQUIRE_INITIALIZED ();

  LOG ("flipping phases");

  unsigned char *phases = cadical_kitten->phases;
  const unsigned vars = cadical_kitten->size / 2;

  unsigned i = 0;
  const unsigned rest = vars & ~7u;

  while (i != rest) {
    uint64_t *p = (uint64_t *) (phases + i);
    *p ^= 0x0101010101010101;
    i += 8;
  }

  while (i != vars)
    phases[i++] ^= 1;
}

void cadical_kitten_no_ticks_limit (cadical_kitten *cadical_kitten) {
  REQUIRE_INITIALIZED ();
  LOG ("forcing no ticks limit");
  cadical_kitten->limits.ticks = UINT64_MAX;
}

uint64_t cadical_kitten_current_ticks (cadical_kitten *cadical_kitten) {
  REQUIRE_INITIALIZED ();
  const uint64_t current = KITTEN_TICKS;
  return current;
}

void cadical_kitten_set_ticks_limit (cadical_kitten *cadical_kitten, uint64_t delta) {
  REQUIRE_INITIALIZED ();
  const uint64_t current = KITTEN_TICKS;
  uint64_t limit;
  if (UINT64_MAX - delta <= current) {
    LOG ("forcing unlimited ticks limit");
    limit = UINT64_MAX;
  } else {
    limit = current + delta;
    LOG ("new limit of %" PRIu64 " ticks after %" PRIu64, limit, delta);
  }

  cadical_kitten->limits.ticks = limit;
}

void cadical_kitten_no_terminator (cadical_kitten *cadical_kitten) {
  REQUIRE_INITIALIZED ();
  LOG ("removing terminator");
  cadical_kitten->terminator = 0;
  cadical_kitten->terminator_data = 0;
}

void cadical_kitten_set_terminator (cadical_kitten *cadical_kitten, void *data,
                            int (*terminator) (void *)) {
  REQUIRE_INITIALIZED ();
  LOG ("setting terminator");
  cadical_kitten->terminator = terminator;
  cadical_kitten->terminator_data = data;
}

static void shuffle_unsigned_array (cadical_kitten *cadical_kitten, size_t size,
                                    unsigned *a) {
  for (size_t i = 0; i < size; i++) {
    const size_t j = kissat_pick_random (&cadical_kitten->generator, 0, i);
    if (j == i)
      continue;
    const unsigned first = a[i];
    const unsigned second = a[j];
    a[i] = second;
    a[j] = first;
  }
}

static void shuffle_unsigned_stack (cadical_kitten *cadical_kitten, unsigneds *stack) {
  const size_t size = SIZE_STACK (*stack);
  unsigned *a = BEGIN_STACK (*stack);
  shuffle_unsigned_array (cadical_kitten, size, a);
}

static void shuffle_katches (cadical_kitten *cadical_kitten) {
  LOG ("shuffling watch lists");
  for (size_t lit = 0; lit < cadical_kitten->lits; lit++)
    shuffle_unsigned_stack (cadical_kitten, &KATCHES (lit));
}

static void shuffle_queue (cadical_kitten *cadical_kitten) {
  LOG ("shuffling variable decision order");

  const unsigned vars = cadical_kitten->lits / 2;
  for (unsigned i = 0; i < vars; i++) {
    const unsigned idx = kissat_pick_random (&cadical_kitten->generator, 0, vars);
    dequeue (cadical_kitten, idx);
    enqueue (cadical_kitten, idx);
  }
  update_search (cadical_kitten, cadical_kitten->queue.last);
}

static void shuffle_units (cadical_kitten *cadical_kitten) {
  LOG ("shuffling units");
  shuffle_unsigned_stack (cadical_kitten, &cadical_kitten->units);
}

void cadical_kitten_shuffle_clauses (cadical_kitten *cadical_kitten) {
  REQUIRE_STATUS (0);
  shuffle_queue (cadical_kitten);
  shuffle_katches (cadical_kitten);
  shuffle_units (cadical_kitten);
}

static inline unsigned *antecedents (klause *c) {
  CADICAL_assert (is_learned_klause (c));
  return c->lits + c->size;
}

static inline void watch_klause (cadical_kitten *cadical_kitten, unsigned lit,
                                 unsigned ref) {
  ROG (ref, "watching %u in", lit);
  katches *watches = &KATCHES (lit);
  PUSH_STACK (*watches, ref);
}

static inline void connect_new_klause (cadical_kitten *cadical_kitten, unsigned ref) {
  ROG (ref, "new");

  klause *c = dereference_klause (cadical_kitten, ref);

  if (!c->size) {
    if (cadical_kitten->inconsistent == INVALID) {
      ROG (ref, "registering inconsistent empty");
      cadical_kitten->inconsistent = ref;
    } else
      ROG (ref, "ignoring inconsistent empty");
  } else if (c->size == 1) {
    ROG (ref, "watching unit");
    PUSH_STACK (cadical_kitten->units, ref);
  } else {
    watch_klause (cadical_kitten, c->lits[0], ref);
    watch_klause (cadical_kitten, c->lits[1], ref);
  }
}

static unsigned new_reference (cadical_kitten *cadical_kitten) {
  size_t ref = SIZE_STACK (cadical_kitten->klauses);
  if (ref >= INVALID) {
    die ("maximum number of literals exhausted");
  }
  const unsigned res = (unsigned) ref;
  CADICAL_assert (res != INVALID);
  INC (cadical_kitten_ticks);
  return res;
}

static void new_original_klause (cadical_kitten *cadical_kitten, unsigned id) {
  unsigned res = new_reference (cadical_kitten);
  unsigned size = SIZE_STACK (cadical_kitten->klause);
  unsigneds *klauses = &cadical_kitten->klauses;
  PUSH_STACK (*klauses, id);
  PUSH_STACK (*klauses, size);
  PUSH_STACK (*klauses, 0);
  for (all_stack (unsigned, lit, cadical_kitten->klause))
    PUSH_STACK (*klauses, lit);
  connect_new_klause (cadical_kitten, res);
  cadical_kitten->end_original_ref = SIZE_STACK (*klauses);
  cadical_kitten->statistics.original++;
}

static void enlarge_external (cadical_kitten *cadical_kitten, size_t eidx) {
  const size_t old_size = cadical_kitten->esize;
  const unsigned old_evars = cadical_kitten->evars;
  CADICAL_assert (old_evars <= eidx);
  const unsigned new_evars = eidx + 1;
  if (old_size <= eidx) {
    size_t new_size = old_size ? 2 * old_size : 1;
    while (new_size <= eidx)
      new_size *= 2;
    LOG ("external resizing to %zu variables from %zu (requested %u)",
         new_size, old_size, new_evars);
    unsigned *old_import = cadical_kitten->import;
    CALLOC (unsigned, cadical_kitten->import, new_size);
    const size_t bytes = old_evars * sizeof *cadical_kitten->import;
    memcpy (cadical_kitten->import, old_import, bytes);
    DEALLOC (old_import, old_size);
    cadical_kitten->esize = new_size;
  }
  cadical_kitten->evars = new_evars;
  LOG ("external variables enlarged to %u", new_evars);
}

static unsigned import_literal (cadical_kitten *cadical_kitten, unsigned elit) {
  const unsigned eidx = elit / 2;
  if (eidx >= cadical_kitten->evars)
    enlarge_external (cadical_kitten, eidx);

  unsigned iidx = cadical_kitten->import[eidx];
  if (!iidx) {
    iidx = SIZE_STACK (cadical_kitten->export_);
    PUSH_STACK (cadical_kitten->export_, eidx);
    cadical_kitten->import[eidx] = iidx + 1;
  } else
    iidx--;
  unsigned ilit = 2 * iidx + (elit & 1);
  LOG ("imported external literal %u as internal literal %u", elit, ilit);
  if (ilit >= cadical_kitten->lits)
    enlarge_internal (cadical_kitten, ilit);
  return ilit;
}

static unsigned export_literal (cadical_kitten *cadical_kitten, unsigned ilit) {
  const unsigned iidx = ilit / 2;
  CADICAL_assert (iidx < SIZE_STACK (cadical_kitten->export_));
  const unsigned eidx = PEEK_STACK (cadical_kitten->export_, iidx);
  const unsigned elit = 2 * eidx + (ilit & 1);
  return elit;
}

unsigned cadical_new_learned_klause (cadical_kitten *cadical_kitten) {
  unsigned res = new_reference (cadical_kitten);
  unsigneds *klauses = &cadical_kitten->klauses;
  const size_t size = SIZE_STACK (cadical_kitten->klause);
  CADICAL_assert (size <= UINT_MAX);
  const size_t aux =
      cadical_kitten->antecedents ? SIZE_STACK (cadical_kitten->resolved) : 0;
  CADICAL_assert (aux <= UINT_MAX);
  PUSH_STACK (*klauses, (unsigned) aux);
  PUSH_STACK (*klauses, (unsigned) size);
  PUSH_STACK (*klauses, LEARNED_FLAG);
  for (all_stack (unsigned, lit, cadical_kitten->klause))
    PUSH_STACK (*klauses, lit);
  if (aux)
    for (all_stack (unsigned, ref, cadical_kitten->resolved))
      PUSH_STACK (*klauses, ref);
  connect_new_klause (cadical_kitten, res);
  cadical_kitten->learned = true;
  cadical_kitten->statistics.learned++;
  return res;
}

void cadical_kitten_clear (cadical_kitten *cadical_kitten) {
  LOG ("clear cadical_kitten of size %zu", cadical_kitten->size);

  CADICAL_assert (EMPTY_STACK (cadical_kitten->analyzed));
  CADICAL_assert (EMPTY_STACK (cadical_kitten->eclause));
  CADICAL_assert (EMPTY_STACK (cadical_kitten->resolved));

  CLEAR_STACK (cadical_kitten->assumptions);
  CLEAR_STACK (cadical_kitten->core);
  CLEAR_STACK (cadical_kitten->klause);
  CLEAR_STACK (cadical_kitten->klauses);
  CLEAR_STACK (cadical_kitten->trail);
  CLEAR_STACK (cadical_kitten->units);
  CLEAR_STACK (cadical_kitten->clause);
  CLEAR_STACK (cadical_kitten->prime[0]);
  CLEAR_STACK (cadical_kitten->prime[1]);

  for (all_kits (kit))
    CLEAR_STACK (KATCHES (kit));

  while (!EMPTY_STACK (cadical_kitten->export_))
    cadical_kitten->import[POP_STACK (cadical_kitten->export_)] = 0;

  const size_t lits = cadical_kitten->size;
  const unsigned vars = lits / 2;

#ifndef CADICAL_NDEBUG
  for (unsigned i = 0; i < vars; i++)
    CADICAL_assert (!cadical_kitten->marks[i]);
#endif

  memset (cadical_kitten->phases, 0, vars);
  memset (cadical_kitten->values, 0, lits);
  memset (cadical_kitten->failed, 0, lits);
  memset (cadical_kitten->vars, 0, vars);

  clear_cadical_kitten (cadical_kitten);
}

void cadical_kitten_release (cadical_kitten *cadical_kitten) {
  RELEASE_STACK (cadical_kitten->analyzed);
  RELEASE_STACK (cadical_kitten->assumptions);
  RELEASE_STACK (cadical_kitten->core);
  RELEASE_STACK (cadical_kitten->eclause);
  RELEASE_STACK (cadical_kitten->export_);
  RELEASE_STACK (cadical_kitten->klause);
  RELEASE_STACK (cadical_kitten->klauses);
  RELEASE_STACK (cadical_kitten->resolved);
  RELEASE_STACK (cadical_kitten->trail);
  RELEASE_STACK (cadical_kitten->units);
  RELEASE_STACK (cadical_kitten->clause);
  RELEASE_STACK (cadical_kitten->prime[0]);
  RELEASE_STACK (cadical_kitten->prime[1]);

  for (size_t lit = 0; lit < cadical_kitten->size; lit++)
    RELEASE_STACK (cadical_kitten->watches[lit]);

  const size_t lits = cadical_kitten->size;
  const unsigned vars = lits / 2;
  DEALLOC (cadical_kitten->marks, vars);
  DEALLOC (cadical_kitten->phases, vars);
  DEALLOC (cadical_kitten->values, lits);
  DEALLOC (cadical_kitten->failed, lits);
  DEALLOC (cadical_kitten->vars, vars);
  DEALLOC (cadical_kitten->links, vars);
  DEALLOC (cadical_kitten->watches, lits);
  DEALLOC (cadical_kitten->import, cadical_kitten->esize);
  (void) vars;

  free (cadical_kitten);
}

static inline void move_to_front (cadical_kitten *cadical_kitten, unsigned idx) {
  if (idx == cadical_kitten->queue.last)
    return;
  LOG ("move to front variable %u", idx);
  dequeue (cadical_kitten, idx);
  enqueue (cadical_kitten, idx);
  CADICAL_assert (cadical_kitten->values[2 * idx]);
}

static inline void assign (cadical_kitten *cadical_kitten, unsigned lit, unsigned reason) {
#ifdef LOGGING
  if (reason == INVALID)
    LOG ("assign %u as decision", lit);
  else
    ROG (reason, "assign %u reason", lit);
#endif
  value *values = cadical_kitten->values;
  const unsigned not_lit = lit ^ 1;
  CADICAL_assert (!values[lit]);
  CADICAL_assert (!values[not_lit]);
  values[lit] = 1;
  values[not_lit] = -1;
  const unsigned idx = lit / 2;
  const unsigned sign = lit & 1;
  cadical_kitten->phases[idx] = sign;
  PUSH_STACK (cadical_kitten->trail, lit);
  kar *v = cadical_kitten->vars + idx;
  v->level = cadical_kitten->level;
  if (!v->level) {
    CADICAL_assert (reason != INVALID);
    klause *c = dereference_klause (cadical_kitten, reason);
    if (c->size > 1) {
      if (cadical_kitten->antecedents) {
        PUSH_STACK (cadical_kitten->resolved, reason);
        for (all_literals_in_klause (other, c))
          if (other != lit) {
            const unsigned other_idx = other / 2;
            const unsigned other_ref = cadical_kitten->vars[other_idx].reason;
            CADICAL_assert (other_ref != INVALID);
            PUSH_STACK (cadical_kitten->resolved, other_ref);
          }
      }
      PUSH_STACK (cadical_kitten->klause, lit);
      reason = cadical_new_learned_klause (cadical_kitten);
      CLEAR_STACK (cadical_kitten->resolved);
      CLEAR_STACK (cadical_kitten->klause);
    }
  }
  v->reason = reason;
  CADICAL_assert (cadical_kitten->unassigned);
  cadical_kitten->unassigned--;
}

static inline unsigned propagate_literal (cadical_kitten *cadical_kitten, unsigned lit) {
  LOG ("propagating %u", lit);
  value *values = cadical_kitten->values;
  CADICAL_assert (values[lit] > 0);
  const unsigned not_lit = lit ^ 1;
  katches *watches = cadical_kitten->watches + not_lit;
  unsigned conflict = INVALID;
  unsigned *q = BEGIN_STACK (*watches);
  const unsigned *const end_watches = END_STACK (*watches);
  unsigned const *p = q;
  uint64_t ticks = (((char *) end_watches - (char *) q) >> 7) + 1;
  while (p != end_watches) {
    const unsigned ref = *q++ = *p++;
    klause *c = dereference_klause (cadical_kitten, ref);
    CADICAL_assert (c->size > 1);
    unsigned *lits = c->lits;
    const unsigned other = lits[0] ^ lits[1] ^ not_lit;
    const value other_value = values[other];
    ticks++;
    if (other_value > 0)
      continue;
    value replacement_value = -1;
    unsigned replacement = INVALID;
    const unsigned *const end_lits = lits + c->size;
    unsigned *r;
    for (r = lits + 2; r != end_lits; r++) {
      replacement = *r;
      replacement_value = values[replacement];
      if (replacement_value >= 0)
        break;
    }
    if (replacement_value >= 0) {
      CADICAL_assert (replacement != INVALID);
      ROG (ref, "unwatching %u in", not_lit);
      lits[0] = other;
      lits[1] = replacement;
      *r = not_lit;
      watch_klause (cadical_kitten, replacement, ref);
      q--;
    } else if (other_value < 0) {
      ROG (ref, "conflict");
      INC (cadical_kitten_conflicts);
      conflict = ref;
      break;
    } else {
      CADICAL_assert (!other_value);
      assign (cadical_kitten, other, ref);
    }
  }
  while (p != end_watches)
    *q++ = *p++;
  SET_END_OF_STACK (*watches, q);
  ADD (cadical_kitten_ticks, ticks);
  return conflict;
}

static inline unsigned propagate (cadical_kitten *cadical_kitten) {
  CADICAL_assert (cadical_kitten->inconsistent == INVALID);
  unsigned propagated = 0;
  unsigned conflict = INVALID;
  while (conflict == INVALID &&
         cadical_kitten->propagated < SIZE_STACK (cadical_kitten->trail)) {
    if (cadical_kitten->terminator &&
        cadical_kitten->terminator (cadical_kitten->terminator_data)) {
      break;
    }
    const unsigned lit = PEEK_STACK (cadical_kitten->trail, cadical_kitten->propagated);
    conflict = propagate_literal (cadical_kitten, lit);
    cadical_kitten->propagated++;
    propagated++;
  }
  ADD (cadical_kitten_propagations, propagated);
  return conflict;
}

static void bump (cadical_kitten *cadical_kitten) {
  value *marks = cadical_kitten->marks;
  for (all_stack (unsigned, idx, cadical_kitten->analyzed)) {
    marks[idx] = 0;
    move_to_front (cadical_kitten, idx);
  }
  check_queue (cadical_kitten);
}

static inline void unassign (cadical_kitten *cadical_kitten, value *values, unsigned lit) {
  const unsigned not_lit = lit ^ 1;
  CADICAL_assert (values[lit]);
  CADICAL_assert (values[not_lit]);
  const unsigned idx = lit / 2;
#ifdef LOGGING
  kar *var = cadical_kitten->vars + idx;
  cadical_kitten->level = var->level;
  LOG ("unassign %u", lit);
#endif
  values[lit] = values[not_lit] = 0;
  CADICAL_assert (cadical_kitten->unassigned < cadical_kitten->lits / 2);
  cadical_kitten->unassigned++;
  kink *links = cadical_kitten->links;
  kink *link = links + idx;
  if (link->stamp > links[cadical_kitten->queue.search].stamp)
    update_search (cadical_kitten, idx);
}

static void backtrack (cadical_kitten *cadical_kitten, unsigned jump) {
  check_queue (cadical_kitten);
  CADICAL_assert (jump < cadical_kitten->level);
  LOG ("back%s to level %u",
       (cadical_kitten->level == jump + 1 ? "tracking" : "jumping"), jump);
  kar *vars = cadical_kitten->vars;
  value *values = cadical_kitten->values;
  unsigneds *trail = &cadical_kitten->trail;
  while (!EMPTY_STACK (*trail)) {
    const unsigned lit = TOP_STACK (*trail);
    const unsigned idx = lit / 2;
    const unsigned level = vars[idx].level;
    if (level == jump)
      break;
    (void) POP_STACK (*trail);
    unassign (cadical_kitten, values, lit);
  }
  cadical_kitten->propagated = SIZE_STACK (*trail);
  cadical_kitten->level = jump;
  check_queue (cadical_kitten);
}

void cadical_completely_backtrack_to_root_level (cadical_kitten *cadical_kitten) {
  check_queue (cadical_kitten);
  LOG ("completely backtracking to level 0");
  value *values = cadical_kitten->values;
  unsigneds *trail = &cadical_kitten->trail;
  unsigneds *units = &cadical_kitten->units;
#ifndef CADICAL_NDEBUG
  kar *vars = cadical_kitten->vars;
#endif
  for (all_stack (unsigned, lit, *trail)) {
    CADICAL_assert (vars[lit / 2].level);
    unassign (cadical_kitten, values, lit);
  }
  CLEAR_STACK (*trail);
  for (all_stack (unsigned, ref, *units)) {
    klause *c = dereference_klause (cadical_kitten, ref);
    CADICAL_assert (c->size == 1);
    const unsigned unit = c->lits[0];
    const value value = values[unit];
    if (value <= 0)
      continue;
    unassign (cadical_kitten, values, unit);
  }
  cadical_kitten->propagated = 0;
  cadical_kitten->level = 0;
  check_queue (cadical_kitten);
}

static void analyze (cadical_kitten *cadical_kitten, unsigned conflict) {
  CADICAL_assert (cadical_kitten->level);
  CADICAL_assert (cadical_kitten->inconsistent == INVALID);
  CADICAL_assert (EMPTY_STACK (cadical_kitten->analyzed));
  CADICAL_assert (EMPTY_STACK (cadical_kitten->resolved));
  CADICAL_assert (EMPTY_STACK (cadical_kitten->klause));
  PUSH_STACK (cadical_kitten->klause, INVALID);
  unsigned reason = conflict;
  value *marks = cadical_kitten->marks;
  const kar *const vars = cadical_kitten->vars;
  const unsigned level = cadical_kitten->level;
  unsigned const *p = END_STACK (cadical_kitten->trail);
  unsigned open = 0, jump = 0, size = 1, uip;
  for (;;) {
    CADICAL_assert (reason != INVALID);
    klause *c = dereference_klause (cadical_kitten, reason);
    CADICAL_assert (c);
    ROG (reason, "analyzing");
    PUSH_STACK (cadical_kitten->resolved, reason);
    for (all_literals_in_klause (lit, c)) {
      const unsigned idx = lit / 2;
      if (marks[idx])
        continue;
      CADICAL_assert (cadical_kitten->values[lit] < 0);
      LOG ("analyzed %u", lit);
      marks[idx] = true;
      PUSH_STACK (cadical_kitten->analyzed, idx);
      const kar *const v = vars + idx;
      const unsigned tmp = v->level;
      if (tmp < level) {
        if (tmp > jump) {
          jump = tmp;
          if (size > 1) {
            const unsigned other = PEEK_STACK (cadical_kitten->klause, 1);
            POKE_STACK (cadical_kitten->klause, 1, lit);
            lit = other;
          }
        }
        PUSH_STACK (cadical_kitten->klause, lit);
        size++;
      } else
        open++;
    }
    unsigned idx;
    do {
      CADICAL_assert (BEGIN_STACK (cadical_kitten->trail) < p);
      uip = *--p;
    } while (!marks[idx = uip / 2]);
    CADICAL_assert (open);
    if (!--open)
      break;
    reason = vars[idx].reason;
  }
  const unsigned not_uip = uip ^ 1;
  LOG ("first UIP %u jump level %u size %u", not_uip, jump, size);
  POKE_STACK (cadical_kitten->klause, 0, not_uip);
  bump (cadical_kitten);
  CLEAR_STACK (cadical_kitten->analyzed);
  const unsigned learned_ref = cadical_new_learned_klause (cadical_kitten);
  CLEAR_STACK (cadical_kitten->resolved);
  CLEAR_STACK (cadical_kitten->klause);
  backtrack (cadical_kitten, jump);
  assign (cadical_kitten, not_uip, learned_ref);
}

static void failing (cadical_kitten *cadical_kitten) {
  CADICAL_assert (cadical_kitten->inconsistent == INVALID);
  CADICAL_assert (!EMPTY_STACK (cadical_kitten->assumptions));
  CADICAL_assert (EMPTY_STACK (cadical_kitten->analyzed));
  CADICAL_assert (EMPTY_STACK (cadical_kitten->resolved));
  CADICAL_assert (EMPTY_STACK (cadical_kitten->klause));
  LOG ("analyzing failing assumptions");
  const value *const values = cadical_kitten->values;
  const kar *const vars = cadical_kitten->vars;
  unsigned failed_clashing = INVALID;
  unsigned first_failed = INVALID;
  unsigned failed_unit = INVALID;
  for (all_stack (unsigned, lit, cadical_kitten->assumptions)) {
    if (values[lit] >= 0)
      continue;
    if (first_failed == INVALID)
      first_failed = lit;
    const unsigned failed_idx = lit / 2;
    const kar *const failed_var = vars + failed_idx;
    if (!failed_var->level) {
      failed_unit = lit;
      break;
    }
    if (failed_clashing == INVALID && failed_var->reason == INVALID)
      failed_clashing = lit;
  }
  unsigned failed;
  if (failed_unit != INVALID)
    failed = failed_unit;
  else if (failed_clashing != INVALID)
    failed = failed_clashing;
  else
    failed = first_failed;
  CADICAL_assert (failed != INVALID);
  const unsigned failed_idx = failed / 2;
  const kar *const failed_var = vars + failed_idx;
  const unsigned failed_reason = failed_var->reason;
  LOG ("first failed assumption %u", failed);
  cadical_kitten->failed[failed] = true;

  if (failed_unit != INVALID) {
    CADICAL_assert (dereference_klause (cadical_kitten, failed_reason)->size == 1);
    LOG ("root-level falsified assumption %u", failed);
    cadical_kitten->failing = failed_reason;
    ROG (cadical_kitten->failing, "failing reason");
    return;
  }

  const unsigned not_failed = failed ^ 1;
  if (failed_clashing != INVALID) {
    LOG ("clashing with negated assumption %u", not_failed);
    cadical_kitten->failed[not_failed] = true;
    CADICAL_assert (cadical_kitten->failing == INVALID);
    return;
  }

  value *marks = cadical_kitten->marks;
  CADICAL_assert (!marks[failed_idx]);
  marks[failed_idx] = true;
  PUSH_STACK (cadical_kitten->analyzed, failed_idx);
  PUSH_STACK (cadical_kitten->klause, not_failed);

  unsigneds work;
  INIT_STACK (work);

  LOGLITS (BEGIN_STACK (cadical_kitten->trail), SIZE_STACK (cadical_kitten->trail),
           "trail");

  CADICAL_assert (SIZE_STACK (cadical_kitten->trail));
  unsigned const *p = END_STACK (cadical_kitten->trail);
  unsigned open = 1;
  for (;;) {
    if (!open)
      break;
    open--;
    unsigned idx, uip;
    do {
      CADICAL_assert (BEGIN_STACK (cadical_kitten->trail) < p);
      uip = *--p;
    } while (!marks[idx = uip / 2]);

    const kar *var = vars + idx;
    const unsigned reason = var->reason;
    if (reason == INVALID) {
      unsigned lit = 2 * idx;
      if (values[lit] < 0)
        lit ^= 1;
      LOG ("failed assumption %u", lit);
      CADICAL_assert (!cadical_kitten->failed[lit]);
      cadical_kitten->failed[lit] = true;
      const unsigned not_lit = lit ^ 1;
      PUSH_STACK (cadical_kitten->klause, not_lit);
    } else {
      ROG (reason, "analyzing");
      PUSH_STACK (cadical_kitten->resolved, reason);
      klause *c = dereference_klause (cadical_kitten, reason);
      for (all_literals_in_klause (other, c)) {
        const unsigned other_idx = other / 2;
        if (marks[other_idx])
          continue;
        CADICAL_assert (other_idx != idx);
        marks[other_idx] = true;
        CADICAL_assert (values[other]);
        if (vars[other_idx].level)
          open++;
        else
          PUSH_STACK (work, other_idx);
        PUSH_STACK (cadical_kitten->analyzed, other_idx);

        LOG ("analyzing final literal %u", other ^ 1);
      }
    }
  }
  for (size_t next = 0; next < SIZE_STACK (work); next++) {
    const unsigned idx = PEEK_STACK (work, next);
    const kar *var = vars + idx;
    const unsigned reason = var->reason;
    if (reason == INVALID) {
      unsigned lit = 2 * idx;
      if (values[lit] < 0)
        lit ^= 1;
      LOG ("failed assumption %u", lit);
      CADICAL_assert (!cadical_kitten->failed[lit]);
      cadical_kitten->failed[lit] = true;
      const unsigned not_lit = lit ^ 1;
      PUSH_STACK (cadical_kitten->klause, not_lit);
    } else {
      ROG (reason, "analyzing unit");
      PUSH_STACK (cadical_kitten->resolved, reason);
    }
  }

  // this is bfs not dfs so it does not work for lrat :/
  /*
  for (size_t next = 0; next < SIZE_STACK (cadical_kitten->analyzed); next++) {
    const unsigned idx = PEEK_STACK (cadical_kitten->analyzed, next);
    CADICAL_assert (marks[idx]);
    const kar *var = vars + idx;
    const unsigned reason = var->reason;
    if (reason == INVALID) {
      unsigned lit = 2 * idx;
      if (values[lit] < 0)
        lit ^= 1;
      LOG ("failed assumption %u", lit);
      CADICAL_assert (!cadical_kitten->failed[lit]);
      cadical_kitten->failed[lit] = true;
      const unsigned not_lit = lit ^ 1;
      PUSH_STACK (cadical_kitten->klause, not_lit);
    } else {
      ROG (reason, "analyzing");
      PUSH_STACK (cadical_kitten->resolved, reason);
      klause *c = dereference_klause (cadical_kitten, reason);
      for (all_literals_in_klause (other, c)) {
        const unsigned other_idx = other / 2;
        if (other_idx == idx)
          continue;
        if (marks[other_idx])
          continue;
        marks[other_idx] = true;
        PUSH_STACK (cadical_kitten->analyzed, other_idx);
        LOG ("analyzing final literal %u", other ^ 1);
      }
    }
  }
  */

  for (all_stack (unsigned, idx, cadical_kitten->analyzed))
    CADICAL_assert (marks[idx]), marks[idx] = 0;
  CLEAR_STACK (cadical_kitten->analyzed);

  RELEASE_STACK (work);

  const size_t resolved = SIZE_STACK (cadical_kitten->resolved);
  CADICAL_assert (resolved);

  if (resolved == 1) {
    cadical_kitten->failing = PEEK_STACK (cadical_kitten->resolved, 0);
    ROG (cadical_kitten->failing, "reusing as core");
  } else {
    cadical_kitten->failing = cadical_new_learned_klause (cadical_kitten);
    ROG (cadical_kitten->failing, "new core");
  }

  CLEAR_STACK (cadical_kitten->resolved);
  CLEAR_STACK (cadical_kitten->klause);
}

static void flush_trail (cadical_kitten *cadical_kitten) {
  unsigneds *trail = &cadical_kitten->trail;
  LOG ("flushing %zu root-level literals from trail", SIZE_STACK (*trail));
  CADICAL_assert (!cadical_kitten->level);
  cadical_kitten->propagated = 0;
  CLEAR_STACK (*trail);
}

static int decide (cadical_kitten *cadical_kitten) {
  if (!cadical_kitten->level && !EMPTY_STACK (cadical_kitten->trail))
    flush_trail (cadical_kitten);

  const value *const values = cadical_kitten->values;
  unsigned decision = INVALID;
  const size_t assumptions = SIZE_STACK (cadical_kitten->assumptions);
  while (cadical_kitten->level < assumptions) {
    unsigned assumption = PEEK_STACK (cadical_kitten->assumptions, cadical_kitten->level);
    value value = values[assumption];
    if (value < 0) {
      LOG ("found failing assumption %u", assumption);
      failing (cadical_kitten);
      return 20;
    } else if (value > 0) {

      cadical_kitten->level++;
      LOG ("pseudo decision level %u for already satisfied assumption %u",
           cadical_kitten->level, assumption);
    } else {
      decision = assumption;
      LOG ("using assumption %u as decision", decision);
      break;
    }
  }

  if (!cadical_kitten->unassigned)
    return 10;

  if (KITTEN_TICKS >= cadical_kitten->limits.ticks) {
    LOG ("ticks limit %" PRIu64 " hit after %" PRIu64 " ticks",
         cadical_kitten->limits.ticks, KITTEN_TICKS);
    return -1;
  }

  if (cadical_kitten->terminator && cadical_kitten->terminator (cadical_kitten->terminator_data)) {
    LOG ("terminator requested termination");
    return -1;
  }

  if (decision == INVALID) {
    unsigned idx = cadical_kitten->queue.search;
    const kink *const links = cadical_kitten->links;
    for (;;) {
      CADICAL_assert (idx != INVALID);
      if (!values[2 * idx])
        break;
      idx = links[idx].prev;
    }
    update_search (cadical_kitten, idx);
    const unsigned phase = cadical_kitten->phases[idx];
    decision = 2 * idx + phase;
    LOG ("decision %u variable %u phase %u", decision, idx, phase);
  }
  INC (cadical_kitten_decisions);
  cadical_kitten->level++;
  assign (cadical_kitten, decision, INVALID);
  return 0;
}

static void inconsistent (cadical_kitten *cadical_kitten, unsigned ref) {
  CADICAL_assert (ref != INVALID);
  CADICAL_assert (cadical_kitten->inconsistent == INVALID);

  if (!cadical_kitten->antecedents) {
    cadical_kitten->inconsistent = ref;
    ROG (ref, "registering inconsistent virtually empty");
    return;
  }

  unsigneds *analyzed = &cadical_kitten->analyzed;
  unsigneds *resolved = &cadical_kitten->resolved;

  CADICAL_assert (EMPTY_STACK (*analyzed));
  CADICAL_assert (EMPTY_STACK (*resolved));

  value *marks = cadical_kitten->marks;
  const kar *const vars = cadical_kitten->vars;
  unsigned next = 0;

  for (;;) {
    CADICAL_assert (ref != INVALID);
    klause *c = dereference_klause (cadical_kitten, ref);
    CADICAL_assert (c);
    ROG (ref, "analyzing inconsistent");
    PUSH_STACK (*resolved, ref);
    for (all_literals_in_klause (lit, c)) {
      const unsigned idx = lit / 2;
      CADICAL_assert (!vars[idx].level);
      if (marks[idx])
        continue;
      CADICAL_assert (cadical_kitten->values[lit] < 0);
      LOG ("analyzed %u", lit);
      marks[idx] = true;
      PUSH_STACK (cadical_kitten->analyzed, idx);
    }
    if (next == SIZE_STACK (cadical_kitten->analyzed))
      break;
    const unsigned idx = PEEK_STACK (cadical_kitten->analyzed, next);
    next++;
    const kar *const v = vars + idx;
    CADICAL_assert (!v->level);
    ref = v->reason;
  }
  CADICAL_assert (EMPTY_STACK (cadical_kitten->klause));
  ref = cadical_new_learned_klause (cadical_kitten);
  ROG (ref, "registering final inconsistent empty");
  cadical_kitten->inconsistent = ref;

  for (all_stack (unsigned, idx, *analyzed))
    marks[idx] = 0;

  CLEAR_STACK (*analyzed);
  CLEAR_STACK (*resolved);
}

static int propagate_units (cadical_kitten *cadical_kitten) {
  if (cadical_kitten->inconsistent != INVALID)
    return 20;

  if (EMPTY_STACK (cadical_kitten->units)) {
    LOG ("no root level unit clauses");
    return 0;
  }

  LOG ("propagating %zu root level unit clauses",
       SIZE_STACK (cadical_kitten->units));

  const value *const values = cadical_kitten->values;

  for (size_t next = 0; next < SIZE_STACK (cadical_kitten->units); next++) {
    const unsigned ref = PEEK_STACK (cadical_kitten->units, next);
    CADICAL_assert (ref != INVALID);
    klause *c = dereference_klause (cadical_kitten, ref);
    CADICAL_assert (c->size == 1);
    ROG (ref, "propagating unit");
    const unsigned unit = c->lits[0];
    const value value = values[unit];
    if (value > 0)
      continue;
    if (value < 0) {
      inconsistent (cadical_kitten, ref);
      return 20;
    }
    assign (cadical_kitten, unit, ref);
  }
  const unsigned conflict = propagate (cadical_kitten);
  if (conflict == INVALID)
    return 0;
  inconsistent (cadical_kitten, conflict);
  return 20;
}

/*------------------------------------------------------------------------*/

static klause *begin_klauses (cadical_kitten *cadical_kitten) {
  return (klause *) BEGIN_STACK (cadical_kitten->klauses);
}

static klause *end_original_klauses (cadical_kitten *cadical_kitten) {
  return (klause *) (BEGIN_STACK (cadical_kitten->klauses) +
                     cadical_kitten->end_original_ref);
}

static klause *end_klauses (cadical_kitten *cadical_kitten) {
  return (klause *) END_STACK (cadical_kitten->klauses);
}

static klause *next_klause (cadical_kitten *cadical_kitten, klause *c) {
  CADICAL_assert (begin_klauses (cadical_kitten) <= c);
  CADICAL_assert (c < end_klauses (cadical_kitten));
  unsigned *res = c->lits + c->size;
  if (cadical_kitten->antecedents && is_learned_klause (c))
    res += c->aux;
  return (klause *) res;
}

/*------------------------------------------------------------------------*/

static void reset_core (cadical_kitten *cadical_kitten) {
  LOG ("resetting core clauses");
  size_t reset = 0;
  for (all_klauses (c))
    if (is_core_klause (c))
      unset_core_klause (c), reset++;
  LOG ("reset %zu core clauses", reset);
  CLEAR_STACK (cadical_kitten->core);
}

static void reset_assumptions (cadical_kitten *cadical_kitten) {
  LOG ("reset %zu assumptions", SIZE_STACK (cadical_kitten->assumptions));
  while (!EMPTY_STACK (cadical_kitten->assumptions)) {
    const unsigned assumption = POP_STACK (cadical_kitten->assumptions);
    cadical_kitten->failed[assumption] = false;
  }
#ifndef CADICAL_NDEBUG
  for (size_t i = 0; i < cadical_kitten->size; i++)
    CADICAL_assert (!cadical_kitten->failed[i]);
#endif
  CLEAR_STACK (cadical_kitten->assumptions);
  if (cadical_kitten->failing != INVALID) {
    ROG (cadical_kitten->failing, "reset failed assumption reason");
    cadical_kitten->failing = INVALID;
  }
}

static void reset_incremental (cadical_kitten *cadical_kitten) {
  // if (cadical_kitten->level)
  cadical_completely_backtrack_to_root_level (cadical_kitten);
  if (!EMPTY_STACK (cadical_kitten->assumptions))
    reset_assumptions (cadical_kitten);
  else
    CADICAL_assert (cadical_kitten->failing == INVALID);
  if (cadical_kitten->status == 21)
    reset_core (cadical_kitten);
  UPDATE_STATUS (0);
}

/*------------------------------------------------------------------------*/

static bool flip_literal (cadical_kitten *cadical_kitten, unsigned lit) {
  INC (cadical_kitten_flip);
  signed char *values = cadical_kitten->values;
  if (values[lit] < 0)
    lit ^= 1;
  LOG ("trying to flip value of satisfied literal %u", lit);
  CADICAL_assert (values[lit] > 0);
  katches *watches = cadical_kitten->watches + lit;
  unsigned *q = BEGIN_STACK (*watches);
  const unsigned *const end_watches = END_STACK (*watches);
  unsigned const *p = q;
  uint64_t ticks = (((char *) end_watches - (char *) q) >> 7) + 1;
  bool res = true;
  while (p != end_watches) {
    const unsigned ref = *q++ = *p++;
    klause *c = dereference_klause (cadical_kitten, ref);
    unsigned *lits = c->lits;
    const unsigned other = lits[0] ^ lits[1] ^ lit;
    const value other_value = values[other];
    ticks++;
    if (other_value > 0)
      continue;
    value replacement_value = -1;
    unsigned replacement = INVALID;
    const unsigned *const end_lits = lits + c->size;
    unsigned *r;
    for (r = lits + 2; r != end_lits; r++) {
      replacement = *r;
      CADICAL_assert (replacement != lit);
      replacement_value = values[replacement];
      CADICAL_assert (replacement_value);
      if (replacement_value > 0)
        break;
    }
    if (replacement_value > 0) {
      CADICAL_assert (replacement != INVALID);
      ROG (ref, "unwatching %u in", lit);
      lits[0] = other;
      lits[1] = replacement;
      *r = lit;
      watch_klause (cadical_kitten, replacement, ref);
      q--;
    } else {
      CADICAL_assert (replacement_value < 0);
      ROG (ref, "single satisfied");
      res = false;
      break;
    }
  }
  while (p != end_watches)
    *q++ = *p++;
  SET_END_OF_STACK (*watches, q);
  ADD (cadical_kitten_ticks, ticks);
  if (res) {
    LOG ("flipping value of %u", lit);
    values[lit] = -1;
    const unsigned not_lit = lit ^ 1;
    values[not_lit] = 1;
    INC (cadical_kitten_flipped);
  } else
    LOG ("failed to flip value of %u", lit);
  return res;
}

/*------------------------------------------------------------------------*/

// this cadical specific clause addition avoids copying clauses multiple
// times just to convert literals to unsigned representation.
//
static unsigned int2u (int lit) {
  CADICAL_assert (lit != 0);
  int idx = abs (lit) - 1;
  return (lit < 0) + 2u * (unsigned) idx;
}

void cadical_kitten_assume (cadical_kitten *cadical_kitten, unsigned elit) {
  REQUIRE_INITIALIZED ();
  if (cadical_kitten->status)
    reset_incremental (cadical_kitten);
  const unsigned ilit = import_literal (cadical_kitten, elit);
  LOG ("registering assumption %u", ilit);
  PUSH_STACK (cadical_kitten->assumptions, ilit);
}

void cadical_kitten_assume_signed (cadical_kitten *cadical_kitten, int elit) {
  unsigned kelit = int2u (elit);
  cadical_kitten_assume (cadical_kitten, kelit);
}

void cadical_kitten_clause_with_id_and_exception (cadical_kitten *cadical_kitten, unsigned id,
                                          size_t size,
                                          const unsigned *elits,
                                          unsigned except) {
  REQUIRE_INITIALIZED ();
  if (cadical_kitten->status)
    reset_incremental (cadical_kitten);
  CADICAL_assert (EMPTY_STACK (cadical_kitten->klause));
  const unsigned *const end = elits + size;
  for (const unsigned *p = elits; p != end; p++) {
    const unsigned elit = *p;
    if (elit == except)
      continue;
    const unsigned ilit = import_literal (cadical_kitten, elit);
    CADICAL_assert (ilit < cadical_kitten->lits);
    const unsigned iidx = ilit / 2;
    if (cadical_kitten->marks[iidx])
      INVALID_API_USAGE ("variable '%u' of literal '%u' occurs twice",
                         elit / 2, elit);
    cadical_kitten->marks[iidx] = true;
    PUSH_STACK (cadical_kitten->klause, ilit);
  }
  for (unsigned *p = cadical_kitten->klause.begin; p != cadical_kitten->klause.end; p++)
    cadical_kitten->marks[*p / 2] = false;
  new_original_klause (cadical_kitten, id);
  CLEAR_STACK (cadical_kitten->klause);
}

void citten_clause_with_id_and_exception (cadical_kitten *cadical_kitten, unsigned id,
                                          size_t size, const int *elits,
                                          unsigned except) {
  REQUIRE_INITIALIZED ();
  if (cadical_kitten->status)
    reset_incremental (cadical_kitten);
  CADICAL_assert (EMPTY_STACK (cadical_kitten->klause));
  const int *const end = elits + size;
  for (const int *p = elits; p != end; p++) {
    const unsigned elit = int2u (*p); // this is the conversion
    if (elit == except)
      continue;
    const unsigned ilit = import_literal (cadical_kitten, elit);
    CADICAL_assert (ilit < cadical_kitten->lits);
    const unsigned iidx = ilit / 2;
    if (cadical_kitten->marks[iidx])
      INVALID_API_USAGE ("variable '%u' of literal '%u' occurs twice",
                         elit / 2, elit);
    cadical_kitten->marks[iidx] = true;
    PUSH_STACK (cadical_kitten->klause, ilit);
  }
  for (unsigned *p = cadical_kitten->klause.begin; p != cadical_kitten->klause.end; p++)
    cadical_kitten->marks[*p / 2] = false;
  new_original_klause (cadical_kitten, id);
  CLEAR_STACK (cadical_kitten->klause);
}

void citten_clause_with_id_and_equivalence (cadical_kitten *cadical_kitten, unsigned id,
                                            size_t size, const int *elits,
                                            unsigned lit, unsigned other) {
  REQUIRE_INITIALIZED ();
  if (cadical_kitten->status)
    reset_incremental (cadical_kitten);
  CADICAL_assert (EMPTY_STACK (cadical_kitten->klause));
  bool sat = false;
  const int *const end = elits + size;
  for (const int *p = elits; p != end; p++) {
    const unsigned elit = int2u (*p); // this is the conversion
    if (elit == (lit ^ 1u) || elit == (other ^ 1u))
      continue;
    if (elit == lit || elit == other) {
      sat = true;
      break;
    }
    const unsigned ilit = import_literal (cadical_kitten, elit);
    CADICAL_assert (ilit < cadical_kitten->lits);
    const unsigned iidx = ilit / 2;
    if (cadical_kitten->marks[iidx])
      INVALID_API_USAGE ("variable '%u' of literal '%u' occurs twice",
                         elit / 2, elit);
    cadical_kitten->marks[iidx] = true;
    PUSH_STACK (cadical_kitten->klause, ilit);
  }
  for (unsigned *p = cadical_kitten->klause.begin; p != cadical_kitten->klause.end; p++)
    cadical_kitten->marks[*p / 2] = false;
  if (!sat)
    new_original_klause (cadical_kitten, id);
  CLEAR_STACK (cadical_kitten->klause);
}

void cadical_kitten_clause (cadical_kitten *cadical_kitten, size_t size, unsigned *elits) {
  cadical_kitten_clause_with_id_and_exception (cadical_kitten, INVALID, size, elits,
                                       INVALID);
}

void citten_clause_with_id (cadical_kitten *cadical_kitten, unsigned id, size_t size,
                            int *elits) {
  citten_clause_with_id_and_exception (cadical_kitten, id, size, elits, INVALID);
}

void cadical_kitten_unit (cadical_kitten *cadical_kitten, unsigned lit) {
  cadical_kitten_clause (cadical_kitten, 1, &lit);
}

void cadical_kitten_binary (cadical_kitten *cadical_kitten, unsigned a, unsigned b) {
  unsigned clause[2] = {a, b};
  cadical_kitten_clause (cadical_kitten, 2, clause);
}

int cadical_kitten_solve (cadical_kitten *cadical_kitten) {
  REQUIRE_INITIALIZED ();
  if (cadical_kitten->status)
    reset_incremental (cadical_kitten);
  else // if (cadical_kitten->level)
    cadical_completely_backtrack_to_root_level (cadical_kitten);

  LOG ("starting solving under %zu assumptions",
       SIZE_STACK (cadical_kitten->assumptions));

  INC (cadical_kitten_solved);

  int res = propagate_units (cadical_kitten);
  while (!res) {
    const unsigned conflict = propagate (cadical_kitten);
    if (cadical_kitten->terminator &&
        cadical_kitten->terminator (cadical_kitten->terminator_data)) {
      LOG ("terminator requested termination");
      res = -1;
      break;
    }
    if (conflict != INVALID) {
      if (cadical_kitten->level)
        analyze (cadical_kitten, conflict);
      else {
        inconsistent (cadical_kitten, conflict);
        res = 20;
      }
    } else
      res = decide (cadical_kitten);
  }

  if (res < 0)
    res = 0;

  if (!res && !EMPTY_STACK (cadical_kitten->assumptions))
    reset_assumptions (cadical_kitten);

  UPDATE_STATUS (res);

  if (res == 10)
    INC (cadical_kitten_sat);
  else if (res == 20)
    INC (cadical_kitten_unsat);
  else
    INC (cadical_kitten_unknown);

  LOG ("finished solving with result %d", res);

  return res;
}

int cadical_kitten_status (cadical_kitten *cadical_kitten) { return cadical_kitten->status; }

unsigned cadical_kitten_compute_clausal_core (cadical_kitten *cadical_kitten,
                                      uint64_t *learned_ptr) {
  REQUIRE_STATUS (20);

  if (!cadical_kitten->antecedents)
    INVALID_API_USAGE ("antecedents not tracked");

  LOG ("computing clausal core");

  unsigneds *resolved = &cadical_kitten->resolved;
  CADICAL_assert (EMPTY_STACK (*resolved));

  unsigned original = 0;
  uint64_t learned = 0;

  unsigned reason_ref = cadical_kitten->inconsistent;

  if (reason_ref == INVALID) {
    CADICAL_assert (!EMPTY_STACK (cadical_kitten->assumptions));
    reason_ref = cadical_kitten->failing;
    if (reason_ref == INVALID) {
      LOG ("assumptions mutually inconsistent");
      //goto DONE;
        if (learned_ptr)
          *learned_ptr = learned;
        
        LOG ("clausal core of %u original clauses", original);
        LOG ("clausal core of %" PRIu64 " learned clauses", learned);
        cadical_kitten->statistics.original = original;
        cadical_kitten->statistics.learned = 0;
        UPDATE_STATUS (21);
        
        return original;
    }
  }

  PUSH_STACK (*resolved, reason_ref);
  unsigneds *core = &cadical_kitten->core;
  CADICAL_assert (EMPTY_STACK (*core));

  while (!EMPTY_STACK (*resolved)) {
    const unsigned c_ref = POP_STACK (*resolved);
    if (c_ref == INVALID) {
      const unsigned d_ref = POP_STACK (*resolved);
      ROG (d_ref, "core[%zu]", SIZE_STACK (*core));
      PUSH_STACK (*core, d_ref);
      klause *d = dereference_klause (cadical_kitten, d_ref);
      CADICAL_assert (!is_core_klause (d));
      set_core_klause (d);
      if (is_learned_klause (d))
        learned++;
      else
        original++;
    } else {
      klause *c = dereference_klause (cadical_kitten, c_ref);
      if (is_core_klause (c))
        continue;
      PUSH_STACK (*resolved, c_ref);
      PUSH_STACK (*resolved, INVALID);
      ROG (c_ref, "analyzing antecedent core");
      if (!is_learned_klause (c))
        continue;
      for (all_antecedents (d_ref, c)) {
        klause *d = dereference_klause (cadical_kitten, d_ref);
        if (!is_core_klause (d))
          PUSH_STACK (*resolved, d_ref);
      }
    }
  }

  //DONE:

  if (learned_ptr)
    *learned_ptr = learned;

  LOG ("clausal core of %u original clauses", original);
  LOG ("clausal core of %" PRIu64 " learned clauses", learned);
  cadical_kitten->statistics.original = original;
  cadical_kitten->statistics.learned = 0;
  UPDATE_STATUS (21);

  return original;
}

void cadical_kitten_traverse_core_ids (cadical_kitten *cadical_kitten, void *state,
                               void (*traverse) (void *, unsigned)) {
  REQUIRE_STATUS (21);

  LOG ("traversing core of original clauses");

  unsigned traversed = 0;

  for (all_original_klauses (c)) {
    // only happens for 'true' incremental calls, i.e. if add happens after
    // solve
    if (is_learned_klause (c))
      continue;
    if (!is_core_klause (c))
      continue;
    ROG (reference_klause (cadical_kitten, c), "traversing");
    traverse (state, c->aux);
    traversed++;
  }

  LOG ("traversed %u original core clauses", traversed);
  (void) traversed;

  CADICAL_assert (cadical_kitten->status == 21);
}

void cadical_kitten_traverse_core_clauses (cadical_kitten *cadical_kitten, void *state,
                                   void (*traverse) (void *, bool, size_t,
                                                     const unsigned *)) {
  REQUIRE_STATUS (21);

  LOG ("traversing clausal core");

  unsigned traversed = 0;

  for (all_stack (unsigned, c_ref, cadical_kitten->core)) {
    klause *c = dereference_klause (cadical_kitten, c_ref);
    CADICAL_assert (is_core_klause (c));
    const bool learned = is_learned_klause (c);
    unsigneds *eclause = &cadical_kitten->eclause;
    CADICAL_assert (EMPTY_STACK (*eclause));
    for (all_literals_in_klause (ilit, c)) {
      const unsigned elit = export_literal (cadical_kitten, ilit);
      PUSH_STACK (*eclause, elit);
    }
    const size_t size = SIZE_STACK (*eclause);
    const unsigned *elits = eclause->begin;
    ROG (reference_klause (cadical_kitten, c), "traversing");
    traverse (state, learned, size, elits);
    CLEAR_STACK (*eclause);
    traversed++;
  }

  LOG ("traversed %u core clauses", traversed);
  (void) traversed;

  CADICAL_assert (cadical_kitten->status == 21);
}

void cadical_kitten_traverse_core_clauses_with_id (
    cadical_kitten *cadical_kitten, void *state,
    void (*traverse) (void *state, unsigned, bool learned, size_t,
                      const unsigned *)) {
  REQUIRE_STATUS (21);

  LOG ("traversing clausal core");

  unsigned traversed = 0;

  for (all_stack (unsigned, c_ref, cadical_kitten->core)) {
    klause *c = dereference_klause (cadical_kitten, c_ref);
    CADICAL_assert (is_core_klause (c));
    const bool learned = is_learned_klause (c);
    unsigneds *eclause = &cadical_kitten->eclause;
    CADICAL_assert (EMPTY_STACK (*eclause));
    for (all_literals_in_klause (ilit, c)) {
      const unsigned elit = export_literal (cadical_kitten, ilit);
      PUSH_STACK (*eclause, elit);
    }
    const size_t size = SIZE_STACK (*eclause);
    const unsigned *elits = eclause->begin;
    ROG (reference_klause (cadical_kitten, c), "traversing");
    unsigned ctag = learned ? 0 : c->aux;
    traverse (state, ctag, learned, size, elits);
    CLEAR_STACK (*eclause);
    traversed++;
  }

  LOG ("traversed %u core clauses", traversed);
  (void) traversed;

  CADICAL_assert (cadical_kitten->status == 21);
}

void cadical_kitten_trace_core (cadical_kitten *cadical_kitten, void *state,
                        void (*trace) (void *, unsigned, unsigned, bool,
                                       size_t, const unsigned *, size_t,
                                       const unsigned *)) {
  REQUIRE_STATUS (21);

  LOG ("tracing clausal core");

  unsigned traced = 0;

  for (all_stack (unsigned, c_ref, cadical_kitten->core)) {
    klause *c = dereference_klause (cadical_kitten, c_ref);
    CADICAL_assert (is_core_klause (c));
    const bool learned = is_learned_klause (c);
    unsigneds *eclause = &cadical_kitten->eclause;
    CADICAL_assert (EMPTY_STACK (*eclause));
    for (all_literals_in_klause (ilit, c)) {
      const unsigned elit = export_literal (cadical_kitten, ilit);
      PUSH_STACK (*eclause, elit);
    }
    const size_t size = SIZE_STACK (*eclause);
    const unsigned *elits = eclause->begin;

    unsigneds *resolved = &cadical_kitten->resolved;
    CADICAL_assert (EMPTY_STACK (*resolved));
    if (learned) {
      for (all_antecedents (ref, c)) {
        PUSH_STACK (*resolved, ref);
      }
    }
    const size_t rsize = SIZE_STACK (*resolved);
    const unsigned *rids = resolved->begin;

    unsigned cid = reference_klause (cadical_kitten, c);
    unsigned ctag = learned ? 0 : c->aux;
    ROG (cid, "tracing");
    trace (state, cid, ctag, learned, size, elits, rsize, rids);
    CLEAR_STACK (*eclause);
    CLEAR_STACK (*resolved);
    traced++;
  }

  LOG ("traced %u core clauses", traced);
  (void) traced;

  CADICAL_assert (cadical_kitten->status == 21);
}

void cadical_kitten_shrink_to_clausal_core (cadical_kitten *cadical_kitten) {
  REQUIRE_STATUS (21);

  LOG ("shrinking formula to core of original clauses");

  CLEAR_STACK (cadical_kitten->trail);

  cadical_kitten->unassigned = cadical_kitten->lits / 2;
  cadical_kitten->propagated = 0;
  cadical_kitten->level = 0;

  update_search (cadical_kitten, cadical_kitten->queue.last);

  memset (cadical_kitten->values, 0, cadical_kitten->lits);

  for (all_kits (lit))
    CLEAR_STACK (KATCHES (lit));

  CADICAL_assert (cadical_kitten->inconsistent != INVALID);
  klause *inconsistent = dereference_klause (cadical_kitten, cadical_kitten->inconsistent);
  if (is_learned_klause (inconsistent) || inconsistent->size) {
    ROG (cadical_kitten->inconsistent, "resetting inconsistent");
    cadical_kitten->inconsistent = INVALID;
  } else
    ROG (cadical_kitten->inconsistent, "keeping inconsistent");

  CLEAR_STACK (cadical_kitten->units);

  klause *begin = begin_klauses (cadical_kitten), *q = begin;
  klause const *const end = end_original_klauses (cadical_kitten);
#ifdef LOGGING
  unsigned original = 0;
#endif
  for (klause *c = begin, *next; c != end; c = next) {
    next = next_klause (cadical_kitten, c);
    // CADICAL_assert (!is_learned_klause (c)); not necessarily true
    if (is_learned_klause (c))
      continue;
    if (!is_core_klause (c))
      continue;
    unset_core_klause (c);
    const unsigned dst = (unsigned *) q - (unsigned *) begin;
    const unsigned size = c->size;
    if (!size) {
      if (cadical_kitten->inconsistent != INVALID)
        cadical_kitten->inconsistent = dst;
    } else if (size == 1) {
      PUSH_STACK (cadical_kitten->units, dst);
      ROG (dst, "keeping");
    } else {
      watch_klause (cadical_kitten, c->lits[0], dst);
      watch_klause (cadical_kitten, c->lits[1], dst);
    }
    if (c == q)
      q = next;
    else {
      const size_t bytes = (char *) next - (char *) c;
      memmove (q, c, bytes);
      q = (klause *) ((char *) q + bytes);
    }
#ifdef LOGGING
    original++;
#endif
  }
  SET_END_OF_STACK (cadical_kitten->klauses, (unsigned *) q);
  cadical_kitten->end_original_ref = SIZE_STACK (cadical_kitten->klauses);
  LOG ("end of original clauses at %zu", cadical_kitten->end_original_ref);
  LOG ("%u original clauses left", original);

  CLEAR_STACK (cadical_kitten->core);

  UPDATE_STATUS (0);
}

signed char cadical_kitten_signed_value (cadical_kitten *cadical_kitten, int selit) {
  REQUIRE_STATUS (10);
  const unsigned elit = int2u (selit);
  const unsigned eidx = elit / 2;
  if (eidx >= cadical_kitten->evars)
    return 0;
  unsigned iidx = cadical_kitten->import[eidx];
  if (!iidx)
    return 0;
  const unsigned ilit = 2 * (iidx - 1) + (elit & 1);
  return cadical_kitten->values[ilit];
}

signed char cadical_kitten_value (cadical_kitten *cadical_kitten, unsigned elit) {
  REQUIRE_STATUS (10);
  const unsigned eidx = elit / 2;
  if (eidx >= cadical_kitten->evars)
    return 0;
  unsigned iidx = cadical_kitten->import[eidx];
  if (!iidx)
    return 0;
  const unsigned ilit = 2 * (iidx - 1) + (elit & 1);
  return cadical_kitten->values[ilit];
}

signed char cadical_kitten_fixed (cadical_kitten *cadical_kitten, unsigned elit) {
  const unsigned eidx = elit / 2;
  if (eidx >= cadical_kitten->evars)
    return 0;
  unsigned iidx = cadical_kitten->import[eidx];
  if (!iidx)
    return 0;
  iidx--;
  const unsigned ilit = 2 * iidx + (elit & 1);
  signed char res = cadical_kitten->values[ilit];
  if (!res)
    return 0;
  kar *v = cadical_kitten->vars + iidx;
  if (v->level)
    return 0;
  return res;
}

signed char cadical_kitten_fixed_signed (cadical_kitten *cadical_kitten, int elit) {
  unsigned kelit = int2u (elit);
  return cadical_kitten_fixed (cadical_kitten, kelit);
}

bool cadical_kitten_flip_literal (cadical_kitten *cadical_kitten, unsigned elit) {
  REQUIRE_STATUS (10);
  const unsigned eidx = elit / 2;
  if (eidx >= cadical_kitten->evars)
    return false;
  unsigned iidx = cadical_kitten->import[eidx];
  if (!iidx)
    return false;
  const unsigned ilit = 2 * (iidx - 1) + (elit & 1);
  if (cadical_kitten_fixed (cadical_kitten, elit))
    return false;
  return flip_literal (cadical_kitten, ilit);
}

bool cadical_kitten_flip_signed_literal (cadical_kitten *cadical_kitten, int elit) {
  REQUIRE_STATUS (10);
  unsigned kelit = int2u (elit);
  return cadical_kitten_flip_literal (cadical_kitten, kelit);
}

bool cadical_kitten_failed (cadical_kitten *cadical_kitten, unsigned elit) {
  REQUIRE_STATUS (20);
  const unsigned eidx = elit / 2;
  if (eidx >= cadical_kitten->evars)
    return false;
  unsigned iidx = cadical_kitten->import[eidx];
  if (!iidx)
    return false;
  const unsigned ilit = 2 * (iidx - 1) + (elit & 1);
  return cadical_kitten->failed[ilit];
}

// checks both watches for clauses with only one literal positively
// assigned. if such a clause is found, return false. Otherwise fix watch
// invariant and return true
static bool prime_propagate (cadical_kitten *cadical_kitten, const unsigned idx,
                             void *state, const bool ignoring,
                             bool (*ignore) (void *, unsigned)) {
  unsigned lit = 2 * idx;
  unsigned conflict = INVALID;
  value *values = cadical_kitten->values;
  for (int i = 0; i < 2; i++) {
    if (conflict != INVALID)
      break;
    lit = lit ^ i;
    const unsigned not_lit = lit ^ 1;
    katches *watches = cadical_kitten->watches + not_lit;
    unsigned *q = BEGIN_STACK (*watches);
    const unsigned *const end_watches = END_STACK (*watches);
    unsigned const *p = q;
    uint64_t ticks = (((char *) end_watches - (char *) q) >> 7) + 1;
    while (p != end_watches) {
      const unsigned ref = *q++ = *p++;
      klause *c = dereference_klause (cadical_kitten, ref);
      if (is_learned_klause (c) || ignore (state, c->aux) == ignoring)
        continue;
      CADICAL_assert (c->size > 1);
      unsigned *lits = c->lits;
      const unsigned other = lits[0] ^ lits[1] ^ not_lit;
      const value other_value = values[other];
      ticks++;
      if (other_value > 0)
        continue;
      value replacement_value = -1;
      unsigned replacement = INVALID;
      const unsigned *const end_lits = lits + c->size;
      unsigned *r;
      for (r = lits + 2; r != end_lits; r++) {
        replacement = *r;
        replacement_value = values[replacement];
        if (replacement_value > 0)
          break;
      }
      if (replacement_value > 0) {
        CADICAL_assert (replacement != INVALID);
        ROG (ref, "unwatching %u in", not_lit);
        lits[0] = other;
        lits[1] = replacement;
        *r = not_lit;
        watch_klause (cadical_kitten, replacement, ref);
        q--;
      } else {
        ROG (ref, "idx %u forced prime by", idx);
        conflict = ref;
        break;
      }
    }
    while (p != end_watches)
      *q++ = *p++;
    SET_END_OF_STACK (*watches, q);
    ADD (cadical_kitten_ticks, ticks);
  }
  return conflict == INVALID;
}

void cadical_kitten_add_prime_implicant (cadical_kitten *cadical_kitten, void *state, int side,
                                 void (*add_implicant) (void *, int, size_t,
                                                        const unsigned *)) {
  REQUIRE_STATUS (11);
  // might be possible in some edge cases
  unsigneds *prime = &cadical_kitten->prime[side];
  unsigneds *prime2 = &cadical_kitten->prime[!side];
  CADICAL_assert (!EMPTY_STACK (*prime) || !EMPTY_STACK (*prime2));
  CLEAR_STACK (*prime2);

  for (all_stack (unsigned, lit, *prime)) {
    const unsigned not_lit = lit ^ 1;
    const unsigned elit = export_literal (cadical_kitten, not_lit);
    PUSH_STACK (*prime2, elit);
  }

  // adds a clause which will reset cadical_kitten status and backtrack
  add_implicant (state, side, SIZE_STACK (*prime2), BEGIN_STACK (*prime2));
  CLEAR_STACK (*prime);
  CLEAR_STACK (*prime2);
}

// computes two prime implicants, only considering clauses based on ignore
// return -1 if no prime implicant has been computed, otherwise returns
// index of shorter implicant.
// TODO does not work if flip has been called beforehand
int cadical_kitten_compute_prime_implicant (cadical_kitten *cadical_kitten, void *state,
                                    bool (*ignore) (void *, unsigned)) {
  REQUIRE_STATUS (10);

  value *values = cadical_kitten->values;
  kar *vars = cadical_kitten->vars;
  unsigneds unassigned;
  INIT_STACK (unassigned);
  bool limit_hit = 0;
  CADICAL_assert (EMPTY_STACK (cadical_kitten->prime[0]) && EMPTY_STACK (cadical_kitten->prime[1]));
  for (int i = 0; i < 2; i++) {
    const bool ignoring = i;
    for (all_stack (unsigned, lit, cadical_kitten->trail)) {
      if (KITTEN_TICKS >= cadical_kitten->limits.ticks) {
        LOG ("ticks limit %" PRIu64 " hit after %" PRIu64 " ticks",
             cadical_kitten->limits.ticks, KITTEN_TICKS);
        limit_hit = 1;
        break;
      }
      CADICAL_assert (values[lit] > 0);
      const unsigned idx = lit / 2;
      const unsigned ref = vars[idx].reason;
      CADICAL_assert (vars[idx].level);
      klause *c = 0;
      if (ref != INVALID)
        c = dereference_klause (cadical_kitten, ref);
      if (ref == INVALID || is_learned_klause (c) ||
          ignore (state, c->aux) == ignoring) {
        LOG ("non-prime candidate var %d", idx);
        if (prime_propagate (cadical_kitten, idx, state, ignoring, ignore)) {
          values[lit] = 0;
          values[lit ^ 1] = 0;
          PUSH_STACK (unassigned, lit);
        } else
          CADICAL_assert (values[lit] > 0);
      }
    }
    unsigneds *prime = &cadical_kitten->prime[i];
    // push on prime implicant stack.
    for (all_kits (lit)) {
      if (values[lit] > 0)
        PUSH_STACK (*prime, lit);
    }
    // reassign all literals on
    for (all_stack (unsigned, lit, unassigned)) {
      CADICAL_assert (!values[lit]);
      values[lit] = 1;
      values[lit ^ 1] = -1;
    }
    CLEAR_STACK (unassigned);
  }
  RELEASE_STACK (unassigned);

  if (limit_hit) {
    CLEAR_STACK (cadical_kitten->prime[0]);
    CLEAR_STACK (cadical_kitten->prime[1]);
    return -1;
  }
  // the only case when one of the prime implicants is allowed to be empty
  // is if ignore returns always true or always false.
  CADICAL_assert (!EMPTY_STACK (cadical_kitten->prime[0]) ||
          !EMPTY_STACK (cadical_kitten->prime[1]));
  UPDATE_STATUS (11);

  int res = SIZE_STACK (cadical_kitten->prime[0]) > SIZE_STACK (cadical_kitten->prime[1]);
  return res;
}

static bool contains_blit (cadical_kitten *cadical_kitten, klause *c, const unsigned blit) {
  for (all_literals_in_klause (lit, c)) {
    if (lit == blit)
      return true;
  }
  return false;
}

static bool prime_propagate_blit (cadical_kitten *cadical_kitten, const unsigned idx,
                                  const unsigned blit) {
  unsigned lit = 2 * idx;
  unsigned conflict = INVALID;
  value *values = cadical_kitten->values;
  LOG ("prime propagating idx %u for blit %u", idx, blit);
  for (int i = 0; i < 2; i++) {
    if (conflict != INVALID)
      break;
    lit = lit ^ i;
    if (lit == blit)
      continue;
    const unsigned not_lit = lit ^ 1;
    katches *watches = cadical_kitten->watches + not_lit;
    unsigned *q = BEGIN_STACK (*watches);
    const unsigned *const end_watches = END_STACK (*watches);
    unsigned const *p = q;
    uint64_t ticks = (((char *) end_watches - (char *) q) >> 7) + 1;
    while (p != end_watches) {
      const unsigned ref = *q++ = *p++;
      klause *c = dereference_klause (cadical_kitten, ref);
      if (is_learned_klause (c))
        continue;
      ROG (ref, "checking with blit %u", blit);
      CADICAL_assert (c->size > 1);
      unsigned *lits = c->lits;
      const unsigned other = lits[0] ^ lits[1] ^ not_lit;
      const value other_value = values[other];
      ticks++;
      bool use = other == blit || not_lit == blit;
      if (other_value > 0)
        continue;
      value replacement_value = -1;
      unsigned replacement = INVALID;
      const unsigned *const end_lits = lits + c->size;
      unsigned *r;
      for (r = lits + 2; r != end_lits; r++) {
        replacement = *r;
        replacement_value = values[replacement];
        use = use || replacement == blit;
        if (replacement_value > 0)
          break;
      }
      if (replacement_value > 0) {
        CADICAL_assert (replacement != INVALID);
        ROG (ref, "unwatching %u in", not_lit);
        lits[0] = other;
        lits[1] = replacement;
        *r = not_lit;
        watch_klause (cadical_kitten, replacement, ref);
        q--;
      } else if (!use) {
        continue;
      } else {
        ROG (ref, "idx %u forced prime by", idx);
        conflict = ref;
        break;
      }
    }
    while (p != end_watches)
      *q++ = *p++;
    SET_END_OF_STACK (*watches, q);
    ADD (cadical_kitten_ticks, ticks);
  }
  return conflict == INVALID;
}

static int compute_prime_implicant_for (cadical_kitten *cadical_kitten, unsigned blit) {
  value *values = cadical_kitten->values;
  kar *vars = cadical_kitten->vars;
  unsigneds unassigned;
  INIT_STACK (unassigned);
  bool limit_hit = false;
  CADICAL_assert (EMPTY_STACK (cadical_kitten->prime[0]) && EMPTY_STACK (cadical_kitten->prime[1]));
  for (int i = 0; i < 2; i++) {
    const unsigned block = blit ^ i;
    const bool ignoring = i;
    if (prime_propagate_blit (cadical_kitten, block / 2, block)) {
      value tmp = values[blit];
      CADICAL_assert (tmp);
      values[blit] = 0;
      values[blit ^ 1] = 0;
      PUSH_STACK (unassigned, tmp > 0 ? blit : blit ^ 1);
      PUSH_STACK (cadical_kitten->prime[i], block); // will be negated!
    } else
      CADICAL_assert (false);
    for (all_stack (unsigned, lit, cadical_kitten->trail)) {
      if (KITTEN_TICKS >= cadical_kitten->limits.ticks) {
        LOG ("ticks limit %" PRIu64 " hit after %" PRIu64 " ticks",
             cadical_kitten->limits.ticks, KITTEN_TICKS);
        limit_hit = true;
        break;
      }
      if (!values[lit])
        continue;
      CADICAL_assert (values[lit]); // not true when flipping is involved
      const unsigned idx = lit / 2;
      const unsigned ref = vars[idx].reason;
      CADICAL_assert (vars[idx].level);
      LOG ("non-prime candidate var %d", idx);
      if (prime_propagate_blit (cadical_kitten, idx, block)) {
        value tmp = values[lit];
        CADICAL_assert (tmp);
        values[lit] = 0;
        values[lit ^ 1] = 0;
        PUSH_STACK (unassigned, tmp > 0 ? lit : lit ^ 1);
      }
    }
    unsigneds *prime = &cadical_kitten->prime[i];
    // push on prime implicant stack.
    for (all_kits (lit)) {
      if (values[lit] > 0)
        PUSH_STACK (*prime, lit);
    }
    // reassign all literals on
    for (all_stack (unsigned, lit, unassigned)) {
      CADICAL_assert (!values[lit]);
      values[lit] = 1;
      values[lit ^ 1] = -1;
    }
    CLEAR_STACK (unassigned);
  }
  RELEASE_STACK (unassigned);

  if (limit_hit) {
    CLEAR_STACK (cadical_kitten->prime[0]);
    CLEAR_STACK (cadical_kitten->prime[1]);
    return -1;
  }
  // the only case when one of the prime implicants is allowed to be empty
  // is if ignore returns always true or always false.
  CADICAL_assert (!EMPTY_STACK (cadical_kitten->prime[0]) ||
          !EMPTY_STACK (cadical_kitten->prime[1]));
  LOGLITS (BEGIN_STACK (cadical_kitten->prime[0]), SIZE_STACK (cadical_kitten->prime[0]),
           "first implicant %u", blit);
  LOGLITS (BEGIN_STACK (cadical_kitten->prime[1]), SIZE_STACK (cadical_kitten->prime[1]),
           "second implicant %u", blit ^ 1);
  UPDATE_STATUS (11);

  int res = SIZE_STACK (cadical_kitten->prime[0]) > SIZE_STACK (cadical_kitten->prime[1]);
  return res;
}

int cadical_kitten_flip_and_implicant_for_signed_literal (cadical_kitten *cadical_kitten,
                                                  int elit) {
  REQUIRE_STATUS (10);
  unsigned kelit = int2u (elit);
  if (!cadical_kitten_flip_literal (cadical_kitten, kelit)) {
    return -2;
  }
  const unsigned eidx = kelit / 2;
  unsigned iidx = cadical_kitten->import[eidx];
  CADICAL_assert (iidx);
  const unsigned ilit = 2 * (iidx - 1) + (kelit & 1);
  return compute_prime_implicant_for (cadical_kitten, ilit);
}

ABC_NAMESPACE_IMPL_END
