#ifndef _stats_hpp_INCLUDED
#define _stats_hpp_INCLUDED

#include "global.h"

#include <cstdint>
#include <cstdlib>
#include <vector>

ABC_NAMESPACE_CXX_HEADER_START

namespace CaDiCaL {

struct Internal;

struct Stats {

  Internal *internal;

  int64_t vars = 0; // internal initialized variables

  int64_t conflicts = 0; // generated conflicts in 'propagate'
  int64_t decisions = 0; // number of decisions in 'decide'

  struct {
    int64_t cover = 0;       // propagated during covered clause elimination
    int64_t instantiate = 0; // propagated during variable instantiation
    int64_t probe = 0;       // propagated during probing
    int64_t search = 0;      // propagated literals during search
    int64_t transred = 0;    // propagated during transitive reduction
    int64_t vivify = 0;      // propagated during vivification
    int64_t walk = 0;        // propagated during local search
  } propagations;

  struct {
    int64_t search[2] = {0};
    int64_t factor = 0;
    int64_t probe = 0;
    int64_t sweep = 0;
    int64_t ternary = 0;
    int64_t vivify = 0;
  } ticks;

  struct {
    int64_t ext_cb = 0; // number of times any external callback was called
    int64_t eprop_call = 0; // number of times external_propagate was called
    int64_t eprop_prop = 0; // number of times external propagate propagated
    int64_t eprop_conf =
        0; // number of times ex-propagate was already falsified
    int64_t eprop_expl =
        0; // number of times external propagate was explained
    int64_t elearn_call =
        0; // number of times external clause learning was tried
    int64_t elearned =
        0; // learned external clauses (incl. eprop explanations)
    int64_t elearn_prop =
        0; // number of learned and propagating external clauses
    int64_t elearn_conf =
        0; // number of learned and conflicting external clauses
    int64_t echeck_call = 0; // number of checking found complete solutions
  } ext_prop;

  int64_t condassinit = 0; // initial assigned literals
  int64_t condassirem = 0; // initial assigned literals for blocked
  int64_t condassrem = 0;  // remaining assigned literals for blocked
  int64_t condassvars = 0; // sum of active variables at initial assignment
  int64_t condautinit = 0; // initial literals in autarky part
  int64_t condautrem = 0;  // remaining literals in autarky part for blocked
  int64_t condcands = 0;   // globally blocked candidate clauses
  int64_t condcondinit = 0; // initial literals in conditional part
  int64_t condcondrem =
      0; // remaining literals in conditional part for blocked
  int64_t conditioned = 0;   // globally blocked clauses eliminated
  int64_t conditionings = 0; // globally blocked clause eliminations
  int64_t condprops = 0;     // propagated unassigned literals

  struct {
    int64_t block = 0;   // block marked literals
    int64_t elim = 0;    // elim marked variables
    int64_t subsume = 0; // subsume marked variables
    int64_t ternary = 0; // ternary marked variables
    int64_t factor = 0;
  } mark;

  struct {
    int64_t total = 0;
    int64_t redundant = 0;
    int64_t irredundant = 0;
  } current, added; // Clauses.

  struct {
    double process = 0, real = 0;
  } time;

  struct {
    int64_t count = 0;      // number of covered clause elimination rounds
    int64_t asymmetric = 0; // number of asymmetric tautologies in CCE
    int64_t blocked = 0;    // number of blocked covered tautologies
    int64_t total = 0;      // total number of eliminated clauses
  } cover;

  struct {
    int64_t tried = 0;
    int64_t succeeded = 0;
    struct {
      int64_t one = 0, zero = 0;
    } constant, forward, backward;
    struct {
      int64_t positive = 0, negative = 0;
    } horn;
  } lucky;

  struct {
    int64_t total = 0;    // total number of happened rephases
    int64_t best = 0;     // how often reset to best phases
    int64_t flipped = 0;  // how often reset phases by flipping
    int64_t inverted = 0; // how often reset to inverted phases
    int64_t original = 0; // how often reset to original phases
    int64_t random = 0;   // how often randomly reset phases
    int64_t walk = 0;     // phases improved through random walked
  } rephased;

  struct {
    int64_t count = 0;
    int64_t broken = 0;
    int64_t flips = 0;
    int64_t minimum = 0;
  } walk;

  struct {
    int64_t count = 0;   // flushings of learned clauses counter
    int64_t learned = 0; // flushed learned clauses
    int64_t hyper = 0;   // flushed hyper binary/ternary clauses
  } flush;

  int64_t compacts = 0;      // number of compactifications
  int64_t shuffled = 0;      // shuffled queues and scores
  int64_t restarts = 0;      // actual number of happened restarts
  int64_t restartlevels = 0; // levels at restart
  int64_t restartstable = 0; // actual number of happened restarts
  int64_t stabphases = 0;    // number of stabilization phases
  int64_t stabconflicts =
      0;                    // number of search conflicts during stabilizing
  int64_t rescored = 0;     // number of times scores were rescored
  int64_t reused = 0;       // number of reused trails
  int64_t reusedlevels = 0; // reused levels at restart
  int64_t reusedstable = 0; // number of reused trails during stabilizing
  int64_t sections = 0;     // 'section' counter
  int64_t chrono = 0;       // chronological backtracks
  int64_t backtracks = 0;   // number of backtracks
  int64_t improvedglue = 0; // improved glue during bumping
  int64_t promoted1 = 0;    // promoted clauses to tier one
  int64_t promoted2 = 0;    // promoted clauses to tier two
  int64_t bumped = 0;       // seen and bumped variables in 'analyze'
  int64_t recomputed = 0;   // recomputed glues 'recompute_glue'
  int64_t searched = 0;     // searched decisions in 'decide'
  int64_t reductions = 0;   // 'reduce' counter
  int64_t reduced = 0;      // number of reduced clauses
  int64_t reduced_sqrt = 0;
  int64_t reduced_prct = 0;
  int64_t collected = 0;      // number of collected bytes
  int64_t collections = 0;    // number of garbage collections
  int64_t hbrs = 0;           // hyper binary resolvents
  int64_t hbrsizes = 0;       // sum of hyper resolved base clauses
  int64_t hbreds = 0;         // redundant hyper binary resolvents
  int64_t hbrsubs = 0;        // subsuming hyper binary resolvents
  int64_t instried = 0;       // number of tried instantiations
  int64_t instantiated = 0;   // number of successful instantiations
  int64_t instrounds = 0;     // number of instantiation rounds
  int64_t subsumed = 0;       // number of subsumed clauses
  int64_t deduplicated = 0;   // number of removed duplicated binary clauses
  int64_t deduplications = 0; // number of deduplication phases
  int64_t strengthened = 0;   // number of strengthened clauses

  int64_t eliminated_equi =
      0; // number of successful equivalence eliminations
  int64_t eliminated_and = 0; // number of successful AND gate eliminations
  int64_t eliminated_ite = 0; // number of successful ITE gate eliminations
  int64_t eliminated_xor = 0; // number of successful XOR gate eliminations
  int64_t eliminated_def =
      0; // number of successful definition eliminations

  int64_t definitions_checked = 0;
  int64_t definitions_extracted = 0;
  int64_t definition_units = 0;
  int64_t definition_ticks = 0;

  int64_t factor = 0;
  int64_t factored = 0;
  int64_t factor_added = 0;
  int64_t variables_extension = 0;
  int64_t variables_original = 0;
  int64_t literals_factored = 0;
  int64_t clauses_unfactored = 0;
  int64_t literals_unfactored = 0;

  int64_t elimotfstr =
      0; // number of on-the-fly strengthened during elimination
  int64_t subirr = 0;     // number of subsumed irredundant clauses
  int64_t subred = 0;     // number of subsumed redundant clauses
  int64_t subtried = 0;   // number of tried subsumptions
  int64_t subchecks = 0;  // number of pair-wise subsumption checks
  int64_t subchecks2 = 0; // same but restricted to binary clauses
  int64_t elimotfsub =
      0; // number of on-the-fly subsumed during elimination
  int64_t subsumerounds = 0; // number of subsumption rounds
  int64_t subsumephases = 0; // number of scheduled subsumption phases
  int64_t eagertried = 0; // number of traversed eager subsumed candidates
  int64_t eagersub =
      0; // number of eagerly subsumed recently learned clauses
  int64_t elimres = 0;        // number of resolved clauses in BVE
  int64_t elimrestried = 0;   // number of tried resolved clauses in BVE
  int64_t elimfastrounds = 0; // number of elimination rounds
  int64_t elimrounds = 0;     // number of elimination rounds
  int64_t elimphases = 0;     // number of scheduled elimination phases
  int64_t elimfastphases = 0; // number of scheduled elimination phases
  int64_t elimcompleted = 0;  // number complete elimination procedures
  int64_t elimtried = 0;      // number of variable elimination attempts
  int64_t elimsubst = 0;  // number of eliminations through substitutions
  int64_t elimgates = 0;  // number of gates found during elimination
  int64_t elimequivs = 0; // number of equivalences found during elimination
  int64_t elimands = 0;   // number of AND gates found during elimination
  int64_t elimites = 0;   // number of ITE gates found during elimination
  int64_t elimxors = 0;   // number of XOR gates found during elimination
  int64_t elimbwsub = 0;  // number of eager backward subsumed clauses
  int64_t elimbwstr = 0;  // number of eager backward strengthened clauses
  int64_t ternary = 0;    // number of ternary resolution phases
  int64_t ternres = 0;    // number of ternary resolutions
  int64_t htrs = 0;       // number of hyper ternary resolvents
  int64_t htrs2 = 0;      // number of binary hyper ternary resolvents
  int64_t htrs3 = 0;      // number of ternary hyper ternary resolvents
  int64_t decompositions = 0; // number of SCC + ELS
  int64_t vivifications = 0;  // number of vivifications
  int64_t vivifychecks = 0;   // checked clauses during vivification
  int64_t vivifydecs = 0;     // vivification decisions
  int64_t vivifyreused = 0;   // reused vivification decisions
  int64_t vivifysched = 0;    // scheduled clauses for vivification
  int64_t vivifysubs = 0;     // subsumed clauses during vivification
  int64_t vivifysubred = 0;   // subsumed clauses during vivification
  int64_t vivifysubirr = 0;   // subsumed clauses during vivification
  int64_t vivifystrs = 0;     // strengthened clauses during vivification
  int64_t vivifystrirr = 0;   // strengthened irredundant clause
  int64_t vivifystred1 = 0;   // strengthened redundant clause (1)
  int64_t vivifystred2 = 0;   // strengthened redundant clause (2)
  int64_t vivifystred3 = 0;   // strengthened redundant clause (3)
  int64_t vivifyunits = 0;    // units during vivification
  int64_t vivifyimplied = 0;  // implied during vivification
  int64_t vivifyinst = 0;     // instantiation during vivification
  int64_t vivifydemote = 0;   // demoting during vivification
  int64_t transreds = 0;
  int64_t transitive = 0;
  struct {
    int64_t literals = 0;
    int64_t clauses = 0;
  } learned;
  int64_t minimized = 0;    // minimized literals
  int64_t shrunken = 0;     // shrunken literals
  int64_t minishrunken = 0; // shrunken during minimization literals

  int64_t irrlits = 0; // literals in irredundant clauses
  struct {
    int64_t bytes = 0;
    int64_t clauses = 0;
    int64_t literals = 0;
  } garbage;

  int64_t sweep_units = 0;
  int64_t sweep_flip_backbone = 0;
  int64_t sweep_fixed_backbone = 0;
  int64_t sweep_flipped_backbone = 0;
  int64_t sweep_solved_backbone = 0;
  int64_t sweep_sat_backbone = 0;
  int64_t sweep_unsat_backbone = 0;
  int64_t sweep_unknown_backbone = 0;
  int64_t sweep_flip_equivalences = 0;
  int64_t sweep_flipped_equivalences = 0;
  int64_t sweep_sat_equivalences = 0;
  int64_t sweep_unsat_equivalences = 0;
  int64_t sweep_unknown_equivalences = 0;
  int64_t sweep_solved_equivalences = 0;
  int64_t sweep_equivalences = 0;
  int64_t sweep_variables = 0;
  int64_t sweep_completed = 0;
  int64_t sweep_solved = 0;
  int64_t sweep_sat = 0;
  int64_t sweep_unsat = 0;
  int64_t sweep_depth = 0;
  int64_t sweep_environment = 0;
  int64_t sweep_clauses = 0;
  int64_t sweep = 0;

  int64_t units = 0;           // learned unit clauses
  int64_t binaries = 0;        // learned binary clauses
  int64_t inprobingphases = 0; // number of scheduled probing phases
  int64_t probingrounds = 0;   // number of probing rounds
  int64_t inprobesuccess = 0;  // number successful probing phases
  int64_t probed = 0;          // number of probed literals
  int64_t failed = 0;          // number of failed literals
  int64_t hyperunary = 0;      // hyper unary resolved unit clauses
  int64_t probefailed = 0;     // failed literals from probing
  int64_t transredunits = 0;   // units derived in transitive reduction
  int64_t blockings = 0;       // number of blocked clause eliminations
  int64_t blocked = 0;         // number of actually blocked clauses
  int64_t blockres = 0;        // number of resolutions during blocking
  int64_t blockcands = 0;      // number of clause / pivot pairs tried
  int64_t blockpured = 0; // number of clauses blocked through pure literals
  int64_t blockpurelits = 0; // number of pure literals
  int64_t extensions = 0;    // number of extended witnesses
  int64_t extended = 0;      // number of flipped literals during extension
  int64_t weakened = 0;      // number of clauses pushed to extension stack
  int64_t weakenedlen = 0;   // lengths of weakened clauses
  int64_t restorations = 0;  // number of restore calls
  int64_t restored = 0;      // number of restored clauses
  int64_t reactivated = 0;   // number of reactivated clauses
  int64_t restoredlits = 0;  // number of restored literals

  int64_t preprocessings = 0;

  int64_t ilbtriggers = 0;
  int64_t ilbsuccess = 0;
  int64_t levelsreused = 0;
  int64_t literalsreused = 0;
  int64_t assumptionsreused = 0;
  int64_t tierecomputed = 0; // number of tier recomputation;

  struct {
    int64_t fixed = 0;          // number of top level assigned variables
    int64_t eliminated = 0;     // number of eliminated variables
    int64_t fasteliminated = 0; // number of fast eliminated variables only
    int64_t substituted = 0;    // number of substituted variables
    int64_t pure = 0;           // number of pure literals
  } all, now;

  struct {
    int64_t strengthened = 0; // number of clauses strengthened during OTFS
    int64_t subsumed = 0;     // number of clauses subsumed by OTFS
  } otfs;

  int64_t unused = 0;   // number of unused variables
  int64_t active = 0;   // number of active variables
  int64_t inactive = 0; // number of inactive variables
  std::vector<uint64_t> bump_used = {0, 0};
  std::vector<std::vector<uint64_t>> used; // used clauses in focused mode

  struct {
    int64_t gates = 0;
    int64_t ands = 0;
    int64_t ites = 0;
    int64_t xors = 0;
    int64_t units = 0;
    int64_t congruent = 0;
    int64_t rounds = 0;
    int64_t unary_and = 0;
    int64_t unaries = 0;
    int64_t rewritten_ands = 0;
    int64_t simplified = 0;
    int64_t simplified_ands = 0;
    int64_t simplified_xors = 0;
    int64_t simplified_ites = 0;
    int64_t subsumed = 0;
    int64_t trivial_ite = 0;
    int64_t unary_ites = 0;
  } congruence;

  Stats ();

  void print (Internal *);
};

/*------------------------------------------------------------------------*/

} // namespace CaDiCaL

ABC_NAMESPACE_CXX_HEADER_END

#endif
