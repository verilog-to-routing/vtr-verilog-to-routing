#ifndef _congruenc_hpp_INCLUDED
#define _congruenc_hpp_INCLUDED

#include "global.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <string>
#include <sys/types.h>
#include <unordered_set>
#include <vector>

#include "clause.hpp"
#include "inttypes.hpp"
#include "util.hpp"
#include "watch.hpp"

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

typedef int64_t LRAT_ID;

// This implements the algorithm algorithm from SAT 2024.
//
// The idea is to:
//   0. handle binary clauses
//   1. detect gates and merge gates with same inputs ('lazy')
//   2. eagerly replace the equivalent literals and merge gates with same
//   inputs
//   3. forward subsume
//
// In step 0 the normalization is fully lazy but we do not care about a
// normal form. Therefore we actually eagerly merge literals.
//
// In step 2 there is a subtility: we only replace with the equivalence
// chain as far as we propagated so far. This is the eager part. For LRAT we
// produce the equivalence up to the point we have propagated, no the full
// chain. This is important for merging literals.  To merge literals we use
// union-find but we only compress paths when rewriting the literal, not
// before. The compression was not considered important in Kissat, but we do
// it aggressively as a mirror of the equivalences we have generated.
//
// We have two structures for merging:
//   - the lazy ones contains alls merges, with functions like
//   find_representative
//
//   - the eager version that gets the merges one by one, with functions
//   like find_eager_representatives
//
//  The two structures are nicely separated and we only working on one of
//  them except for:
//
//    1. When propagating one equivalence, we first important the
//    equivalence from the lazy to the eager version, producing the full
//    chain.
//
//    2. When merging the literals, we merge the literals given by the lazy
//    structure, then we merge their representative in the eager version,
//    updating only the lazy structure. We do not update the eager version.
//
// An important point: We cannot use internal->lrat_chain and
// internal->clause because in most places we can interrupt the
// transformation to learn a new clause representing an equivalence.
// However, we can only have 2 layers so we use this->lrat_chain and
// internal->lrat_chain when we really produce the proof.
struct Internal;

#define LD_MAX_ARITY 26
#define MAX_ARITY ((1 << LD_MAX_ARITY) - 1)

enum class Gate_Type { And_Gate, XOr_Gate, ITE_Gate };

// Wrapper when we are looking for implication in if-then-else gates
struct lit_implication {
  int first;
  int second;
  Clause *clause;
  lit_implication (int f, int s, Clause *_id)
      : first (f), second (s), clause (_id) {}
  lit_implication (int f, int s) : first (f), second (s), clause (0) {}
  lit_implication () : first (0), second (0), clause (nullptr) {}
  void swap () { std::swap (first, second); }
};

// Wrapper when we are looking for equivalence for if-then-else-gate. They
// are produced by merging implication
struct lit_equivalence {
  int first;
  int second;
  Clause *first_clause;
  Clause *second_clause;
  void check_invariant () {
    CADICAL_assert (second_clause);
    CADICAL_assert (first_clause);
    CADICAL_assert (std::find (begin (*first_clause), end (*first_clause), first) !=
            end (*first_clause));
    CADICAL_assert (std::find (begin (*second_clause), end (*second_clause),
                       second) != end (*second_clause));
    CADICAL_assert (std::find (begin (*first_clause), end (*first_clause),
                       -second) != end (*first_clause));
    CADICAL_assert (std::find (begin (*second_clause), end (*second_clause),
                       -first) != end (*second_clause));
  }
  lit_equivalence (int f, Clause *f_id, int s, Clause *s_id)
      : first (f), second (s), first_clause (f_id), second_clause (s_id) {}
  lit_equivalence (int f, int s)
      : first (f), second (s), first_clause (nullptr),
        second_clause (nullptr) {}
  lit_equivalence ()
      : first (0), second (0), first_clause (nullptr),
        second_clause (nullptr) {}
  lit_equivalence swap () {
    std::swap (first, second);
    std::swap (first_clause, second_clause);
    return *this;
  }
  lit_equivalence negate_both () {
    first = -first;
    second = -second;
    std::swap (first_clause, second_clause);
    return *this;
  }
};

typedef std::vector<lit_implication> lit_implications;
typedef std::vector<lit_equivalence> lit_equivalences;

std::string string_of_gate (Gate_Type t);

struct LitClausePair {
  int current_lit; // current literal from the gate
  Clause *clause;
  LitClausePair (int lit, Clause *cl) : current_lit (lit), clause (cl) {}
  LitClausePair () : current_lit (0), clause (nullptr) {}
};
struct LitIdPair {
  int lit; // current literal from the gate
  LRAT_ID id;
  LitIdPair (int l, LRAT_ID i) : lit (l), id (i) {}
  LitIdPair () : lit (0), id (0) {}
};

/*------------------------------------------------------------------------*/

// Sorting the scheduled clauses is way faster if we compute and save the
// clause size in the schedule to avoid pointer access to clauses during
// sorting.  This slightly increases the schedule size though.

struct ClauseSize {
  size_t size;
  Clause *clause;
  ClauseSize (int s, Clause *c) : size (s), clause (c) {}
  ClauseSize (Clause *c): size (c->size), clause (c) {}
  ClauseSize () {}
};

struct smaller_clause_size_rank {
  typedef size_t Type;
  Type operator() (const ClauseSize &a) { return a.size; }
};

/*------------------------------------------------------------------------*/
// There are many special cases for ITE gates and we have to keep track of
// them as it is a gate property (rewriting might not make it obvious
// anymore).
// a = (a ? t : e) results in no -t and no +e gate (a --> a = t  == (-a v -a v t) & (-a v a v -t))
// a = (-a ? t : e) results in no +t and no -e gate
// a = (c ? a : e) results in no t gate (none of them)
// a = (c ? t : a) results in no e gate (none of them)

enum Special_ITE_GATE {
  NORMAL = 0,
  NO_PLUS_THEN = (1 << 0),
  NO_NEG_THEN = (1 << 1),
  NO_THEN = NO_PLUS_THEN + NO_NEG_THEN,
  NO_PLUS_ELSE = (1 << 2),
  NO_NEG_ELSE = (1 << 3),
  NO_ELSE = NO_PLUS_ELSE + NO_NEG_ELSE,
  COND_LHS = NO_NEG_THEN + NO_PLUS_ELSE,
  UCOND_LHS = NO_PLUS_THEN + NO_NEG_ELSE,
};

inline bool ite_flags_no_then_clauses (int8_t flag) {
  return (flag & NO_THEN) == NO_THEN;
}

inline bool ite_flags_no_else_clauses (int8_t flag) {
  return (flag & NO_ELSE) == NO_ELSE;
}

inline bool ite_flags_neg_cond_lhs (int8_t flag) {
  return (flag & UCOND_LHS) == UCOND_LHS;
}

inline bool ite_flags_cond_lhs (int8_t flag) {
  return (flag & COND_LHS) == COND_LHS;
}

/*------------------------------------------------------------------------*/

// The core structure of this algorithm: the gate. It is composed of a
// left-hand side and an array of right-hand side.
//
// There are a few tags to help remembering the status of the gate (like
// deleted)
//
// To keep track of the proof we use two extra arrays:
//  - `neg_lhs_ids' contains the long clause for AND gates. Otherwise, it is
//  empty. TODO: change to std::option as it contains at most one element
//  - `pos_lhs_ids' contains all the remaining gates.
//
// We keep the reasons with an index. This index depends on the gates:

//   - AND-Gates and ITE-Gates: the index is the literal from the RHS
//
//   - XOR-Gates: if you order the clauses by the order of the literals,
//   each literal is either positive (bit '1') or negative (bit '0'). This
//   gives a number that we can use.
//
// TODO Florian: I do not think that you have to changed anything, look at
// the 'Look at this first' in the CPP file.
//
// Important for the proofs: the LHS is not updated.
//
// TODO: we currently use a vector for the rhs, but we could also use FMA
// and inline the structure to avoid any indirection.
//
// One warning for degenerated gate: it is a monotone property on the
// defining clauses, but not on the LHS/RHS as the LHS is not rewritten:
// take 4 = AND 3 4 (degenerated with only the clause -4 3) with a rewriting
// 4 -> 1 (unchanged clause) and later 1 -> 3 (unchanged clause) but you do
// not know anymore from the gate that it is degenerated
struct Gate {
#ifdef LOGGING
  uint64_t id;
#endif
  int lhs;
  Gate_Type tag;
  bool garbage : 1;
  bool indexed : 1;
  bool marked : 1;
  bool shrunken : 1;
  size_t hash; // TODO remove this field (the C++ implementation is caching
               // it anyway)
  vector<LitClausePair> pos_lhs_ids;
  vector<LitClausePair> neg_lhs_ids;
  bool degenerated_and_neg = false; // LRAT only relevant for AND Gates, neg lhs in RHS
  bool degenerated_and_pos = false; // LRAT only relevant for AND Gates, pos lhs in RHS
  int8_t degenerated_ite = Special_ITE_GATE::NORMAL;
  vector<int> rhs;

  size_t arity () const { return rhs.size (); }

  bool operator== (Gate const &lhs) {
    return tag == lhs.tag && hash == lhs.hash && rhs == lhs.rhs;
  }
};

typedef vector<Gate *> GOccs;

struct GateEqualTo {
  bool operator() (const Gate *const lhs, const Gate *const rhs) const {
    return lhs->rhs == rhs->rhs && lhs->tag == rhs->tag;
  }
};

struct CompactBinary {
  Clause *clause;
  LRAT_ID id;
  int lit1, lit2;
  CompactBinary (Clause *c, LRAT_ID i, int l1, int l2)
      : clause (c), id (i), lit1 (l1), lit2 (l2) {}
  CompactBinary () : clause (nullptr), id (0), lit1 (0), lit2 (0) {}
};

struct Hash {
  Hash (std::array<int, 16> &ncs) : nonces (ncs) {}
  std::array<int, 16> &nonces;
  size_t operator() (const Gate *const g) const;
};

struct Rewrite {
  int src, dst;
  LRAT_ID id1;
  LRAT_ID id2;

  Rewrite (int _src, int _dst, LRAT_ID _id1, LRAT_ID _id2)
      : src (_src), dst (_dst), id1 (_id1), id2 (_id2) {}
  Rewrite () : src (0), dst (0), id1 (0), id2 (0) {}
};

struct Closure {

  Closure (Internal *i);

  Internal *const internal;
  vector<Clause*> extra_clauses;
  vector<CompactBinary> binaries;
  std::vector<std::pair<size_t, size_t>> offsetsize;
  bool full_watching = false;
  std::array<int, 16> nonces;
  typedef unordered_set<Gate *, Hash, GateEqualTo> GatesTable;

  vector<bool> scheduled;
  vector<signed char> marks;
  vector<LitClausePair> mu1_ids, mu2_ids,
      mu4_ids; // remember the ids and the literal. 2 and 4 are
               // only used for lrat proofs, but we need 1 to
               // promote binary clauses to irredundant

  vector<int> lits;         // result of definitions
  vector<int> rhs;          // stack for storing RHS
  vector<int> unsimplified; // stack for storing unsimplified version (XOR,
                            // ITEs) for DRAT proof
  vector<int> chain;  // store clauses to be able to delete them properly
  vector<int> clause; // storing partial clauses
  vector<uint64_t>
      glargecounts; // count for large clauses to complement internal->noccs
  vector<uint64_t> gnew_largecounts; // count for large clauses to
                                     // complement internal->noccs
  GatesTable table;
  std::array<lit_implications, 2> condbin;
  std::array<lit_equivalences, 2> condeq;

  std::vector<Clause *> new_unwatched_binary_clauses;
  // LRAT proofs
  vector<int> resolvent_analyzed;
  mutable vector<LRAT_ID> lrat_chain; // storing LRAT chain

#ifdef LOGGING
  uint64_t fresh_id;
#endif

  uint64_t &new_largecounts (int lit);
  uint64_t &largecounts (int lit);

  void unmark_all ();
  vector<int> representant;              // union-find
  vector<int> eager_representant;        // union-find
  vector<LRAT_ID> representant_id;       // lrat version of union-find
  vector<LRAT_ID> eager_representant_id; // lrat version of union-find
  int &representative (int lit);
  int representative (int lit) const;
  LRAT_ID &representative_id (int lit);
  LRAT_ID representative_id (int lit) const;
  int &eager_representative (int lit);
  int eager_representative (int lit) const;
  LRAT_ID &eager_representative_id (int lit);
  LRAT_ID eager_representative_id (int lit) const;
  std::vector<char> lazy_propagated_idx;
  char &lazy_propagated (int lit);

  int find_lrat_representative_with_marks (int lit);
  // representative in the union-find structure in the lazy equivalences
  int find_representative (int lit);
  // find the representative and produce the binary clause representing the
  // normalization from the literal to the result.
  int find_representative_and_compress (int, bool update_eager = true);
  // find the lazy representative for the `lit' and `-lit'
  void find_representative_and_compress_both (int);
  // find the eager representative
  int find_eager_representative (int);

  // compreses the path from lit to the representative with a new clause if
  // needed. Save internal->lrat_chain to avoid any issue.
  int find_eager_representative_and_compress (int);
  // Import the path from the literal and its negation to the representative
  // in the lazy graph to the eager part, producing the binary clauses.
  void import_lazy_and_find_eager_representative_and_compress_both (
      int); // generates clauses for -lit and lit

  // returns the ID of the LRAT clause for the normalization from the
  // literal lit to its argument, assuming that the representative was
  // already compressed.
  LRAT_ID find_representative_lrat (int lit);
  // returns the ID of the LRAT clause for the eager normalization from the
  // literal lit to its argument assuming that the representative was
  // already compressed.
  LRAT_ID find_eager_representative_lrat (int lit);

  // Writes the LRAT chain required for the eager normalization to
  // `lrat_chain`.
  void produce_eager_representative_lrat (int lit);
  // Writes the LRAT chain required for the lazy normalization to
  // `lrat_chain`.
  void produce_representative_lrat (int lit);

  // learns a binary clause if not unit
  Clause *maybe_add_binary_clause (int a, int b);
  // add binary clause
  Clause *add_binary_clause (int a, int b);
  // add tmp clause
  Clause *add_tmp_binary_clause (int a, int b);
  // add clause taking core of tmp or full
  Clause *learn_binary_tmp_or_full_clause (int a, int b);

  // promotes a clause from redundant to irredundant. We do this for all
  // clauses involved in gates to make sure that we produce correct result.
  void promote_clause (Clause *);

  // Merge functions. We actually need different several versions for LRAT
  // in order to simplify the proof production.
  //
  // When merging binary clauses, we can simply produce the LRAT chain by
  // (1) using the two binary clauses and (2) the reason clause from the
  // literals to the representatives.
  //
  // The same approach does not work for merging gates because the
  // representative might be also a representative of another literal
  // (because of eager rewriting), requiring to resolve more than once on
  // the same literal. An example of this are the two gates 4=-2&7 and
  // 6=-2&1, the rewriting 7=1 and the equivalence 4=1. The simple road of
  // merging 6 and 4 (requires resolving away 1) + adding the rewrite 4 to 1
  // (requires adding 1) does not work.
  //
  // Therefore, we actually go for the more regular road and produce two
  // equivalence: the merge from the LHS, followed by the actual equivalence
  // (by combining it with the rewrite).  In DRAT this is less important
  // because the checker finds a chain and is less restricted than our LRAT
  // chain.
  bool merge_literals_equivalence (int lit, int other, Clause *c1,
                                   Clause *c2);
  bool merge_literals_lrat (Gate *g, Gate *h, int lit, int other,
                            const std::vector<LRAT_ID> & = {},
                            const std::vector<LRAT_ID> & = {});
  bool merge_literals_lrat (int lit, int other,
                            const std::vector<LRAT_ID> & = {},
                            const std::vector<LRAT_ID> & = {});

  // proof production
  vector<LitClausePair> lrat_chain_and_gate;
  void push_lrat_id (const Clause *const c, int lit);
  void push_lrat_unit (int lit);

  // pushes the clause with the reasons to rewrite clause
  // unless:
  //   - the rewriting is not necessary (resolvent_marked == 1)
  //   - it is overwritten by one of the arguments
  void push_id_and_rewriting_lrat_unit (Clause *c, Rewrite rewrite1,
                                        std::vector<LRAT_ID> &chain,
                                        bool = true,
                                        Rewrite rewrite2 = Rewrite (),
                                        int execept_lhs = 0,
                                        int except_lhs2 = 0);
  void push_id_and_rewriting_lrat_full (Clause *c, Rewrite rewrite1,
                                        std::vector<LRAT_ID> &chain,
                                        bool = true,
                                        Rewrite rewrite2 = Rewrite (),
                                        int execept_lhs = 0,
                                        int except_lhs2 = 0);
  // TODO: does nothing except pushing on the stack, remove!
  void push_id_on_chain (std::vector<LRAT_ID> &chain, Clause *c);
  // TODO: does nothing except pushing on the stack, remove!
  void push_id_on_chain (std::vector<LRAT_ID> &chain,
                         const std::vector<LitClausePair> &c);
  // TODO: does nothing except pushing on the stack, remove!
  void push_id_on_chain (std::vector<LRAT_ID> &chain, Rewrite rewrite, int);
  void update_and_gate_build_lrat_chain (
      Gate *g, Gate *h, std::vector<LRAT_ID> &extra_reasons_lit,
					 std::vector<LRAT_ID> &extra_reasons_ulit, bool remove_units = true);
  void update_and_gate_unit_build_lrat_chain (
      Gate *g, int src, LRAT_ID id1, LRAT_ID id2, int dst,
      std::vector<LRAT_ID> &extra_reasons_lit,
      std::vector<LRAT_ID> &extra_reasons_ulit);
  // occs
  vector<GOccs> gtab;
  GOccs &goccs (int lit);
  void connect_goccs (Gate *g, int lit);
  vector<Gate *> garbage;
  void mark_garbage (Gate *);
  // remove the gate from the table
  bool remove_gate (Gate *);
  bool remove_gate (GatesTable::iterator git);
  void index_gate (Gate *);

  // second counter for size, complements noccs
  uint64_t &largecount (int lit);

  // simplification
  bool skip_and_gate (Gate *g);
  bool skip_xor_gate (Gate *g);
  void update_and_gate (Gate *g, GatesTable::iterator, int src, int dst,
                        LRAT_ID id1, LRAT_ID id2, int falsified = 0,
                        int clashing = 0);
  void update_xor_gate (Gate *g, GatesTable::iterator);
  void shrink_and_gate (Gate *g, int falsified = 0, int clashing = 0);
  bool simplify_gate (Gate *g);
  void simplify_and_gate (Gate *g);
  void simplify_ite_gate (Gate *g);
  Clause *simplify_xor_clause (int lhs, Clause *);
  void simplify_xor_gate (Gate *g);
  bool simplify_gates (int lit);
  void simplify_and_sort_xor_lrat_clauses (const vector<LitClausePair> &,
                                           vector<LitClausePair> &, int,
                                           int except2 = 0, bool flip = 0);
  void simplify_unit_xor_lrat_clauses (const vector<LitClausePair> &, int);

  // rewriting
  bool rewriting_lhs (Gate *g, int dst);
  bool rewrite_gates (int dst, int src, LRAT_ID id1, LRAT_ID id2);
  bool rewrite_gate (Gate *g, int dst, int src, LRAT_ID id1, LRAT_ID id2);
  void rewrite_xor_gate (Gate *g, int dst, int src);
  void rewrite_and_gate (Gate *g, int dst, int src, LRAT_ID id1,
                         LRAT_ID id2);
  void rewrite_ite_gate (Gate *g, int dst, int src);

  size_t units; // next trail position to propagate
  bool propagate_unit (int lit);
  bool propagate_units ();
  size_t propagate_units_and_equivalences ();
  bool propagate_equivalence (int lit);

  // gates
  void init_closure ();
  void reset_closure ();
  void reset_extraction ();
  void reset_and_gate_extraction ();
  void extract_and_gates (Closure &);
  void extract_gates ();
  void extract_and_gates_with_base_clause (Clause *c);
  void init_and_gate_extraction ();
  Gate *find_first_and_gate (Clause *base_clause, int lhs);
  Gate *find_remaining_and_gate (Clause *base_clause, int lhs);
  void extract_and_gates ();

  Gate *find_and_lits (const vector<int> &rhs, Gate *except = nullptr);
  // rhs is sorted, so passing by copy
  Gate *find_gate_lits (const vector<int> &rhs, Gate_Type typ,
                        Gate *except = nullptr);
  Gate *find_xor_lits (const vector<int> &rhs);
  // not const to normalize negations, also fixes the order of the LRAT
  Gate *find_ite_gate (Gate *, bool &);
  Gate *find_xor_gate (Gate *);

  void reset_xor_gate_extraction ();
  void init_xor_gate_extraction (std::vector<Clause*> &candidates);
  LRAT_ID check_and_add_to_proof_chain (vector<int> &clause);
  void add_xor_matching_proof_chain (Gate *g, int lhs1,
                                     const vector<LitClausePair> &,
                                     int lhs2, vector<LRAT_ID> &,
                                     vector<LRAT_ID> &);
  void add_xor_shrinking_proof_chain (Gate *g, int src);
  void extract_xor_gates ();
  void extract_xor_gates_with_base_clause (Clause *c);
  Clause *find_large_xor_side_clause (std::vector<int> &lits);

  void merge_condeq (int cond, lit_equivalences &condeq,
                     lit_equivalences &not_condeq);
  void find_conditional_equivalences (int lit, lit_implications &condbin,
                                      lit_equivalences &condeq);
  void copy_conditional_equivalences (int lit, lit_implications &condbin);
  void check_ite_implied (int lhs, int cond, int then_lit, int else_lit);
  void check_ite_gate_implied (Gate *g);
  void check_and_gate_implied (Gate *g);
  void check_ite_lrat_reasons (Gate *g, bool = false);
  void check_contained_module_rewriting (Clause *c, int lit, bool,
                                         int except);
  void delete_proof_chain ();

  // ite gate extraction
  void extract_ite_gates_of_literal (int);
  void extract_ite_gates_of_variable (int idx);
  void extract_condeq_pairs (int lit, lit_implications &condbin,
                             lit_equivalences &condeq);
  void init_ite_gate_extraction (std::vector<ClauseSize> &candidates);
  lit_implications::const_iterator find_lit_implication_second_literal (
      int lit, lit_implications::const_iterator begin,
      lit_implications::const_iterator end);
  void search_condeq (int lit, int pos_lit,
                      lit_implications::const_iterator pos_begin,
                      lit_implications::const_iterator pos_end, int neg_lit,
                      lit_implications::const_iterator neg_begin,
                      lit_implications::const_iterator neg_end,
                      lit_equivalences &condeq);
  void reset_ite_gate_extraction ();
  void extract_ite_gates ();

  void forward_subsume_matching_clauses ();

  void extract_congruence ();

  void add_ite_matching_proof_chain (Gate *g, Gate *h, int lhs1, int lhs2,
                                     std::vector<LRAT_ID> &reasons1,
                                     std::vector<LRAT_ID> &reasons2);
  void add_ite_turned_and_binary_clauses (Gate *g);
  Gate *new_and_gate (Clause *, int);
  Gate *new_ite_gate (int lhs, int cond, int then_lit, int else_lit,
                      std::vector<LitClausePair> &&clauses);
  Gate *new_xor_gate (const vector<LitClausePair> &, int);
  // check
  void check_xor_gate_implied (Gate const *const);
  void check_ternary (int a, int b, int c);
  void check_binary_implied (int a, int b);
  void check_implied ();

  // learn units. You can delay units if you want to learn several at once before
  // propagation. Otherwise, propagate! If you need propagation even if nothing is set, use the
  // second parameter.
  //
  // The function can also learn the empty clause if the unit is already set. Do not add the unit in
  // the chain!
  bool learn_congruence_unit (int unit, bool = false, bool = false);
  bool fully_propagate ();  
  void learn_congruence_unit_falsifies_lrat_chain (Gate *g, int src,
                                                   int dst, 
                                                   int clashing,
                                                   int falsified, int unit);
  void learn_congruence_unit_when_lhs_set (Gate *g, int src, LRAT_ID id1,
                                           LRAT_ID id2, int dst);

  void find_units ();
  void find_equivalences ();
  void subsume_clause (Clause *subsuming, Clause *subsumed);
  bool find_subsuming_clause (Clause *c);
  void produce_rewritten_clause_lrat_and_clean (vector<LitClausePair> &,
                                                int execept_lhs = 0,
                                                bool = true);
  // rewrite the clause using eager rewriting and rew1 and rew2, except for
  // 2 literals Usage:
  //   - the except are used to ignore LHS of gates that have not and should
  //   not be rewritten.
  //   - TODO: except_lhs2 should never be used actually
  //   - the Rewrite are for additional rewrite to allow for lazy rewrites
  //   to be taken into account without being added to the eager rewriting
  //   (yet)
  Clause *produce_rewritten_clause_lrat (Clause *c, int execept_lhs = 0,
                                         bool remove_units = true, bool = true);
  void produce_rewritten_clause_lrat (vector<LitClausePair> &,
                                                int execept_lhs = 0,
                                                bool = true);
  void compute_rewritten_clause_lrat_simple (Clause *c, int except);
  // variant where we update the indices after removing the tautologies and
  // remove the tautological clauses
  void produce_rewritten_clause_lrat_and_clean (
      std::vector<LitClausePair> &litIds, int except_lhs,
      size_t &old_position1, size_t &old_position2,
      bool remove_units = true);
  // binary extraction and ternary strengthening
  void extract_binaries ();
  bool find_binary (int, int) const;

  Clause *new_tmp_clause (std::vector<int> &clause);
  Clause *maybe_promote_tmp_binary_clause (Clause *);
  void check_not_tmp_binary_clause (Clause *c);
  Clause *new_clause ();
  //
  void sort_literals_by_var (vector<int> &rhs);
  void sort_literals_by_var_except (vector<int> &rhs, int, int except2 = 0);

  // schedule
  queue<int> schedule;
  void schedule_literal (int lit);
  void add_clause_to_chain (std::vector<int>, LRAT_ID);
  // proof. If delete_id is non-zero, then delete the clause instead of
  // learning it
  LRAT_ID simplify_and_add_to_proof_chain (vector<int> &unsimplified,
                                           LRAT_ID delete_id = 0);

  // we define our own wrapper as cadical has otherwise a non-compatible
  // marking system
  signed char &marked (int lit);
  void set_mu1_reason (int lit, Clause *c);
  void set_mu2_reason (int lit, Clause *c);
  void set_mu4_reason (int lit, Clause *c);
  LitClausePair marked_mu1 (int lit);
  LitClausePair marked_mu2 (int lit);
  LitClausePair marked_mu4 (int lit);

  // XOR
  uint32_t number_from_xor_reason_reversed (const std::vector<int> &rhs);
  uint32_t number_from_xor_reason (const std::vector<int> &rhs, int,
                                   int except2 = 0, bool flip = 0);
  void gate_sort_lrat_reasons (std::vector<LitClausePair> &, int,
                               int except2 = 0, bool flip = 0);
  void gate_sort_lrat_reasons (LitClausePair &, int, int except2 = 0,
                               bool flip = 0);

  bool rewrite_ite_gate_to_and (Gate *g, int dst, int src, size_t c,
                                size_t d, int cond_lit_to_learn_if_degenerated);
  void produce_ite_merge_then_else_reasons (
      Gate *g, int dst, int src, std::vector<LRAT_ID> &reasons_implication,
      std::vector<LRAT_ID> &reasons_back);
  void produce_ite_merge_lhs_then_else_reasons (
      Gate *g, std::vector<LRAT_ID> &reasons_implication,
      std::vector<LRAT_ID> &reasons_back,
      std::vector<LRAT_ID> &reasons_unit, bool, bool &);
  void rewrite_ite_gate_update_lrat_reasons (Gate *g, int src, int dst);
  void simplify_ite_gate_produce_unit_lrat (Gate *g, int lit, size_t idx1,
                                            size_t idx2);
  void merge_and_gate_lrat_produce_lrat (
      Gate *g, Gate *h, std::vector<LRAT_ID> &reasons_lrat,
      std::vector<LRAT_ID> &reasons_lrat_back, bool remove_units = true);
  // first index is a binary clause after unit propagation and the second
  // has length 3
  bool simplify_ite_gate_to_and (Gate *g, size_t idx1, size_t idx2,
                                 int removed);
  void
  merge_ite_gate_same_then_else_lrat (std::vector<LitClausePair> &clauses,
                               std::vector<LRAT_ID> &reasons_implication,
                               std::vector<LRAT_ID> &reasons_back);
  void simplify_ite_gate_then_else_set (
      Gate *g, std::vector<LRAT_ID> &reasons_implication,
      std::vector<LRAT_ID> &reasons_back, size_t idx1, size_t idx2);

  void simplify_ite_gate_condition_set (
      Gate *g, std::vector<LRAT_ID> &reasons_lrat,
      std::vector<LRAT_ID> &reasons_back_lrat, size_t idx1, size_t idx2);
  bool normalize_ite_lits_gate (Gate *rhs);
};

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
