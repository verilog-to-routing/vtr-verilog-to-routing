#ifndef _options_h_INLCUDED
#define _options_h_INLCUDED

#include <assert.h>
#include <stdbool.h>

#include "global.h"
ABC_NAMESPACE_HEADER_START

// clang-format off

#define OPTIONS \
  OPTION (ands, 1, 0, 1, "extract and eliminate and gates") \
  OPTION (backbone, 1, 0, 2, "binary clause backbone (2=eager)") \
  OPTION (backboneeffort, 20, 0, 1e5, "effort in per mille") \
  OPTION (backbonemaxrounds, 1e3, 1, INT_MAX, "maximum backbone rounds") \
  OPTION (backbonerounds, 100, 1, INT_MAX, "backbone rounds limit") \
  OPTION (bigbigfraction, 990, 0, 1000, "big binary clause fraction per mille") \
  OPTION (bump, 1, 0, 1, "enable variable bumping") \
  OPTION (bumpreasons, 1, 0, 1, "bump reason side literals too") \
  OPTION (bumpreasonslimit, 10, 1, INT_MAX, "relative reason literals limit") \
  OPTION (bumpreasonsrate, 10, 1, INT_MAX, "decision rate limit") \
  DBGOPT (check, 2, 0, 2, "check model (1) and derived clauses (2)") \
  OPTION (chrono, 1, 0, 1, "allow chronological backtracking") \
  OPTION (chronolevels, 100, 0, INT_MAX, "maximum jumped over levels") \
  OPTION (compact, 1, 0, 1, "enable compacting garbage collection") \
  OPTION (compactlim, 10, 0, 100, "compact inactive limit (in percent)") \
  OPTION (congruence, 1, 0, 1, "congruence closure on extracted gates") \
  OPTION (congruenceandarity, 1000000, 2, 50000000, "AND gate arity limit") \
  OPTION (congruenceands, 1, 0, 1, "extract AND gates for congruence closure") \
  OPTION (congruencebinaries, 1, 0, 1, "extract certain binary clauses") \
  OPTION (congruenceites, 1, 0, 1, "extract ITE gates for congruence closure") \
  OPTION (congruenceonce, 0, 0, 1, "congruence closure only initially") \
  OPTION (congruencexorarity, 4, 2, 20, "congruence XOR gate arity limit") \
  OPTION (congruencexorcounts, 2, 1, INT_MAX, "XOR counting rounds") \
  OPTION (congruencexors, 1, 0, 1, "extract XOR gates for congruence closure") \
  OPTION (decay, 50, 1, 200, "per mille scores decay") \
  OPTION (definitioncores, 2, 1, 100, "how many cores") \
  OPTION (definitions, 1, 0, 1, "extract general definitions") \
  OPTION (definitionticks, 1e6, 0, INT_MAX, "kitten ticks limits") \
  OPTION (defraglim, 75, 50, 100, "usable defragmentation limit in percent") \
  OPTION (defragsize, 1 << 18, 10, INT_MAX, "size defragmentation limit") \
  OPTION (eagersubsume, 4, 0, 4, "eagerly subsume previous learned clauses") \
  OPTION (eliminate, 1, 0, 1, "bounded variable elimination BVE") \
  OPTION (eliminatebound, 16, 0, 1 << 13, "maximum elimination bound") \
  OPTION (eliminateclslim, 100, 1, INT_MAX, "elimination clause size limit") \
  OPTION (eliminateeffort, 100, 0, 2e3, "effort in per mille") \
  OPTION (eliminateinit, 500, 0, INT_MAX, "initial elimination interval") \
  OPTION (eliminateint, 500, 10, INT_MAX, "base elimination interval") \
  OPTION (eliminateocclim, 2e3, 0, INT_MAX, "elimination occurrence limit") \
  OPTION (eliminaterounds, 2, 1, 1e4, "elimination rounds limit") \
  OPTION (emafast, 33, 10, 1e6, "fast exponential moving average window") \
  OPTION (emaslow, 1e5, 100, 1e6, "slow exponential moving average window") \
  EMBOPT (embedded, 1, 0, 1, "parse and apply embedded options") \
  OPTION (equivalences, 1, 0, 1, "extract and eliminate equivalence gates") \
  OPTION (extract, 1, 0, 1, "extract gates in variable elimination") \
  OPTION (factor, 1, 0, 1, "bounded variable addition") \
  OPTION (factorcandrounds, 2, 0, INT_MAX, "candidates reduction rounds") \
  OPTION (factoreffort, 50, 0, 1e6, "bounded variable effort in per mille") \
  OPTION (factorhops, 3, 1, 10, "structural factoring heuristic hops") \
  OPTION (factoriniticks, 700, 1, 1000000, "initial ticks ticks in millions") \
  OPTION (factorsize, 5, 2, INT_MAX, "bounded variable addition clause size") \
  OPTION (factorstructural, 0, 0, 1, "structural bounded variable addition") \
  OPTION (fastel, 1, 0, 1, "initial fast variable elimination") \
  OPTION (fastelclslim, 100, 1, INT_MAX, "fast elimination clause length limit") \
  OPTION (fastelim, 8, 1, 1000, "fast elimination resolvents limit") \
  OPTION (fasteloccs, 100, 1, 1000, "fast elimination occurrence limit") \
  OPTION (fastelrounds, 4, 1, 1000, "fast elimination rounds") \
  OPTION (fastelsub, 1, 0, 1, "forward subsuming fast variable elimination") \
  OPTION (flushproof, 0, 0, 1, "flush proof lines immediately") \
  OPTION (focusedtiers, 1, 0, 1, "always used focused mode tiers") \
  OPTION (forcephase, 0, 0, 1, "force initial phase") \
  OPTION (forward, 1, 0, 1, "forward subsumption in BVE") \
  OPTION (forwardeffort, 100, 0, 1e6, "effort in per mille") \
  OPTION (ifthenelse, 1, 0, 1, "extract and eliminate if-then-else gates") \
  OPTION (incremental, 0, 0, 1, "enable incremental solving") \
  OPTION (jumpreasons, 1, 0, 1, "jump binary reasons") \
  LOGOPT (log, 0, 0, 5, "logging level (1=on,2=more,3=check,4/5=mem)") \
  OPTION (lucky, 1, 0, 1, "try some lucky assignments") \
  OPTION (luckyearly, 1, 0, 1, "lucky assignments before preprocessing") \
  OPTION (luckylate, 1, 0, 1, "lucky assignments after preprocessing") \
  OPTION (mineffort, 10, 0, INT_MAX, "minimum absolute effort in millions") \
  OPTION (minimize, 1, 0, 1, "learned clause minimization") \
  OPTION (minimizedepth, 1e3, 1, 1e6, "minimization depth") \
  OPTION (minimizeticks, 1, 0, 1, "count ticks in minimize and shrink") \
  OPTION (modeinit, 1e3, 10, 1e8, "initial focused conflicts limit") \
  OPTION (modeint, 1e3, 10, 1e8, "focused conflicts interval") \
  OPTION (otfs, 1, 0, 1, "on-the-fly strengthening") \
  OPTION (phase, 1, 0, 1, "initial decision phase") \
  OPTION (phasesaving, 1, 0, 1, "enable phase saving") \
  OPTION (preprocess, 1, 0, 1, "initial preprocessing") \
  OPTION (preprocessbackbone, 1, 0, 1, "backbone preprocessing") \
  OPTION (preprocesscongruence, 1, 0, 1, "congruence preprocessing") \
  OPTION (preprocessfactor, 1, 0, 1, "variable addition preprocessing") \
  OPTION (preprocessprobe, 1, 0, 1, "probing preprocessing") \
  OPTION (preprocessrounds, 1, 1, INT_MAX, "initial preprocessing rounds") \
  OPTION (preprocessweep, 1, 0, 1, "sweep preprocessing") \
  OPTION (probe, 1, 0, 1, "enable probing") \
  OPTION (probeinit, 100, 0, INT_MAX, "initial probing interval") \
  OPTION (probeint, 100, 2, INT_MAX, "probing interval") \
  OPTION (proberounds, 2, 1, INT_MAX, "probing rounds") \
  NQTOPT (profile, 2, 0, 4, "profile level") \
  OPTION (promote, 1, 0, 1, "promote clauses") \
  NQTOPT (quiet, 0, 0, 1, "disable all messages") \
  OPTION (randec, 1, 0, 1, "random decisions") \
  OPTION (randecfocused, 1, 0, 1, "random decisions in focused mode") \
  OPTION (randecinit, 500, 0, INT_MAX, "random decisions interval") \
  OPTION (randecint, 500, 0, INT_MAX, "initial random decisions interval") \
  OPTION (randeclength, 10, 1, INT_MAX, "random conflicts length") \
  OPTION (randecstable, 0, 0, 1, "random decisions in stable mode") \
  OPTION (reduce, 1, 0, 1, "learned clause reduction") \
  OPTION (reducehigh, 900, 0, 1000, "high reduce fraction per mille") \
  OPTION (reduceinit, 1e3, 2, 1e5, "initial reduce interval") \
  OPTION (reduceint, 1e3, 2, 1e5, "base reduce interval") \
  OPTION (reducelow, 500, 0, 1000, "low reduce fraction per mille") \
  OPTION (reluctant, 1, 0, 1, "stable reluctant doubling restarting") \
  OPTION (reluctantint, 1 << 10, 2, 1 << 15, "reluctant interval") \
  OPTION (reluctantlim, 1 << 20, 0, 1 << 30, "reluctant limit (0=unlimited)") \
  OPTION (reorder, 2, 0, 2, "reorder decisions (1=stable-mode-only)") \
  OPTION (reorderinit, 1e4, 0, 1e5, "initial reorder interval") \
  OPTION (reorderint, 1e4, 1, 1e5, "base reorder interval") \
  OPTION (reordermaxsize, 100, 2, 256, "reorder maximum clause size") \
  OPTION (rephase, 1, 0, 1, "reinitialization of decision phases") \
  OPTION (rephaseinit, 1e3, 10, 1e5, "initial rephase interval") \
  OPTION (rephaseint, 1e3, 10, 1e5, "base rephase interval") \
  OPTION (restart, 1, 0, 1, "enable restarts") \
  OPTION (restartint, RESTARTINT_DEFAULT, 1, 1e4, "base restart interval") \
  OPTION (restartmargin, 10, 0, 25, "fast/slow margin in percent") \
  OPTION (restartreusetrail, 1, 0, 1, "restarts tries to reuse trail") \
  OPTION (seed, 0, 0, INT_MAX, "random seed") \
  OPTION (shrink, 3, 0, 3, "learned clauses (1=bin,2=lrg,3=rec)") \
  OPTION (simplify, 1, 0, 1, "enable probing and elimination") \
  OPTION (smallclauses, 1e5, 0, INT_MAX, "small clauses limit") \
  OPTION (stable, STABLE_DEFAULT, 0, 2, "enable stable search mode") \
  NQTOPT (statistics, 0, 0, 1, "print complete statistics") \
  OPTION (substitute, 1, 0, 1, "equivalent literal substitution") \
  OPTION (substituteeffort, 10, 1, 1e3, "effort in per mille") \
  OPTION (substituterounds, 2, 1, 100, "maximum substitution rounds") \
  OPTION (subsumeclslim, 1e3, 1, INT_MAX, "subsumption clause size limit") \
  OPTION (subsumeocclim, 1e3, 0, INT_MAX, "subsumption occurrence limit") \
  OPTION (sweep, 1, 0, 1, "enable SAT sweeping") \
  OPTION (sweepclauses, 1024, 0, INT_MAX, "environment clauses") \
  OPTION (sweepcomplete, 0, 0, 1, "run SAT sweeping until completion") \
  OPTION (sweepdepth, 2, 0, INT_MAX, "environment depth") \
  OPTION (sweepeffort, 100, 0, 1e4, "effort in per mille") \
  OPTION (sweepfliprounds, 1, 0, INT_MAX, "flipping rounds") \
  OPTION (sweepmaxclauses, 32768, 2, INT_MAX, "maximum environment clauses") \
  OPTION (sweepmaxdepth, 3, 1, INT_MAX, "maximum environment depth") \
  OPTION (sweepmaxvars, 8192, 2, INT_MAX, "maximum environment variables") \
  OPTION (sweeprand, 0, 0, 1, "randomize sweeping environment") \
  OPTION (sweepvars, 256, 0, INT_MAX, "environment variables") \
  OPTION (target, TARGET_DEFAULT, 0, 2, "target phases (1=stable,2=focused)") \
  OPTION (tier1, 2, 1, 100, "learned clause tier one glue limit") \
  OPTION (tier1relative, 500, 0, 1000, "relative tier one glue limit") \
  OPTION (tier2, 6, 1, 1e3, "learned clause tier two glue limit") \
  OPTION (tier2relative, 900, 0, 1000, "relative tier two glue limit") \
  OPTION (transitive, 1, 0, 1, "transitive reduction of binary clauses") \
  OPTION (transitiveeffort, 20, 0, 2e3, "effort in per mille") \
  OPTION (transitivekeep, 1, 0, 1, "keep transitivity candidates") \
  OPTION (tumble, 1, 0, 1, "tumbled external indices order") \
  NQTOPT (verbose, 0, 0, 3, "verbosity level") \
  OPTION (vivify, 1, 0, 1, "vivify clauses") \
  OPTION (vivifyeffort, 100, 0, 1e3, "effort in per mille") \
  OPTION (vivifyfocusedtiers, 1, 0, 1, "use focused tier limits") \
  OPTION (vivifyirr, 3, 0, 100, "relative irredundant effort") \
  OPTION (vivifysort, 1, 0, 1, "sort vivification candidates") \
  OPTION (vivifytier1, 3, 0, 100, "relative tier1 effort") \
  OPTION (vivifytier2, 3, 0, 100, "relative tier2 effort") \
  OPTION (vivifytier3, 1, 0, 100, "relative tier3 effort") \
  OPTION (walkeffort, 50, 0, 1e6, "effort in per mille") \
  OPTION (walkinitially, 0, 0, 1, "initial local search") \
  OPTION (warmup, 1, 0, 1, "initialize phases by unit propagation")

// clang-format on

#define TARGET_SAT 2
#define TARGET_DEFAULT 1

#define STABLE_DEFAULT 1
#define STABLE_UNSAT 0

#define RESTARTINT_DEFAULT 1
#define RESTARTINT_SAT 50

#ifdef SAT
#undef TARGET_DEFAULT
#define TARGET_DEFAULT TARGET_SAT
#undef RESTARTINT_DEFAULT
#define RESTARTINT_DEFAULT RESTARTINT_SAT
#endif

#ifdef UNSAT
#undef STABLE_DEFAULT
#define STABLE_DEFAULT STABLE_UNSAT
#endif

#if defined(LOGGING) && !defined(KISSAT_QUIET)
#define LOGOPT OPTION
#else
#define LOGOPT(...) /**/
#endif

#ifndef KISSAT_QUIET
#define NQTOPT OPTION
#else
#define NQTOPT(...) /**/
#endif

#ifndef KISSAT_NDEBUG
#define DBGOPT OPTION
#else
#define DBGOPT(...) /**/
#endif

#ifdef EMBEDDED
#define EMBOPT OPTION
#else
#define EMBOPT(...) /**/
#endif

// clang-format on

#define TIER1RELATIVE (GET_OPTION (tier1relative) / 1000.0)
#define TIER2RELATIVE (GET_OPTION (tier2relative) / 1000.0)

typedef struct opt opt;

struct opt {
  const char *name;
#ifndef KISSAT_NOPTIONS
  int value;
  const int low;
  const int high;
#else
  const int value;
#endif
  const char *description;
};

extern const opt *kissat_options_begin;
extern const opt *kissat_options_end;

#define all_options(O) \
  opt const *O = kissat_options_begin; \
  O != kissat_options_end; \
  ++O

const char *kissat_parse_option_name (const char *arg, const char *name);
bool kissat_parse_option_value (const char *val_str, int *res_ptr);

#ifndef KISSAT_NOPTIONS

void kissat_options_usage (void);

const opt *kissat_options_has (const char *name);

#define kissat_options_max_name_buffer_size ((size_t) 32)

bool kissat_options_parse_arg (const char *arg, char *name, int *val_str);
void kissat_options_print_value (int value, char *buffer);

typedef struct options options;

struct options {
#define OPTION(N, V, L, H, D) int N;
  OPTIONS
#undef OPTION
};

void kissat_init_options (options *);

int kissat_options_get (const options *, const char *name);
int kissat_options_set_opt (options *, const opt *, int new_value);
int kissat_options_set (options *, const char *name, int new_value);

void kissat_print_embedded_option_list (void);
void kissat_print_option_range_list (void);

static inline int *kissat_options_ref (const options *options,
                                       const opt *o) {
  if (!o)
    return 0;
  KISSAT_assert (kissat_options_begin <= o);
  KISSAT_assert (o < kissat_options_end);
  return (int *) options + (o - kissat_options_begin);
}

#define GET_OPTION(NAME) ((int) solver->options.NAME)

#else

void kissat_init_options (void);
int kissat_options_get (const char *name);

#define GET_OPTION(N) kissat_options_##N

#define OPTION(N, V, L, H, D) static const int GET_OPTION (N) = (int) (V);
OPTIONS
#undef OPTION
#endif
#define GET1K_OPTION(NAME) (((int64_t) 1000) * GET_OPTION (NAME))
ABC_NAMESPACE_HEADER_END

#endif
