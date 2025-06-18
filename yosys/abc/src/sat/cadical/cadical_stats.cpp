// vim: set tw=300: set VIM text width to 300 characters for this file.
#include "global.h"


#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

Stats::Stats () {
  time.real = absolute_real_time ();
  time.process = absolute_process_time ();
  walk.minimum = LONG_MAX;
  used.resize (2);
  used[0].resize (127);
  used[1].resize (127);
}

/*------------------------------------------------------------------------*/

#define PRT(FMT, ...) \
  do { \
    if (FMT[0] == ' ' && !all) \
      break; \
    MSG (FMT, __VA_ARGS__); \
  } while (0)

/*------------------------------------------------------------------------*/

void Stats::print (Internal *internal) {

#ifdef CADICAL_QUIET
  (void) internal;
#else

  Stats &stats = internal->stats;

  int all = internal->opts.verbose > 0 || internal->opts.stats;
#ifdef LOGGING
  if (internal->opts.log)
    all = true;
#endif // ifdef LOGGING

  if (internal->opts.profile)
    internal->print_profile ();

  double t = internal->solve_time ();

  int64_t propagations = 0;
  propagations += stats.propagations.cover;
  propagations += stats.propagations.probe;
  propagations += stats.propagations.search;
  propagations += stats.propagations.transred;
  propagations += stats.propagations.vivify;
  propagations += stats.propagations.walk;

  int64_t vivified = stats.vivifysubs + stats.vivifystrs;
  int64_t searchticks = stats.ticks.search[0] + stats.ticks.search[1];
  int64_t inprobeticks = stats.ticks.vivify + stats.ticks.probe +
                         stats.ticks.factor + stats.ticks.ternary +
                         stats.ticks.sweep;
  int64_t totalticks = searchticks + inprobeticks;

  size_t extendbytes = internal->external->extension.size ();
  extendbytes *= sizeof (int);

  SECTION ("statistics");

  if (all || stats.blocked) {
    PRT ("blocked:         %15" PRId64
         "   %10.2f %%  of irredundant clauses",
         stats.blocked, percent (stats.blocked, stats.added.irredundant));
    PRT ("  blockings:     %15" PRId64 "   %10.2f    internal",
         stats.blockings, relative (stats.conflicts, stats.blockings));
    PRT ("  candidates:    %15" PRId64 "   %10.2f    per blocking ",
         stats.blockcands, relative (stats.blockcands, stats.blockings));
    PRT ("  blockres:      %15" PRId64 "   %10.2f    per candidate",
         stats.blockres, relative (stats.blockres, stats.blockcands));
    PRT ("  pure:          %15" PRId64 "   %10.2f %%  of all variables",
         stats.all.pure, percent (stats.all.pure, stats.vars));
    PRT ("  pureclauses:   %15" PRId64 "   %10.2f    per pure literal",
         stats.blockpured, relative (stats.blockpured, stats.all.pure));
  }
  if (all || stats.chrono)
    PRT ("chronological:   %15" PRId64 "   %10.2f %%  of conflicts",
         stats.chrono, percent (stats.chrono, stats.conflicts));
  if (all)
    PRT ("compacts:        %15" PRId64 "   %10.2f    interval",
         stats.compacts, relative (stats.conflicts, stats.compacts));
  if (all || stats.conflicts) {
    PRT ("conflicts:       %15" PRId64 "   %10.2f    per second",
         stats.conflicts, relative (stats.conflicts, t));
    PRT ("  backtracked:   %15" PRId64 "   %10.2f %%  of conflicts",
         stats.backtracks, percent (stats.backtracks, stats.conflicts));
  }
  if (all || stats.conditioned) {
    PRT ("conditioned:     %15" PRId64
         "   %10.2f %%  of irredundant clauses",
         stats.conditioned,
         percent (stats.conditioned, stats.added.irredundant));
    PRT ("  conditionings: %15" PRId64 "   %10.2f    interval",
         stats.conditionings,
         relative (stats.conflicts, stats.conditionings));
    PRT ("  condcands:     %15" PRId64 "   %10.2f    candidate clauses",
         stats.condcands, relative (stats.condcands, stats.conditionings));
    PRT ("  condassinit:   %17.1f  %9.2f %%  initial assigned",
         relative (stats.condassinit, stats.conditionings),
         percent (stats.condassinit, stats.condassvars));
    PRT ("  condcondinit:  %17.1f  %9.2f %%  initial condition",
         relative (stats.condcondinit, stats.conditionings),
         percent (stats.condcondinit, stats.condassinit));
    PRT ("  condautinit:   %17.1f  %9.2f %%  initial autarky",
         relative (stats.condautinit, stats.conditionings),
         percent (stats.condautinit, stats.condassinit));
    PRT ("  condassrem:    %17.1f  %9.2f %%  final assigned",
         relative (stats.condassrem, stats.conditioned),
         percent (stats.condassrem, stats.condassirem));
    PRT ("  condcondrem:   %19.3f  %7.2f %%  final conditional",
         relative (stats.condcondrem, stats.conditioned),
         percent (stats.condcondrem, stats.condassrem));
    PRT ("  condautrem:    %19.3f  %7.2f %%  final autarky",
         relative (stats.condautrem, stats.conditioned),
         percent (stats.condautrem, stats.condassrem));
    PRT ("  condprops:     %15" PRId64 "   %10.2f    per candidate",
         stats.condprops, relative (stats.condprops, stats.condcands));
  }
  if (all || stats.cover.total) {
    PRT ("covered:         %15" PRId64
         "   %10.2f %%  of irredundant clauses",
         stats.cover.total,
         percent (stats.cover.total, stats.added.irredundant));
    PRT ("  coverings:     %15" PRId64 "   %10.2f    interval",
         stats.cover.count, relative (stats.conflicts, stats.cover.count));
    PRT ("  asymmetric:    %15" PRId64 "   %10.2f %%  of covered clauses",
         stats.cover.asymmetric,
         percent (stats.cover.asymmetric, stats.cover.total));
    PRT ("  blocked:       %15" PRId64 "   %10.2f %%  of covered clauses",
         stats.cover.blocked,
         percent (stats.cover.blocked, stats.cover.total));
  }
  if (all || stats.decisions) {
    PRT ("decisions:       %15" PRId64 "   %10.2f    per second",
         stats.decisions, relative (stats.decisions, t));
    PRT ("  searched:      %15" PRId64 "   %10.2f    per decision",
         stats.searched, relative (stats.searched, stats.decisions));
  }
  if (all || stats.all.eliminated) {
    PRT ("eliminated:      %15" PRId64 "   %10.2f %%  of all variables",
         stats.all.eliminated, percent (stats.all.eliminated, stats.vars));
    PRT ("  fastelim:      %15" PRId64 "   %10.2f %%  of eliminated",
         stats.all.fasteliminated,
         percent (stats.all.fasteliminated, stats.all.eliminated));
    PRT ("  elimphases:    %15" PRId64 "   %10.2f    interval",
         stats.elimphases, relative (stats.conflicts, stats.elimphases));
    PRT ("  elimrounds:    %15" PRId64 "   %10.2f    per phase",
         stats.elimrounds, relative (stats.elimrounds, stats.elimphases));
    PRT ("  elimtried:     %15" PRId64 "   %10.2f %%  eliminated",
         stats.elimtried, percent (stats.all.eliminated, stats.elimtried));
    PRT ("  elimgates:     %15" PRId64 "   %10.2f %%  gates per tried",
         stats.elimgates, percent (stats.elimgates, stats.elimtried));
    PRT ("  elimequivs:    %15" PRId64 "   %10.2f %%  equivalence gates",
         stats.elimequivs, percent (stats.elimequivs, stats.elimgates));
    PRT ("  elimands:      %15" PRId64 "   %10.2f %%  and gates",
         stats.elimands, percent (stats.elimands, stats.elimgates));
    PRT ("  elimites:      %15" PRId64 "   %10.2f %%  if-then-else gates",
         stats.elimites, percent (stats.elimites, stats.elimgates));
    PRT ("  elimxors:      %15" PRId64 "   %10.2f %%  xor gates",
         stats.elimxors, percent (stats.elimxors, stats.elimgates));
    PRT ("  elimdefs:      %15" PRId64 "   %10.2f %%  definitions",
         stats.definitions_extracted,
         percent (stats.definitions_extracted, stats.elimgates));
    PRT ("  elimsubst:     %15" PRId64 "   %10.2f %%  substituted",
         stats.elimsubst, percent (stats.elimsubst, stats.all.eliminated));
    PRT ("  elimsubstequi: %15" PRId64 "   %10.2f %%  equivalence gates",
         stats.eliminated_equi,
         percent (stats.eliminated_equi, stats.elimsubst));
    PRT ("  elimsubstands: %15" PRId64 "   %10.2f %%  and gates",
         stats.eliminated_and,
         percent (stats.eliminated_and, stats.elimsubst));
    PRT ("  elimsubstites: %15" PRId64 "   %10.2f %%  if-then-else gates",
         stats.eliminated_ite,
         percent (stats.eliminated_ite, stats.elimsubst));
    PRT ("  elimsubstxors: %15" PRId64 "   %10.2f %%  xor gates",
         stats.eliminated_xor,
         percent (stats.eliminated_xor, stats.elimsubst));
    PRT ("  elimsubstdefs: %15" PRId64 "   %10.2f %%  definitions",
         stats.eliminated_def,
         percent (stats.eliminated_def, stats.elimsubst));
    PRT ("  elimres:       %15" PRId64 "   %10.2f    per eliminated",
         stats.elimres, relative (stats.elimres, stats.all.eliminated));
    PRT ("  elimrestried:  %15" PRId64 "   %10.2f %%  per resolution",
         stats.elimrestried, percent (stats.elimrestried, stats.elimres));
    PRT ("  def checked:   %15" PRId64 "   %10.2f    per phase",
         stats.definitions_checked,
         relative (stats.definitions_checked, stats.elimphases));
    PRT ("  def extracted: %15" PRId64 "   %10.2f %%  per checked",
         stats.definitions_extracted,
         percent (stats.definitions_extracted, stats.definitions_checked));
    PRT ("  def units:     %15" PRId64 "   %10.2f %%  per checked",
         stats.definition_units,
         percent (stats.definition_units, stats.definitions_checked));
  }
  if (all || stats.ext_prop.ext_cb) {
    PRT ("ext.prop. calls: %15" PRId64 "   %10.2f %%  of queries",
         stats.ext_prop.eprop_call,
         percent (stats.ext_prop.eprop_call, stats.ext_prop.ext_cb));
    PRT ("  propagating:   %15" PRId64 "   %10.2f %%  per eprop-call",
         stats.ext_prop.eprop_prop,
         percent (stats.ext_prop.eprop_prop, stats.ext_prop.eprop_call));
    PRT ("  explained:     %15" PRId64 "   %10.2f %%  per eprop-call",
         stats.ext_prop.eprop_expl,
         percent (stats.ext_prop.eprop_expl, stats.ext_prop.eprop_call));
    PRT ("  falsified:     %15" PRId64 "   %10.2f %%  per eprop-call",
         stats.ext_prop.eprop_conf,
         percent (stats.ext_prop.eprop_conf, stats.ext_prop.eprop_call));
    PRT ("ext.clause calls:%15" PRId64 "   %10.2f %%  of queries",
         stats.ext_prop.elearn_call,
         percent (stats.ext_prop.elearn_call, stats.ext_prop.ext_cb));
    PRT ("  learned:       %15" PRId64 "   %10.2f %%  per called",
         stats.ext_prop.elearned,
         percent (stats.ext_prop.elearned, stats.ext_prop.elearn_call));
    PRT ("  conflicting:   %15" PRId64 "   %10.2f %%  per learned",
         stats.ext_prop.elearn_conf,
         percent (stats.ext_prop.elearn_conf, stats.ext_prop.elearned));
    PRT ("  propagating:   %15" PRId64 "   %10.2f %%  per learned",
         stats.ext_prop.elearn_prop,
         percent (stats.ext_prop.elearn_prop, stats.ext_prop.elearned));
    PRT ("ext.final check: %15" PRId64 "   %10.2f %%  of queries",
         stats.ext_prop.echeck_call,
         percent (stats.ext_prop.echeck_call, stats.ext_prop.ext_cb));
  }
  if (all || stats.factored) {
    PRT ("factored:        %15" PRId64 "   %10.2f %%  of variables",
         stats.factored, percent (stats.factored, internal->max_var));
    PRT ("  factor:        %15" PRId64 "   %10.2f    conflict interval",
         stats.factor, relative (stats.conflicts, stats.factor));
    PRT ("  cls factored:  %15" PRId64 "   %10.2f    per factored",
         stats.factor_added, relative (stats.factor_added, factored));
    PRT ("  lits factored: %15" PRId64 "   %10.2f    per factored",
         stats.literals_factored,
         relative (stats.literals_factored, factored));
    PRT ("  cls unfactored:%15" PRId64 "   %10.2f    per factored",
         stats.clauses_unfactored,
         relative (stats.clauses_unfactored, factored));
    PRT ("  lits unfactored:%14" PRId64 "   %10.2f    per factored",
         stats.literals_unfactored,
         relative (stats.literals_unfactored, factored));
  }
  if (all || stats.all.fixed) {
    PRT ("fixed:           %15" PRId64 "   %10.2f %%  of all variables",
         stats.all.fixed, percent (stats.all.fixed, stats.vars));
    PRT ("  failed:        %15" PRId64 "   %10.2f %%  of all variables",
         stats.failed, percent (stats.failed, stats.vars));
    PRT ("  probefailed:   %15" PRId64 "   %10.2f %%  per failed",
         stats.probefailed, percent (stats.probefailed, stats.failed));
    PRT ("  transredunits: %15" PRId64 "   %10.2f %%  per failed",
         stats.transredunits, percent (stats.transredunits, stats.failed));
    PRT ("  inprobephases: %15" PRId64 "   %10.2f    interval",
         stats.inprobingphases,
         relative (stats.conflicts, stats.inprobingphases));
    PRT ("  inprobesuccess:%15" PRId64 "   %10.2f %%  phases",
         stats.inprobesuccess,
         percent (stats.inprobesuccess, stats.inprobingphases));
    PRT ("  probingrounds: %15" PRId64 "   %10.2f    per phase",
         stats.probingrounds,
         relative (stats.probingrounds, stats.inprobingphases));
    PRT ("  probed:        %15" PRId64 "   %10.2f    per failed",
         stats.probed, relative (stats.probed, stats.failed));
    PRT ("  hbrs:          %15" PRId64 "   %10.2f    per probed",
         stats.hbrs, relative (stats.hbrs, stats.probed));
    PRT ("  hbrsizes:      %15" PRId64 "   %10.2f    per hbr",
         stats.hbrsizes, relative (stats.hbrsizes, stats.hbrs));
    PRT ("  hbreds:        %15" PRId64 "   %10.2f %%  per hbr",
         stats.hbreds, percent (stats.hbreds, stats.hbrs));
    PRT ("  hbrsubs:       %15" PRId64 "   %10.2f %%  per hbr",
         stats.hbrsubs, percent (stats.hbrsubs, stats.hbrs));
  }
  PRT ("  units:         %15" PRId64 "   %10.2f    interval", stats.units,
       relative (stats.conflicts, stats.units));
  PRT ("  binaries:      %15" PRId64 "   %10.2f    interval",
       stats.binaries, relative (stats.conflicts, stats.binaries));
  if (all || stats.flush.learned) {
    PRT ("flushed:         %15" PRId64 "   %10.2f %%  per conflict",
         stats.flush.learned,
         percent (stats.flush.learned, stats.conflicts));
    PRT ("  hyper:         %15" PRId64 "   %10.2f %%  per conflict",
         stats.flush.hyper, relative (stats.flush.hyper, stats.conflicts));
    PRT ("  flushings:     %15" PRId64 "   %10.2f    interval",
         stats.flush.count, relative (stats.conflicts, stats.flush.count));
  }
  if (all || stats.instantiated) {
    PRT ("instantiated:    %15" PRId64 "   %10.2f %%  of tried",
         stats.instantiated, percent (stats.instantiated, stats.instried));
    PRT ("  instrounds:    %15" PRId64 "   %10.2f %%  of elimrounds",
         stats.instrounds, percent (stats.instrounds, stats.elimrounds));
  }
  if (all || stats.conflicts) {
    PRT ("learned:         %15" PRId64 "   %10.2f %%  per conflict",
         stats.learned.clauses,
         percent (stats.learned.clauses, stats.conflicts));
    PRT ("@ bumped:        %15" PRId64 "   %10.2f    per learned",
         stats.bumped, relative (stats.bumped, stats.learned.clauses));
    PRT ("  recomputed:    %15" PRId64 "   %10.2f %%  per learned",
         stats.recomputed,
         percent (stats.recomputed, stats.learned.clauses));
    PRT ("  promoted1:     %15" PRId64 "   %10.2f %%  per learned",
         stats.promoted1, percent (stats.promoted1, stats.learned.clauses));
    PRT ("  promoted2:     %15" PRId64 "   %10.2f %%  per learned",
         stats.promoted2, percent (stats.promoted2, stats.learned.clauses));
    PRT ("  improvedglue:  %15" PRId64 "   %10.2f %%  per learned",
         stats.improvedglue,
         percent (stats.improvedglue, stats.learned.clauses));
  }
  if (all || stats.lucky.succeeded) {
    PRT ("lucky:           %15" PRId64 "   %10.2f %%  of tried",
         stats.lucky.succeeded,
         percent (stats.lucky.succeeded, stats.lucky.tried));
    PRT ("  constantzero   %15" PRId64 "   %10.2f %%  of tried",
         stats.lucky.constant.zero,
         percent (stats.lucky.constant.zero, stats.lucky.tried));
    PRT ("  constantone    %15" PRId64 "   %10.2f %%  of tried",
         stats.lucky.constant.one,
         percent (stats.lucky.constant.one, stats.lucky.tried));
    PRT ("  backwardone    %15" PRId64 "   %10.2f %%  of tried",
         stats.lucky.backward.one,
         percent (stats.lucky.backward.one, stats.lucky.tried));
    PRT ("  backwardzero   %15" PRId64 "   %10.2f %%  of tried",
         stats.lucky.backward.zero,
         percent (stats.lucky.backward.zero, stats.lucky.tried));
    PRT ("  forwardone     %15" PRId64 "   %10.2f %%  of tried",
         stats.lucky.forward.one,
         percent (stats.lucky.forward.one, stats.lucky.tried));
    PRT ("  forwardzero    %15" PRId64 "   %10.2f %%  of tried",
         stats.lucky.forward.zero,
         percent (stats.lucky.forward.zero, stats.lucky.tried));
    PRT ("  positivehorn   %15" PRId64 "   %10.2f %%  of tried",
         stats.lucky.horn.positive,
         percent (stats.lucky.horn.positive, stats.lucky.tried));
    PRT ("  negativehorn   %15" PRId64 "   %10.2f %%  of tried",
         stats.lucky.horn.negative,
         percent (stats.lucky.horn.negative, stats.lucky.tried));
  }
  PRT ("  extendbytes:   %15zd   %10.2f    bytes and MB", extendbytes,
       extendbytes / (double) (1l << 20));
  if (all || stats.learned.clauses)
    PRT ("learned_lits:    %15" PRId64 "   %10.2f %%  learned literals",
         stats.learned.literals,
         percent (stats.learned.literals, stats.learned.literals));
  PRT ("minimized:       %15" PRId64 "   %10.2f %%  learned literals",
       stats.minimized, percent (stats.minimized, stats.learned.literals));
  PRT ("shrunken:        %15" PRId64 "   %10.2f %%  learned literals",
       stats.shrunken, percent (stats.shrunken, stats.learned.literals));
  PRT ("minishrunken:    %15" PRId64 "   %10.2f %%  learned literals",
       stats.minishrunken,
       percent (stats.minishrunken, stats.learned.literals));

  if (all || stats.conflicts) {
    PRT ("otfs:            %15" PRId64 "   %10.2f %%  of conflict",
         stats.otfs.subsumed + stats.otfs.strengthened,
         percent (stats.otfs.subsumed + stats.otfs.strengthened,
                  stats.conflicts));
    PRT ("  subsumed       %15" PRId64 "   %10.2f %%  of conflict",
         stats.otfs.subsumed,
         percent (stats.otfs.subsumed, stats.conflicts));
    PRT ("  strengthened   %15" PRId64 "   %10.2f %%  of conflict",
         stats.otfs.strengthened,
         percent (stats.otfs.strengthened, stats.conflicts));
  }

  PRT ("propagations:    %15" PRId64 "   %10.2f M  per second",
       propagations, relative (propagations / 1e6, t));
  PRT ("  coverprops:    %15" PRId64 "   %10.2f %%  of propagations",
       stats.propagations.cover,
       percent (stats.propagations.cover, propagations));
  PRT ("  probeprops:    %15" PRId64 "   %10.2f %%  of propagations",
       stats.propagations.probe,
       percent (stats.propagations.probe, propagations));
  PRT ("  searchprops:   %15" PRId64 "   %10.2f %%  of propagations",
       stats.propagations.search,
       percent (stats.propagations.search, propagations));
  PRT ("  transredprops: %15" PRId64 "   %10.2f %%  of propagations",
       stats.propagations.transred,
       percent (stats.propagations.transred, propagations));
  PRT ("  vivifyprops:   %15" PRId64 "   %10.2f %%  of propagations",
       stats.propagations.vivify,
       percent (stats.propagations.vivify, propagations));
  PRT ("  walkprops:     %15" PRId64 "   %10.2f %%  of propagations",
       stats.propagations.walk,
       percent (stats.propagations.walk, propagations));
  if (all || stats.reactivated) {
    PRT ("reactivated:     %15" PRId64 "   %10.2f %%  of all variables",
         stats.reactivated, percent (stats.reactivated, stats.vars));
  }
  if (all || stats.reduced) {
    PRT ("reduced:         %15" PRId64 "   %10.2f %%  per conflict",
         stats.reduced, percent (stats.reduced, stats.conflicts));
    PRT ("  reductions:    %15" PRId64 "   %10.2f    interval",
         stats.reductions, relative (stats.conflicts, stats.reductions));
    PRT ("  sqrt scheme:   %15" PRId64 "   %10.2f %%  reductions",
         stats.reduced_sqrt,
         relative (stats.reduced_sqrt, stats.reductions));
    PRT ("  prct scheme:   %15" PRId64 "   %10.2f %%  reductions",
         stats.reduced_prct,
         relative (stats.reduced_prct, stats.reductions));
    PRT ("  collections:   %15" PRId64 "   %10.2f    interval",
         stats.collections, relative (stats.conflicts, stats.collections));
  }
  if (all || stats.rephased.total) {
    PRT ("rephased:        %15" PRId64 "   %10.2f    interval",
         stats.rephased.total,
         relative (stats.conflicts, stats.rephased.total));
    PRT ("  rephasedbest:  %15" PRId64 "   %10.2f %%  rephased best",
         stats.rephased.best,
         percent (stats.rephased.best, stats.rephased.total));
    PRT ("  rephasedflip:  %15" PRId64 "   %10.2f %%  rephased flipping",
         stats.rephased.flipped,
         percent (stats.rephased.flipped, stats.rephased.total));
    PRT ("  rephasedinv:   %15" PRId64 "   %10.2f %%  rephased inverted",
         stats.rephased.inverted,
         percent (stats.rephased.inverted, stats.rephased.total));
    PRT ("  rephasedorig:  %15" PRId64 "   %10.2f %%  rephased original",
         stats.rephased.original,
         percent (stats.rephased.original, stats.rephased.total));
    PRT ("  rephasedrand:  %15" PRId64 "   %10.2f %%  rephased random",
         stats.rephased.random,
         percent (stats.rephased.random, stats.rephased.total));
    PRT ("  rephasedwalk:  %15" PRId64 "   %10.2f %%  rephased walk",
         stats.rephased.walk,
         percent (stats.rephased.walk, stats.rephased.total));
  }
  if (all)
    PRT ("rescored:        %15" PRId64 "   %10.2f    interval",
         stats.rescored, relative (stats.conflicts, stats.rescored));
  if (all || stats.restarts) {
    PRT ("restarts:        %15" PRId64 "   %10.2f    interval",
         stats.restarts, relative (stats.conflicts, stats.restarts));
    PRT ("  reused:        %15" PRId64 "   %10.2f %%  per restart",
         stats.reused, percent (stats.reused, stats.restarts));
    PRT ("  reusedlevels:  %15" PRId64 "   %10.2f %%  per restart levels",
         stats.reusedlevels,
         percent (stats.reusedlevels, stats.restartlevels));
  }
  if (all || stats.restored) {
    PRT ("restored:        %15" PRId64 "   %10.2f %%  per weakened",
         stats.restored, percent (stats.restored, stats.weakened));
    PRT ("  restorations:  %15" PRId64 "   %10.2f %%  per extension",
         stats.restorations,
         percent (stats.restorations, stats.extensions));
    PRT ("  literals:      %15" PRId64 "   %10.2f    per restored clause",
         stats.restoredlits, relative (stats.restoredlits, stats.restored));
  }
  if (all || stats.stabphases) {
    PRT ("stabilizing:     %15" PRId64 "   %10.2f %%  of conflicts",
         stats.stabphases, percent (stats.stabconflicts, stats.conflicts));
    PRT ("  restartstab:   %15" PRId64 "   %10.2f %%  of all restarts",
         stats.restartstable,
         percent (stats.restartstable, stats.restarts));
    PRT ("  reusedstab:    %15" PRId64 "   %10.2f %%  per stable restarts",
         stats.reusedstable,
         percent (stats.reusedstable, stats.restartstable));
  }
  if (all || stats.all.substituted) {
    PRT ("substituted:     %15" PRId64 "   %10.2f %%  of all variables",
         stats.all.substituted,
         percent (stats.all.substituted, stats.vars));
    PRT ("  decompositions:%15" PRId64 "   %10.2f    per phase",
         stats.decompositions,
         relative (stats.decompositions, stats.inprobingphases));
  }
  if (all || stats.sweep_equivalences) {
    PRT ("sweep equivs:    %15" PRId64 "   %10.2f %%  of swept variables",
         stats.sweep_equivalences,
         percent (stats.sweep_equivalences, stats.sweep_variables));
    PRT ("  sweepings:     %15" PRId64 "   %10.2f    vars per sweeping",
         stats.sweep, relative (stats.sweep_variables, stats.sweep));
    PRT ("  swept vars:    %15" PRId64 "   %10.2f %%  of all variables",
         stats.sweep_variables,
         percent (stats.sweep_variables, stats.vars));
    PRT ("  sweep units:   %15" PRId64 "   %10.2f %%  of all variables",
         stats.sweep_units, percent (stats.sweep_units, stats.vars));
    PRT ("  solved:        %15" PRId64 "   %10.2f    per swept variable",
         stats.sweep_solved,
         relative (stats.sweep_solved, stats.sweep_variables));
    PRT ("  sat:           %15" PRId64 "   %10.2f %%  solved",
         stats.sweep_sat, percent (stats.sweep_sat, stats.sweep_solved));
    PRT ("  unsat:         %15" PRId64 "   %10.2f %%  solved",
         stats.sweep_unsat,
         percent (stats.sweep_unsat, stats.sweep_solved));
    PRT ("  backbone solved:%14" PRId64 "   %10.2f %%  solved",
         stats.sweep_solved_backbone,
         percent (stats.sweep_solved_backbone, stats.sweep_solved));
    PRT ("    sat:         %15" PRId64 "   %10.2f %%  backbone solved",
         stats.sweep_sat_backbone,
         percent (stats.sweep_sat_backbone, stats.sweep_solved_backbone));
    PRT ("    unsat:       %15" PRId64 "   %10.2f %%  backbone solved",
         stats.sweep_unsat_backbone,
         percent (stats.sweep_unsat_backbone, stats.sweep_solved_backbone));
    PRT ("    unknown:     %15" PRId64 "   %10.2f %%  backbone solved",
         stats.sweep_unknown_backbone,
         percent (stats.sweep_unknown_backbone,
                  stats.sweep_solved_backbone));
    PRT ("    fixed:       %15" PRId64 "   %10.2f   per swept variable",
         stats.sweep_fixed_backbone,
         relative (stats.sweep_fixed_backbone, stats.sweep_variables));
    PRT ("    flip:        %15" PRId64 "   %10.2f   per swept variable",
         stats.sweep_flip_backbone,
         relative (stats.sweep_flip_backbone, stats.sweep_variables));
    PRT ("    flipped:     %15" PRId64 "   %10.2f %%  of backbone flip",
         stats.sweep_flipped_backbone,
         percent (stats.sweep_flipped_backbone, stats.sweep_flip_backbone));
    PRT ("  equiv solved:  %15" PRId64 "   %10.2f %%  solved",
         stats.sweep_solved_equivalences,
         percent (stats.sweep_solved_equivalences, stats.sweep_solved));
    PRT ("    sat:         %15" PRId64 "   %10.2f %%  equiv solved",
         stats.sweep_sat_equivalences,
         percent (stats.sweep_sat_equivalences,
                  stats.sweep_solved_equivalences));
    PRT ("    unsat:       %15" PRId64 "   %10.2f %%  equiv solved",
         stats.sweep_unsat_equivalences,
         percent (stats.sweep_unsat_equivalences,
                  stats.sweep_solved_equivalences));
    PRT ("    unknown:     %15" PRId64 "   %10.2f %%  equiv solved",
         stats.sweep_unknown_equivalences,
         percent (stats.sweep_unknown_equivalences,
                  stats.sweep_solved_equivalences));
    PRT ("    flip:        %15" PRId64 "   %10.2f    per swept variable",
         stats.sweep_flip_equivalences,
         relative (stats.sweep_flip_equivalences, stats.sweep_variables));
    PRT ("    flipped:     %15" PRId64 "   %10.2f %%  of equiv flip",
         stats.sweep_flipped_equivalences,
         percent (stats.sweep_flipped_equivalences,
                  stats.sweep_flip_equivalences));
    PRT ("  depth:         %15" PRId64 "   %10.2f    per swept variable",
         stats.sweep_depth,
         relative (stats.sweep_depth, stats.sweep_variables));
    PRT ("  environment:   %15" PRId64 "   %10.2f    per swept variable",
         stats.sweep_environment,
         relative (stats.sweep_environment, stats.sweep_variables));
    PRT ("  clauses:       %15" PRId64 "   %10.2f    per swept variable",
         stats.sweep_clauses,
         relative (stats.sweep_clauses, stats.sweep_variables));
    PRT ("  completed:     %15" PRId64 "   %10.2f    sweeps to complete",
         stats.sweep_completed,
         relative (stats.sweep, stats.sweep_completed));
  }
  if (all || stats.subsumed) {
    PRT ("subsumed:        %15" PRId64 "   %10.2f %%  of all clauses",
         stats.subsumed, percent (stats.subsumed, stats.added.total));
    PRT ("  subsumephases: %15" PRId64 "   %10.2f    interval",
         stats.subsumephases,
         relative (stats.conflicts, stats.subsumephases));
    PRT ("  subsumerounds: %15" PRId64 "   %10.2f    per phase",
         stats.subsumerounds,
         relative (stats.subsumerounds, stats.subsumephases));
    PRT ("  deduplicated:  %15" PRId64 "   %10.2f %%  per subsumed",
         stats.deduplicated, percent (stats.deduplicated, stats.subsumed));
    PRT ("  transreds:     %15" PRId64 "   %10.2f    interval",
         stats.transreds, relative (stats.conflicts, stats.transreds));
    PRT ("  transitive:    %15" PRId64 "   %10.2f %%  per subsumed",
         stats.transitive, percent (stats.transitive, stats.subsumed));
    PRT ("  subirr:        %15" PRId64 "   %10.2f %%  of subsumed",
         stats.subirr, percent (stats.subirr, stats.subsumed));
    PRT ("  subred:        %15" PRId64 "   %10.2f %%  of subsumed",
         stats.subred, percent (stats.subred, stats.subsumed));
    PRT ("  subtried:      %15" PRId64 "   %10.2f    tried per subsumed",
         stats.subtried, relative (stats.subtried, stats.subsumed));
    PRT ("  subchecks:     %15" PRId64 "   %10.2f    per tried",
         stats.subchecks, relative (stats.subchecks, stats.subtried));
    PRT ("  subchecks2:    %15" PRId64 "   %10.2f %%  per subcheck",
         stats.subchecks2, percent (stats.subchecks2, stats.subchecks));
    PRT ("  elimotfsub:    %15" PRId64 "   %10.2f %%  of subsumed",
         stats.elimotfsub, percent (stats.elimotfsub, stats.subsumed));
    PRT ("  elimbwsub:     %15" PRId64 "   %10.2f %%  of subsumed",
         stats.elimbwsub, percent (stats.elimbwsub, stats.subsumed));
    PRT ("  eagersub:      %15" PRId64 "   %10.2f %%  of subsumed",
         stats.eagersub, percent (stats.eagersub, stats.subsumed));
    PRT ("  eagertried:    %15" PRId64 "   %10.2f    tried per eagersub",
         stats.eagertried, relative (stats.eagertried, stats.eagersub));
  }
  if (all || stats.strengthened) {
    PRT ("strengthened:    %15" PRId64 "   %10.2f %%  of all clauses",
         stats.strengthened,
         percent (stats.strengthened, stats.added.total));
    PRT ("  elimotfstr:    %15" PRId64 "   %10.2f %%  of strengthened",
         stats.elimotfstr, percent (stats.elimotfstr, stats.strengthened));
    PRT ("  elimbwstr:     %15" PRId64 "   %10.2f %%  of strengthened",
         stats.elimbwstr, percent (stats.elimbwstr, stats.strengthened));
  }
  if (all || stats.htrs) {
    PRT ("ternary:         %15" PRId64 "   %10.2f %%  of resolved",
         stats.htrs, percent (stats.htrs, stats.ternres));
    PRT ("  phases:        %15" PRId64 "   %10.2f    interval",
         stats.ternary, relative (stats.conflicts, stats.ternary));
    PRT ("  htr3:          %15" PRId64
         "   %10.2f %%  ternary hyper ternres",
         stats.htrs3, percent (stats.htrs3, stats.htrs));
    PRT ("  htr2:          %15" PRId64 "   %10.2f %%  binary hyper ternres",
         stats.htrs2, percent (stats.htrs2, stats.htrs));
  }
  PRT ("ticks:           %15" PRId64 "   %10.2f    propagation", totalticks,
       relative (totalticks, stats.propagations.search));
  PRT (" searchticks:    %15" PRId64 "   %10.2f %%  totalticks",
       searchticks, percent (searchticks, totalticks));
  PRT ("   stableticks:  %15" PRId64 "   %10.2f %%  searchticks",
       stats.ticks.search[1], percent (stats.ticks.search[1], searchticks));
  PRT ("   unstableticks:%15" PRId64 "   %10.2f %%  searchticks",
       stats.ticks.search[0], percent (stats.ticks.search[0], searchticks));
  PRT (" inprobeticks:   %15" PRId64 "   %10.2f %%  totalticks",
       inprobeticks, percent (inprobeticks, totalticks));
  PRT ("   factorticks:  %15" PRId64 "   %10.2f %%  searchticks",
       stats.ticks.factor, percent (stats.ticks.factor, searchticks));
  PRT ("   probeticks:   %15" PRId64 "   %10.2f %%  searchticks",
       stats.ticks.probe, percent (stats.ticks.probe, searchticks));
  PRT ("   sweepticks:   %15" PRId64 "   %10.2f %%  searchticks",
       stats.ticks.sweep, percent (stats.ticks.sweep, searchticks));
  PRT ("   ternaryticks: %15" PRId64 "   %10.2f %%  searchticks",
       stats.ticks.ternary, percent (stats.ticks.ternary, searchticks));
  PRT ("   vivifyticks:  %15" PRId64 "   %10.2f %%  searchticks",
       stats.ticks.vivify, percent (stats.ticks.vivify, searchticks));
  if (all) {
    PRT ("tier recomputed: %15" PRId64 "   %10.2f    interval",
         stats.tierecomputed,
         relative (stats.conflicts, stats.tierecomputed));
  }
  if (all || stats.ilbtriggers) {
    PRT ("trail reuses:    %15" PRId64 "   %10.2f %%  of incremental calls",
         stats.ilbsuccess, percent (stats.ilbsuccess, stats.ilbtriggers));
    PRT ("  levels:        %15" PRId64 "   %10.2f    per reuse",
         stats.levelsreused,
         relative (stats.levelsreused, stats.ilbsuccess));
    PRT ("  literals:      %15" PRId64 "   %10.2f    per reuse",
         stats.literalsreused,
         relative (stats.literalsreused, stats.ilbsuccess));
    PRT ("  assumptions:   %15" PRId64 "   %10.2f    per reuse",
         stats.assumptionsreused,
         relative (stats.assumptionsreused, stats.ilbsuccess));
  }
  if (all || vivified) {
    PRT ("vivified:        %15" PRId64 "   %10.2f %%  of all clauses",
         vivified, percent (vivified, stats.added.total));
    PRT ("  vivifications: %15" PRId64 "   %10.2f    interval",
         stats.vivifications,
         relative (stats.conflicts, stats.vivifications));
    PRT ("  vivifychecks:  %15" PRId64 "   %10.2f %%  per conflict",
         stats.vivifychecks, percent (stats.vivifychecks, stats.conflicts));
    PRT ("  vivifysched:   %15" PRId64 "   %10.2f %%  checks per scheduled",
         stats.vivifysched,
         percent (stats.vivifychecks, stats.vivifysched));
    PRT ("  vivifyunits:   %15" PRId64 "   %10.2f %%  per vivify check",
         stats.vivifyunits,
         percent (stats.vivifyunits, stats.vivifychecks));
    PRT ("  vivifyinst:    %15" PRId64 "   %10.2f %%  per vivify check",
         stats.vivifyinst, percent (stats.vivifyinst, stats.vivifychecks));
    PRT ("  vivifysubs:    %15" PRId64 "   %10.2f %%  per subsumed",
         stats.vivifysubs, percent (stats.vivifysubs, stats.subsumed));
    PRT ("  vivifysubred:  %15" PRId64 "   %10.2f %%  per subs",
         stats.vivifysubred,
         percent (stats.vivifysubred, stats.vivifysubs));
    PRT ("  vivifysubirr:  %15" PRId64 "   %10.2f %%  per subs",
         stats.vivifysubirr,
         percent (stats.vivifysubirr, stats.vivifysubs));
    PRT ("  vivifystrs:    %15" PRId64 "   %10.2f %%  per strengthened",
         stats.vivifystrs, percent (stats.vivifystrs, stats.strengthened));
    PRT ("  vivifystrirr:  %15" PRId64 "   %10.2f %%  per vivifystrs",
         stats.vivifystrirr,
         percent (stats.vivifystrirr, stats.vivifystrs));
    PRT ("  vivifystred1:  %15" PRId64 "   %10.2f %%  per vivifystrs",
         stats.vivifystred1,
         percent (stats.vivifystred1, stats.vivifystrs));
    PRT ("  vivifystred2:  %15" PRId64 "   %10.2f %%  per viviyfstrs",
         stats.vivifystred2,
         percent (stats.vivifystred2, stats.vivifystrs));
    PRT ("  vivifystred3:  %15" PRId64 "   %10.2f %%  per vivifystrs",
         stats.vivifystred3,
         percent (stats.vivifystred3, stats.vivifystrs));
    PRT ("  vivifydemote:  %15" PRId64 "   %10.2f %%  per vivifystrs",
         stats.vivifydemote,
         percent (stats.vivifydemote, stats.vivifystrs));
    PRT ("  vivifydecs:    %15" PRId64 "   %10.2f    per checks",
         stats.vivifydecs, relative (stats.vivifydecs, stats.vivifychecks));
    PRT ("  vivifyreused:  %15" PRId64 "   %10.2f %%  per decision",
         stats.vivifyreused,
         percent (stats.vivifyreused, stats.vivifydecs));
  }
  if (all || stats.walk.count) {
    PRT ("walked:          %15" PRId64 "   %10.2f    interval",
         stats.walk.count, relative (stats.conflicts, stats.walk.count));
#ifndef CADICAL_QUIET
    if (internal->profiles.walk.value > 0)
      PRT ("  flips:         %15" PRId64 "   %10.2f M  per second",
           stats.walk.flips,
           relative (1e-6 * stats.walk.flips,
                     internal->profiles.walk.value));
    else
#endif
      PRT ("  flips:         %15" PRId64 "   %10.2f    per walk",
           stats.walk.flips, relative (stats.walk.flips, stats.walk.count));
    if (stats.walk.minimum < LONG_MAX)
      PRT ("  minimum:       %15" PRId64 "   %10.2f %%  clauses",
           stats.walk.minimum,
           percent (stats.walk.minimum, stats.added.irredundant));
    PRT ("  broken:        %15" PRId64 "   %10.2f    per flip",
         stats.walk.broken, relative (stats.walk.broken, stats.walk.flips));
  }
  if (all || stats.weakened) {
    PRT ("weakened:        %15" PRId64 "   %10.2f    average size",
         stats.weakened, relative (stats.weakenedlen, stats.weakened));
    PRT ("  extensions:    %15" PRId64 "   %10.2f    interval",
         stats.extensions, relative (stats.conflicts, stats.extensions));
    PRT ("  flipped:       %15" PRId64 "   %10.2f    per weakened",
         stats.extended, relative (stats.extended, stats.weakened));
  }

  if (all || stats.congruence.gates) {
    PRT ("congruence:      %15" PRId64 "   %10.2f    interval",
         stats.congruence.rounds,
         relative (stats.conflicts, stats.congruence.rounds));
    PRT ("   units:        %15" PRId64 "   %10.2f    per congruent",
         stats.congruence.units,
         relative (stats.congruence.units, stats.congruence.congruent));
    PRT ("   cong-and:     %15" PRId64 "   %10.2f    per found gates",
         stats.congruence.ands,
         relative (stats.congruence.ands, stats.congruence.gates));
    PRT ("   cong-ite:     %15" PRId64 "   %10.2f    per found gates",
         stats.congruence.ites,
         relative (stats.congruence.ites, stats.congruence.gates));
    PRT ("   cong-xor:     %15" PRId64 "   %10.2f    per found gates",
         stats.congruence.xors,
         relative (stats.congruence.xors, stats.congruence.gates));
    PRT ("   congruent:    %15" PRId64 "   %10.2f    per round",
         stats.congruence.congruent,
         relative (stats.congruence.rounds, stats.congruence.congruent));
    PRT ("   unaries:      %15" PRId64 "   %10.2f    per round",
         stats.congruence.unaries,
         relative (stats.congruence.rounds, stats.congruence.unaries));
    PRT ("   rewri.-ands:  %15" PRId64 "   %10.2f    per round",
         stats.congruence.rewritten_ands,
         relative (stats.congruence.rounds,
                   stats.congruence.rewritten_ands));
    PRT ("   subsumed:     %15" PRId64 "   %10.2f    per round",
         stats.congruence.subsumed,
         relative (stats.congruence.rounds, stats.congruence.subsumed));
  }

  LINE ();
  MSG ("%sseconds are measured in %s time for solving%s",
       tout.magenta_code (), internal->opts.realtime ? "real" : "process",
       tout.normal_code ());

#endif // ifndef CADICAL_QUIET
}

void Internal::print_resource_usage () {
#ifndef CADICAL_QUIET
  SECTION ("resources");
  uint64_t m = maximum_resident_set_size ();
  MSG ("total process time since initialization: %12.2f    seconds",
       internal->process_time ());
  MSG ("total real time since initialization:    %12.2f    seconds",
       internal->real_time ());
  MSG ("maximum resident set size of process:    %12.2f    MB",
       m / (double) (1l << 20));
#endif
}

/*------------------------------------------------------------------------*/

void Checker::print_stats () {

  if (!stats.added && !stats.deleted)
    return;

  SECTION ("checker statistics");

  MSG ("checks:          %15" PRId64 "", stats.checks);
  MSG ("assumptions:     %15" PRId64 "   %10.2f    per check",
       stats.assumptions, relative (stats.assumptions, stats.checks));
  MSG ("propagations:    %15" PRId64 "   %10.2f    per check",
       stats.propagations, relative (stats.propagations, stats.checks));
  MSG ("original:        %15" PRId64 "   %10.2f %%  of all clauses",
       stats.original, percent (stats.original, stats.added));
  MSG ("derived:         %15" PRId64 "   %10.2f %%  of all clauses",
       stats.derived, percent (stats.derived, stats.added));
  MSG ("deleted:         %15" PRId64 "   %10.2f %%  of all clauses",
       stats.deleted, percent (stats.deleted, stats.added));
  MSG ("insertions:      %15" PRId64 "   %10.2f %%  of all clauses",
       stats.insertions, percent (stats.insertions, stats.added));
  MSG ("collections:     %15" PRId64 "   %10.2f    deleted per collection",
       stats.collections, relative (stats.collections, stats.deleted));
  MSG ("collisions:      %15" PRId64 "   %10.2f    per search",
       stats.collisions, relative (stats.collisions, stats.searches));
  MSG ("searches:        %15" PRId64 "", stats.searches);
  MSG ("units:           %15" PRId64 "", stats.units);
}

void LratChecker::print_stats () {

  if (!stats.added && !stats.deleted)
    return;

  SECTION ("lrat checker statistics");

  MSG ("checks:          %15" PRId64 "", stats.checks);
  MSG ("insertions:      %15" PRId64 "   %10.2f %%  of all clauses",
       stats.insertions, percent (stats.insertions, stats.added));
  MSG ("original:        %15" PRId64 "   %10.2f %%  of all clauses",
       stats.original, percent (stats.original, stats.added));
  MSG ("derived:         %15" PRId64 "   %10.2f %%  of all clauses",
       stats.derived, percent (stats.derived, stats.added));
  MSG ("deleted:         %15" PRId64 "   %10.2f %%  of all clauses",
       stats.deleted, percent (stats.deleted, stats.added));
  MSG ("finalized:       %15" PRId64 "   %10.2f %%  of all clauses",
       stats.finalized, percent (stats.finalized, stats.added));
  MSG ("collections:     %15" PRId64 "   %10.2f    deleted per collection",
       stats.collections, relative (stats.collections, stats.deleted));
  MSG ("collisions:      %15" PRId64 "   %10.2f    per search",
       stats.collisions, relative (stats.collisions, stats.searches));
  MSG ("searches:        %15" PRId64 "", stats.searches);
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
