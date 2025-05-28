#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

/*------------------------------------------------------------------------*/

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// See corresponding header file 'cadical.hpp' (!) for more information.
//
// Again, to avoid confusion, note that, 'cadical.hpp' is the header file of
// this file 'solver.cpp', since we want to call the application and main
// file 'cadical.cpp', while at the same time using 'cadical.hpp' as the
// main header file of the library (and not 'solver.hpp').

/*------------------------------------------------------------------------*/
#ifdef LOGGING

// Needs to be kept in sync with the color schemes used in 'logging.cpp'.
//
#define api_code blue_code            // API call color
#define log_code magenta_code         // standard/default logging color
#define emph_code bright_magenta_code // emphasized logging color

#endif
/*------------------------------------------------------------------------*/

// Log state transitions.

#define STATE(S) \
  do { \
    CADICAL_assert (is_power_of_two (S)); \
    if (_state == S) \
      break; \
    _state = S; \
    LOG ("API enters state %s" #S "%s", tout.emph_code (), \
         tout.normal_code ()); \
  } while (0)

void Solver::transition_to_steady_state () {
  if (state () == CONFIGURING) {
    LOG ("API leaves state %sCONFIGURING%s", tout.emph_code (),
         tout.normal_code ());
    if (internal->opts.check && internal->opts.checkproof) {
      internal->check ();
    }
  } else if (state () == SATISFIED) {
    LOG ("API leaves state %sSATISFIED%s", tout.emph_code (),
         tout.normal_code ());
    external->reset_assumptions ();
    external->reset_concluded ();
    external->reset_constraint ();
  } else if (state () == UNSATISFIED) {
    LOG ("API leaves state %sUNSATISFIED%s", tout.emph_code (),
         tout.normal_code ());
    external->reset_assumptions ();
    external->reset_concluded ();
    external->reset_constraint ();
  } else if (state() == INCONCLUSIVE) {
    external->reset_assumptions ();
    external->reset_concluded ();
    external->reset_constraint ();
  }
  if (state () != STEADY)
    STATE (STEADY);
}

/*------------------------------------------------------------------------*/
#ifdef LOGGING
/*------------------------------------------------------------------------*/

// The following logging code is useful for debugging mostly (or trying to
// understand what the solver is actually doing).  It needs to be enabled
// during configuration using the '-l' option for './configure', which
// forces 'LOGGING' to be defined during compilation.  This includes all the
// logging code, which then still needs to enabled during run-time with the
// '-l' or 'log' option.

static void log_api_call (Internal *internal, const char *name,
                          const char *suffix) {
  Logger::log (internal, "API call %s'%s ()'%s %s", tout.api_code (), name,
               tout.log_code (), suffix);
}

static void log_api_call (Internal *internal, const char *name, int arg,
                          const char *suffix) {
  Logger::log (internal, "API call %s'%s (%d)'%s %s", tout.api_code (),
               name, arg, tout.log_code (), suffix);
}

static void log_api_call (Internal *internal, const char *name,
                          const char *arg, const char *suffix) {
  Logger::log (internal, "API call %s'%s (\"%s\")'%s %s", tout.api_code (),
               name, arg, tout.log_code (), suffix);
}

static void log_api_call (Internal *internal, const char *name,
                          const char *a1, int a2, const char *s) {
  Logger::log (internal, "API call %s'%s (\"%s\", %d)'%s %s",
               tout.api_code (), name, a1, a2, tout.log_code (), s);
}

/*------------------------------------------------------------------------*/

// We factored out API call begin/end logging and use overloaded functions.

static void log_api_call_begin (Internal *internal, const char *name) {
  Logger::log_empty_line (internal);
  log_api_call (internal, name, "started");
}

static void log_api_call_begin (Internal *internal, const char *name,
                                int arg) {
  Logger::log_empty_line (internal);
  log_api_call (internal, name, arg, "started");
}

static void log_api_call_begin (Internal *internal, const char *name,
                                const char *arg) {
  Logger::log_empty_line (internal);
  log_api_call (internal, name, arg, "started");
}

static void log_api_call_begin (Internal *internal, const char *name,
                                const char *arg1, int arg2) {
  Logger::log_empty_line (internal);
  log_api_call (internal, name, arg1, arg2, "started");
}

/*------------------------------------------------------------------------*/

static void log_api_call_end (Internal *internal, const char *name) {
  log_api_call (internal, name, "succeeded");
}

static void log_api_call_end (Internal *internal, const char *name,
                              int lit) {
  log_api_call (internal, name, lit, "succeeded");
}

static void log_api_call_end (Internal *internal, const char *name,
                              const char *arg) {
  Logger::log_empty_line (internal);
  log_api_call (internal, name, arg, "succeeded");
}

static void log_api_call_end (Internal *internal, const char *name,
                              const char *arg, bool res) {
  log_api_call (internal, name, arg, res ? "succeeded" : "failed");
}

static void log_api_call_end (Internal *internal, const char *name,
                              const char *arg, int val, bool res) {
  log_api_call (internal, name, arg, val, res ? "succeeded" : "failed");
}

static void log_api_call_returns (Internal *internal, const char *name,
                                  bool res) {
  log_api_call (internal, name, res ? "returns 'true'" : "returns 'false'");
}

static void log_api_call_returns (Internal *internal, const char *name,
                                  int res) {
  char fmt[32];
  snprintf (fmt, sizeof fmt, "returns '%d'", res);
  log_api_call (internal, name, fmt);
}

static void log_api_call_returns (Internal *internal, const char *name,
                                  int64_t res) {
  char fmt[32];
  snprintf (fmt, sizeof fmt, "returns '%" PRId64 "'", res);
  log_api_call (internal, name, fmt);
}

static void log_api_call_returns (Internal *internal, const char *name,
                                  int lit, int res) {
  char fmt[32];
  snprintf (fmt, sizeof fmt, "returns '%d'", res);
  log_api_call (internal, name, lit, fmt);
}

static void log_api_call_returns (Internal *internal, const char *name,
                                  const char *arg, bool res) {
  log_api_call (internal, name, arg,
                res ? "returns 'true'" : "returns 'false'");
}

static void log_api_call_returns (Internal *internal, const char *name,
                                  int lit, bool res) {
  log_api_call (internal, name, lit,
                res ? "returns 'true'" : "returns 'false'");
}

static void log_api_call_returns (Internal *internal, const char *name,
                                  const char *arg, const char *res) {
  Logger::log (internal, "API call %s'%s (\"%s\")'%s returns '%s'",
               tout.api_code (), name, arg, tout.log_code (),
               res ? res : "<null>");
}

static void log_api_call_returns (Internal *internal, const char *name,
                                  const char *arg1, int arg2,
                                  const char *res) {
  Logger::log (internal, "API call %s'%s (\"%s\", %d)'%s returns '%s'",
               tout.api_code (), name, arg1, arg2, tout.log_code (),
               res ? res : "<null>");
}

/*------------------------------------------------------------------------*/

#define LOG_API_CALL_BEGIN(...) \
  do { \
    if (!internal->opts.log) \
      break; \
    log_api_call_begin (internal, __VA_ARGS__); \
  } while (0)

#define LOG_API_CALL_END(...) \
  do { \
    if (!internal->opts.log) \
      break; \
    log_api_call_end (internal, __VA_ARGS__); \
  } while (0)

#define LOG_API_CALL_RETURNS(...) \
  do { \
    if (!internal->opts.log) \
      break; \
    log_api_call_returns (internal, __VA_ARGS__); \
  } while (0)

/*------------------------------------------------------------------------*/
#else // end of 'then' part of 'ifdef LOGGING'
/*------------------------------------------------------------------------*/

#define LOG_API_CALL_BEGIN(...) \
  do { \
  } while (0)
#define LOG_API_CALL_END(...) \
  do { \
  } while (0)
#define LOG_API_CALL_RETURNS(...) \
  do { \
  } while (0)

/*------------------------------------------------------------------------*/
#endif // end of 'else' part of 'ifdef LOGGING'
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
#ifndef CADICAL_NTRACING
/*------------------------------------------------------------------------*/

#define TRACE(...) \
  do { \
    /*if ((this == 0)) break; */ /* gcc-12 produces warning */ \
    if ((internal == 0)) \
      break; \
    LOG_API_CALL_BEGIN (__VA_ARGS__); \
    if (!trace_api_file) \
      break; \
    trace_api_call (__VA_ARGS__); \
  } while (0)

void Solver::trace_api_call (const char *s0) const {
  CADICAL_assert (trace_api_file);
  LOG ("TRACE %s", s0);
  fprintf (trace_api_file, "%s\n", s0);
  fflush (trace_api_file);
}

void Solver::trace_api_call (const char *s0, int i1) const {
  CADICAL_assert (trace_api_file);
  LOG ("TRACE %s %d", s0, i1);
  fprintf (trace_api_file, "%s %d\n", s0, i1);
  fflush (trace_api_file);
}

void Solver::trace_api_call (const char *s0, const char *s1) const {
  CADICAL_assert (trace_api_file);
  LOG ("TRACE %s %s", s0, s1);
  fprintf (trace_api_file, "%s %s\n", s0, s1);
  fflush (trace_api_file);
}

void Solver::trace_api_call (const char *s0, const char *s1, int i2) const {
  CADICAL_assert (trace_api_file);
  LOG ("TRACE %s %s %d", s0, s1, i2);
  fprintf (trace_api_file, "%s %s %d\n", s0, s1, i2);
  fflush (trace_api_file);
}

/*------------------------------------------------------------------------*/

// The global 'tracing_api_calls_through_environment_variable_method' flag
// is used to ensure that only one solver traces to a file.  Otherwise the
// method to use an environment variable to point to the trace file is
// bogus, since those different solver instances would all write to the same
// file producing garbage.  A more sophisticated solution would use a
// different mechanism to tell the solver to which file to trace to, but in
// our experience it is quite convenient to get traces out of applications
// which use the solver as library by just setting an environment variable
// without requiring to change any application code.
//
static bool tracing_api_calls_through_environment_variable_method;

/*------------------------------------------------------------------------*/
#else // CADICAL_NTRACING
/*------------------------------------------------------------------------*/

#define TRACE(...) \
  do { \
  } while (0)

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/

static bool tracing_nb_lidrup_env_var_method = false;

Solver::Solver () {

#ifndef CADICAL_NTRACING
  const char *path = getenv ("CADICAL_API_TRACE");
  if (!path)
    path = getenv ("CADICALAPITRACE");
  if (path) {
    if (tracing_api_calls_through_environment_variable_method)
      FATAL ("can not trace API calls of two solver instances "
             "using environment variable 'CADICAL_API_TRACE'");
    if (!(trace_api_file = fopen (path, "w")))
      FATAL ("failed to open file '%s' to trace API calls "
             "using environment variable 'CADICAL_API_TRACE'",
             path);
    close_trace_api_file = true;
    tracing_api_calls_through_environment_variable_method = true;
  } else {
    tracing_api_calls_through_environment_variable_method = false;
    close_trace_api_file = false;
    trace_api_file = 0;
  }
#endif

  adding_clause = false;
  adding_constraint = false;
  _state = INITIALIZING;
  internal = new Internal ();
  DeferDeletePtr<Internal> delete_internal (internal);
  TRACE ("init");
  external = new External (internal);
  DeferDeletePtr<External> delete_external (external);
  STATE (CONFIGURING);
#ifndef CADICAL_NTRACING
  if (tracing_api_calls_through_environment_variable_method)
    message ("tracing API calls to '%s'", path);
#endif

  const char *lidrup_path = getenv ("CADICAL_LIDRUP_TRACE");
  if (!lidrup_path)
    lidrup_path = getenv ("CADICALLIDRUPTRACE");
  if (lidrup_path) {

    // if (tracing_nb_lidrup_env_var_method)
    // FATAL ("can not trace LIDRUP of two solver instances "
    //   "using environment variable 'CADICAL_LIDRUP_TRACE'");
    // Here we use the solver interface to setup non-binary IDRUP tracing to
    // the defined file. Options set by the user can and will overwrite
    // these settings if neeed be.
    set ("lidrup", 1);
    set ("binary", 0);
    trace_proof (lidrup_path);
    tracing_nb_lidrup_env_var_method = true;
  } else {
    tracing_nb_lidrup_env_var_method = false;
  }

  delete_internal.release ();
  delete_external.release ();
}

Solver::~Solver () {

  TRACE ("reset");
  REQUIRE_VALID_OR_SOLVING_STATE ();
  STATE (DELETING);

  tracing_nb_lidrup_env_var_method = false;

#ifdef LOGGING
  //
  // After deleting 'internal' logging does not work anymore.
  //
  bool logging = internal->opts.log;
  int level = internal->level;
  string prefix = internal->prefix;
#endif

  delete internal;
  delete external;

#ifndef CADICAL_NTRACING
  if (close_trace_api_file) {
    close_trace_api_file = false;
    CADICAL_assert (trace_api_file);
    CADICAL_assert (tracing_api_calls_through_environment_variable_method);
    fclose (trace_api_file);
    tracing_api_calls_through_environment_variable_method = false;
  }
#endif

#ifdef LOGGING
  //
  // Need to log success of this API call manually.
  //
  if (logging) {
    printf ("%s%sLOG %s%d%s API call %s'reset ()'%s succeeded%s\n",
            prefix.c_str (), tout.log_code (), tout.emph_code (), level,
            tout.log_code (), tout.api_code (), tout.log_code (),
            tout.normal_code ());
    fflush (stdout);
  }
#endif
}

/*------------------------------------------------------------------------*/

int Solver::vars () {
  TRACE ("vars");
  REQUIRE_VALID_OR_SOLVING_STATE ();
  int res = external->max_var;
  LOG_API_CALL_RETURNS ("vars", res);
  return res;
}

void Solver::reserve (int min_max_var) {
  TRACE ("reserve", min_max_var);
  REQUIRE_VALID_STATE ();
  transition_to_steady_state ();
  external->reset_extended ();
  external->init (min_max_var);
  LOG_API_CALL_END ("reserve", min_max_var);
}

int Solver::reserve_difference (int number_of_vars) {
  TRACE ("reserve_difference", number_of_vars);
  REQUIRE_VALID_STATE ();
  transition_to_steady_state ();
  external->reset_extended ();
  int new_max_var = external->max_var + number_of_vars;
  external->init (new_max_var);
  LOG_API_CALL_END ("reserve_difference", number_of_vars);
  return new_max_var;
}

/*------------------------------------------------------------------------*/
#ifndef CADICAL_NTRACING

void Solver::trace_api_calls (FILE *file) {
  LOG_API_CALL_BEGIN ("trace_api_calls");
  REQUIRE_VALID_STATE ();
  REQUIRE (file != 0, "invalid zero file argument");
  REQUIRE (!tracing_api_calls_through_environment_variable_method,
           "already tracing API calls "
           "using environment variable 'CADICAL_API_TRACE'");
  REQUIRE (!trace_api_file, "called twice");
  trace_api_file = file;
  LOG_API_CALL_END ("trace_api_calls");
  trace_api_call ("init");
}

#endif
/*------------------------------------------------------------------------*/

bool Solver::is_valid_option (const char *name) {
  return Options::has (name);
}

bool Solver::is_preprocessing_option (const char *name) {
  return Options::is_preprocessing_option (name);
}

bool Solver::is_valid_long_option (const char *arg) {
  string name;
  int tmp;
  return Options::parse_long_option (arg, name, tmp);
}

int Solver::get (const char *arg) {
  REQUIRE_VALID_OR_SOLVING_STATE ();
  return internal->opts.get (arg);
}

bool Solver::set (const char *arg, int val) {
  TRACE ("set", arg, val);
  REQUIRE_VALID_STATE ();
  if (strcmp (arg, "log") && strcmp (arg, "quiet") &&
      strcmp (arg, "report") && strcmp (arg, "verbose")) {
    REQUIRE (
        state () == CONFIGURING,
        "can only set option 'set (\"%s\", %d)' right after initialization",
        arg, val);
  }
  bool res = internal->opts.set (arg, val);
  LOG_API_CALL_END ("set", arg, val, res);

  return res;
}

bool Solver::set_long_option (const char *arg) {
  LOG_API_CALL_BEGIN ("set", arg);
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
           "can only set option '%s' right after initialization", arg);
  bool res;
  if (arg[0] != '-' || arg[1] != '-')
    res = false;
  else {
    int val;
    string name;
    res = Options::parse_long_option (arg, name, val);
    if (res)
      set (name.c_str (), val);
  }
  LOG_API_CALL_END ("set", arg, res);
  return res;
}

void Solver::optimize (int arg) {
  LOG_API_CALL_BEGIN ("optimize", arg);
  REQUIRE_VALID_STATE ();
  internal->opts.optimize (arg);
  LOG_API_CALL_END ("optimize", arg);
}

bool Solver::limit (const char *arg, int val) {
  TRACE ("limit", arg, val);
  REQUIRE_VALID_STATE ();
  bool res = internal->limit (arg, val);
  LOG_API_CALL_END ("limit", arg, val, res);
  return res;
}

bool Solver::is_valid_limit (const char *arg) {
  return Internal::is_valid_limit (arg);
}

void Solver::prefix (const char *str) {
  LOG_API_CALL_BEGIN ("prefix", str);
  REQUIRE_VALID_OR_SOLVING_STATE ();
  internal->prefix = str;
  LOG_API_CALL_END ("prefix", str);
}

bool Solver::is_valid_configuration (const char *name) {
  return Config::has (name);
}

bool Solver::configure (const char *name) {
  TRACE ("configure", name);
  LOG_API_CALL_BEGIN ("configure", name);
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
           "can only set configuration '%s' right after initialization",
           name);
  bool res = Config::set (internal->opts, name);
  LOG_API_CALL_END ("configure", name, res);
  return res;
}

/*===== IPASIR BEGIN =====================================================*/

void Solver::add (int lit) {
  TRACE ("add", lit);
  REQUIRE_VALID_STATE ();
  if (lit)
    REQUIRE_VALID_LIT (lit);
  transition_to_steady_state ();
  external->add (lit);
  adding_clause = lit;
  if (adding_clause)
    STATE (ADDING);
  else if (!adding_constraint)
    STATE (STEADY);
  LOG_API_CALL_END ("add", lit);
}

void Solver::clause (int a) {
  REQUIRE_VALID_LIT (a);
  add (a), add (0);
}

void Solver::clause (int a, int b) {
  REQUIRE_VALID_LIT (a);
  REQUIRE_VALID_LIT (b);
  add (a), add (b), add (0);
}

void Solver::clause (int a, int b, int c) {
  REQUIRE_VALID_LIT (a);
  REQUIRE_VALID_LIT (b);
  REQUIRE_VALID_LIT (c);
  add (a), add (b), add (c), add (0);
}

void Solver::clause (int a, int b, int c, int d) {
  REQUIRE_VALID_LIT (a);
  REQUIRE_VALID_LIT (b);
  REQUIRE_VALID_LIT (c);
  REQUIRE_VALID_LIT (d);
  add (a), add (b), add (c), add (d), add (0);
}

void Solver::clause (int a, int b, int c, int d, int e) {
  REQUIRE_VALID_LIT (a);
  REQUIRE_VALID_LIT (b);
  REQUIRE_VALID_LIT (c);
  REQUIRE_VALID_LIT (d);
  REQUIRE_VALID_LIT (e);
  add (a), add (b), add (c), add (d), add (e), add (0);
}

void Solver::clause (const int *lits, size_t size) {
  REQUIRE (!size || lits,
           "first argument 'lits' zero while second argument 'size' not");
  const int *end = lits + size;
  for (const int *p = lits; p != end; p++) {
    const int lit = *p;
    REQUIRE_VALID_LIT (lit);
    add (lit);
  }
  add (0);
}

void Solver::clause (const std::vector<int> &lits) {
  for (auto lit : lits) {
    REQUIRE_VALID_LIT (lit);
    add (lit);
  }
  add (0);
}

bool Solver::inconsistent () { return internal->unsat; }

void Solver::constrain (int lit) {
  TRACE ("constrain", lit);
  REQUIRE_VALID_STATE ();
  if (lit)
    REQUIRE_VALID_LIT (lit);
  transition_to_steady_state ();
  external->constrain (lit);
  adding_constraint = lit;
  if (adding_constraint)
    STATE (ADDING);
  else if (!adding_clause)
    STATE (STEADY);
  LOG_API_CALL_END ("constrain", lit);
}

void Solver::assume (int lit) {
  TRACE ("assume", lit);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (lit);
  transition_to_steady_state ();
  external->assume (lit);
  LOG_API_CALL_END ("assume", lit);
}

int Solver::lookahead () {
  TRACE ("lookahead");
  REQUIRE_VALID_OR_SOLVING_STATE ();
  int lit = external->lookahead ();
  TRACE ("lookahead");
  return lit;
}

Solver::CubesWithStatus Solver::generate_cubes (int depth, int min_depth) {
  TRACE ("lookahead_cubes");
  REQUIRE_VALID_OR_SOLVING_STATE ();
  auto cubes = external->generate_cubes (depth, min_depth);
  TRACE ("lookahead_cubes");

  CubesWithStatus cubes2;
  cubes2.status = cubes.status;
  cubes2.cubes = cubes.cubes;
  return cubes2;
}

void Solver::reset_assumptions () {
  TRACE ("reset_assumptions");
  REQUIRE_VALID_STATE ();
  transition_to_steady_state ();
  external->reset_assumptions ();
  external->reset_concluded ();
  LOG_API_CALL_END ("reset_assumptions");
}

void Solver::reset_constraint () {
  TRACE ("reset_constraint");
  REQUIRE_VALID_STATE ();
  transition_to_steady_state ();
  external->reset_constraint ();
  external->reset_concluded ();
  LOG_API_CALL_END ("reset_constraint");
}

/*------------------------------------------------------------------------*/

int Solver::propagate () {
  TRACE ("propagate_assumptions");
  REQUIRE_VALID_STATE ();
  transition_to_steady_state ();
  const int res = external->propagate_assumptions ();
  if (tracing_nb_lidrup_env_var_method)
    flush_proof_trace (true);
  LOG_API_CALL_RETURNS ("propagate_assumptions", res);
  if (res == 10)
    STATE (SATISFIED);
  else if (res == 20)
    STATE (UNSATISFIED);
  else
    STATE (INCONCLUSIVE);
  return res;
}

void Solver::implied (std::vector<int> &entrailed) {
  TRACE ("implied");
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == INCONCLUSIVE,
          "can only get implied literals only in unknown state");
  external->conclude_unknown ();
  external->implied (entrailed);
  if (tracing_nb_lidrup_env_var_method)
    flush_proof_trace (true);
  LOG_API_CALL_RETURNS ("implied", (int) entrailed.size ());
}

/*------------------------------------------------------------------------*/

int Solver::call_external_solve_and_check_results (bool preprocess_only) {
  transition_to_steady_state ();
  CADICAL_assert (state () & READY);
  STATE (SOLVING);
  const int res = external->solve (preprocess_only);
  if (res == 10)
    STATE (SATISFIED);
  else if (res == 20)
    STATE (UNSATISFIED);
  else
    STATE (INCONCLUSIVE);
#if 0 // EXPENSIVE ALTERNATIVE ASSUMPTION CHECKING
  // This checks that the set of failed assumptions form a core using the
  // external 'copy (...)' function to copy the solver, which can be trusted
  // less, since it involves copying the extension stack too.  The
  // 'External::check_assumptions_failing' is a better alternative and can
  // be enabled by options too.  We keep this code though to have an
  // alternative failed assumption checking available for debugging.
  //
  if (res == 20 && !external->assumptions.empty ()) {
    Solver checker;
    // checking restored clauses does not work (because the clauses are not added)
    checker.set("checkproof", 1);
    checker.set("lrat", 0);
    checker.prefix ("checker ");
    copy (checker);
    checker.set("log", 1);
    for (const auto & lit : external->assumptions)
      if (failed (lit))
        checker.add (lit), checker.add (0);
    if (checker.solve () != 20)
      FATAL ("copying assumption checker failed");
  }
#endif
  if (!res) {
    external->reset_assumptions ();
    external->reset_constraint ();
    external->reset_concluded ();
  }
  return res;
}

int Solver::solve () {
  TRACE ("solve");
  REQUIRE_READY_STATE ();
  const int res = call_external_solve_and_check_results (false);
  LOG_API_CALL_RETURNS ("solve", res);
  if (tracing_nb_lidrup_env_var_method)
    flush_proof_trace (true);
  return res;
}

int Solver::simplify (int rounds) {
  TRACE ("simplify", rounds);
  REQUIRE_READY_STATE ();
  REQUIRE (rounds >= 0, "negative number of simplification rounds '%d'",
           rounds);
  internal->limit ("preprocessing", rounds);
  const int res = call_external_solve_and_check_results (true);
  LOG_API_CALL_RETURNS ("simplify", rounds, res);
  return res;
}

/*------------------------------------------------------------------------*/

int Solver::val (int lit) {
  TRACE ("val", lit);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (lit);
  REQUIRE (state () == SATISFIED, "can only get value in satisfied state");
  if (!external->extended)
    external->extend ();
  external->conclude_sat ();
  int res = external->ival (lit);
  LOG_API_CALL_RETURNS ("val", lit, res);
  CADICAL_assert (state () == SATISFIED);
  CADICAL_assert (res == lit || res == -lit);
  return res;
}

bool Solver::flip (int lit) {
  TRACE ("flip", lit);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (lit);
  REQUIRE (state () == SATISFIED, "can only flip value in satisfied state");
  REQUIRE (!external->propagator,
           "can only flip when no external propagator is present");
  bool res = external->flip (lit);
  LOG_API_CALL_RETURNS ("flip", lit, res);
  CADICAL_assert (state () == SATISFIED);
  return res;
}

bool Solver::flippable (int lit) {
  TRACE ("flippable", lit);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (lit);
  REQUIRE (state () == SATISFIED, "can only flip value in satisfied state");
  REQUIRE (!external->propagator,
           "can only flip when no external propagator is present");
  bool res = external->flippable (lit);
  LOG_API_CALL_RETURNS ("flippable", lit, res);
  CADICAL_assert (state () == SATISFIED);
  return res;
}

bool Solver::failed (int lit) {
  TRACE ("failed", lit);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (lit);
  REQUIRE (state () == UNSATISFIED,
           "can only get failed assumptions in unsatisfied state");
  bool res = external->failed (lit);
  LOG_API_CALL_RETURNS ("failed", lit, res);
  CADICAL_assert (state () == UNSATISFIED);
  return res;
}

bool Solver::constraint_failed () {
  TRACE ("constraint_failed");
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == UNSATISFIED,
           "can only determine if constraint failed in unsatisfied state");
  bool res = external->failed_constraint ();
  LOG_API_CALL_RETURNS ("constraint_failed", res);
  CADICAL_assert (state () == UNSATISFIED);
  return res;
}

int Solver::fixed (int lit) const {
  TRACE ("fixed", lit);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (lit);
  int res = external->fixed (lit);
  LOG_API_CALL_RETURNS ("fixed", lit, res);
  return res;
}

void Solver::phase (int lit) {
  TRACE ("phase", lit);
  REQUIRE_VALID_OR_SOLVING_STATE ();
  REQUIRE_VALID_LIT (lit);
  external->phase (lit);
  LOG_API_CALL_END ("phase", lit);
}

void Solver::unphase (int lit) {
  TRACE ("unphase", lit);
  REQUIRE_VALID_OR_SOLVING_STATE ();
  REQUIRE_VALID_LIT (lit);
  external->unphase (lit);
  LOG_API_CALL_END ("unphase", lit);
}

/*------------------------------------------------------------------------*/

void Solver::terminate () {
  LOG_API_CALL_BEGIN ("terminate");
  REQUIRE_VALID_OR_SOLVING_STATE ();
  external->terminate ();
  LOG_API_CALL_END ("terminate");
}

void Solver::connect_terminator (Terminator *terminator) {
  LOG_API_CALL_BEGIN ("connect_terminator");
  REQUIRE_VALID_STATE ();
  REQUIRE (terminator, "can not connect zero terminator");
#ifdef LOGGING
  if (external->terminator)
    LOG ("connecting new terminator (disconnecting previous one)");
  else
    LOG ("connecting new terminator (no previous one)");
#endif
  external->terminator = terminator;
  LOG_API_CALL_END ("connect_terminator");
}

void Solver::disconnect_terminator () {
  LOG_API_CALL_BEGIN ("disconnect_terminator");
  REQUIRE_VALID_STATE ();
#ifdef LOGGING
  if (external->terminator)
    LOG ("disconnecting previous terminator");
  else
    LOG ("ignoring to disconnect terminator (no previous one)");
#endif
  external->terminator = 0;
  LOG_API_CALL_END ("disconnect_terminator");
}

/*------------------------------------------------------------------------*/

void Solver::connect_learner (Learner *learner) {
  LOG_API_CALL_BEGIN ("connect_learner");
  REQUIRE_VALID_STATE ();
  REQUIRE (learner, "can not connect zero learner");
#ifdef LOGGING
  if (external->learner)
    LOG ("connecting new learner (disconnecting previous one)");
  else
    LOG ("connecting new learner (no previous one)");
#endif
  external->learner = learner;
  LOG_API_CALL_END ("connect_learner");
}

void Solver::disconnect_learner () {
  LOG_API_CALL_BEGIN ("disconnect_learner");
  REQUIRE_VALID_STATE ();
#ifdef LOGGING
  if (external->learner)
    LOG ("disconnecting previous learner");
  else
    LOG ("ignoring to disconnect learner (no previous one)");
#endif
  external->learner = 0;
  LOG_API_CALL_END ("disconnect_learner");
}

/*===== IPASIR END =======================================================*/

void Solver::connect_fixed_listener (
    FixedAssignmentListener *fixed_listener) {
  LOG_API_CALL_BEGIN ("connect_fixed_listener");
  REQUIRE_VALID_STATE ();
  REQUIRE (fixed_listener, "can not connect zero fixed listener");

#ifdef LOGGING
  if (external->fixed_listener)
    LOG ("connecting new listener of fixed assignments (disconnecting "
         "previous one)");
  else
    LOG ("connecting new listener of fixed assigments (no previous one)");
#endif
  if (external->fixed_listener)
    disconnect_fixed_listener ();
  external->fixed_listener = fixed_listener;
  // Listeners are treated as real-time listeners, thus previously found
  // fixed assignments are not sent out (would be rather expensive to
  // recover it retrospect, see external_propagate.cpp/get_fixed_literals ()
  // function).
  LOG_API_CALL_END ("connect_fixed_listener");
}

void Solver::disconnect_fixed_listener () {
  LOG_API_CALL_BEGIN ("disconnect_fixed_listener");
  REQUIRE_VALID_STATE ();
#ifdef LOGGING
  if (external->fixed_listener)
    LOG ("disconnecting previous listener of fixed assignments");
  else
    LOG ("ignoring to disconnect listener of fixed assignments (no "
         "previous one)");
#endif
  external->fixed_listener = 0;
  LOG_API_CALL_END ("disconnect_fixed_listener");
}

/*===== IPASIR-UP BEGIN ==================================================*/

void Solver::connect_external_propagator (ExternalPropagator *propagator) {
  LOG_API_CALL_BEGIN ("connect_external_propagator");
  REQUIRE_VALID_STATE ();
  REQUIRE (propagator, "can not connect zero propagator");
#ifdef LOGGING
  if (external->propagator)
    LOG ("connecting new external propagator (disconnecting previous one)");
  else
    LOG ("connecting new external propagator (no previous one)");
#endif
  if (external->propagator)
    disconnect_external_propagator ();

  external->propagator = propagator;
  internal->connect_propagator ();
  internal->external_prop = true;
  internal->external_prop_is_lazy = propagator->is_lazy;
  LOG_API_CALL_END ("connect_external_propagator");
}

void Solver::disconnect_external_propagator () {
  LOG_API_CALL_BEGIN ("disconnect_external_propagator");
  REQUIRE_VALID_STATE ();

#ifdef LOGGING
  if (external->propagator)
    LOG ("disconnecting previous external propagator");
  else
    LOG ("ignoring to disconnect external propagator (no previous one)");
#endif
  if (external->propagator)
    external->reset_observed_vars ();

  external->propagator = 0;
  internal->set_tainted_literal ();
  internal->external_prop = false;
  internal->external_prop_is_lazy = true;
  LOG_API_CALL_END ("disconnect_external_propagator");
}

void Solver::add_observed_var (int idx) {
  TRACE ("observe", idx);
  REQUIRE_VALID_OR_SOLVING_STATE ();
  REQUIRE_VALID_LIT (idx);
  external->add_observed_var (idx);
  LOG_API_CALL_END ("observe", idx);
}

void Solver::remove_observed_var (int idx) {
  TRACE ("unobserve", idx);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (idx);
  external->remove_observed_var (idx);
  LOG_API_CALL_END ("unobserve", idx);
}

void Solver::reset_observed_vars () {
  TRACE ("reset_observed_vars");
  REQUIRE_VALID_OR_SOLVING_STATE ();
  external->reset_observed_vars ();
  LOG_API_CALL_END ("reset_observed_vars");
}

/*===== IPASIR-UP END ====================================================*/

int Solver::active () const {
  TRACE ("active");
  REQUIRE_VALID_STATE ();
  int res = internal->active ();
  LOG_API_CALL_RETURNS ("active", res);
  return res;
}

int64_t Solver::redundant () const {
  TRACE ("redundant");
  REQUIRE_VALID_STATE ();
  int64_t res = internal->redundant ();
  LOG_API_CALL_RETURNS ("redundant", res);
  return res;
}

int64_t Solver::irredundant () const {
  TRACE ("irredundant");
  REQUIRE_VALID_STATE ();
  int64_t res = internal->irredundant ();
  LOG_API_CALL_RETURNS ("irredundant", res);
  return res;
}

/*------------------------------------------------------------------------*/

void Solver::freeze (int lit) {
  TRACE ("freeze", lit);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (lit);
  external->freeze (lit);
  LOG_API_CALL_END ("freeze", lit);
}

void Solver::melt (int lit) {
  TRACE ("melt", lit);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (lit);
  REQUIRE (external->frozen (lit),
           "can not melt completely melted literal '%d'", lit);
  external->melt (lit);
  LOG_API_CALL_END ("melt", lit);
}

bool Solver::frozen (int lit) const {
  TRACE ("frozen", lit);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (lit);
  bool res = external->frozen (lit);
  LOG_API_CALL_RETURNS ("frozen", lit, res);
  return res;
}

/*------------------------------------------------------------------------*/

bool Solver::trace_proof (FILE *external_file, const char *name) {
  TRACE ("trace_proof", name);
  REQUIRE_VALID_STATE ();
  REQUIRE (
      state () == CONFIGURING,
      "can only start proof tracing to '%s' right after initialization",
      name);
  File *internal_file = File::write (internal, external_file, name);
  CADICAL_assert (internal_file);
  internal->trace (internal_file);
  LOG_API_CALL_RETURNS ("trace_proof", name, true);
  return true;
}

bool Solver::trace_proof (const char *path) {
  TRACE ("trace_proof", path);
  REQUIRE_VALID_STATE ();
  REQUIRE (
      state () == CONFIGURING,
      "can only start proof tracing to '%s' right after initialization",
      path);
  File *internal_file = File::write (internal, path);
  bool res = (internal_file != 0);
  internal->trace (internal_file);
  LOG_API_CALL_RETURNS ("trace_proof", path, res);
  return res;
}

void Solver::flush_proof_trace (bool print_statistics_unless_quiet) {
  TRACE ("flush_proof_trace");
  REQUIRE_VALID_STATE ();
  REQUIRE (!internal->file_tracers.empty (), "proof is not traced");
  REQUIRE (!internal->file_tracers.back ()->closed (),
           "proof trace already closed");
  internal->flush_trace (print_statistics_unless_quiet);
  LOG_API_CALL_END ("flush_proof_trace");
}

void Solver::close_proof_trace (bool print_statistics_unless_quiet) {
  TRACE ("close_proof_trace");
  REQUIRE_VALID_STATE ();
  REQUIRE (!internal->file_tracers.empty (), "proof is not traced");
  REQUIRE (!internal->file_tracers.back ()->closed (),
           "proof trace already closed");
  internal->close_trace (print_statistics_unless_quiet);
  LOG_API_CALL_END ("close_proof_trace");
}

/*------------------------------------------------------------------------*/

void Solver::connect_proof_tracer (Tracer *tracer, bool antecedents,
                                   bool finalize_clauses) {
  LOG_API_CALL_BEGIN ("connect proof tracer");
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
           "can only start proof tracing to right after initialization");
  REQUIRE (tracer, "can not connect zero tracer");
  internal->connect_proof_tracer (tracer, antecedents, finalize_clauses);
  LOG_API_CALL_END ("connect proof tracer");
}

void Solver::connect_proof_tracer (InternalTracer *tracer, bool antecedents,
                                   bool finalize_clauses) {
  LOG_API_CALL_BEGIN ("connect proof tracer");
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
           "can only start proof tracing to right after initialization");
  REQUIRE (tracer, "can not connect zero tracer");
  internal->connect_proof_tracer (tracer, antecedents, finalize_clauses);
  LOG_API_CALL_END ("connect proof tracer");
}

void Solver::connect_proof_tracer (StatTracer *tracer, bool antecedents,
                                   bool finalize_clauses) {
  LOG_API_CALL_BEGIN ("connect proof tracer with stats");
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
           "can only start proof tracing to right after initialization");
  REQUIRE (tracer, "can not connect zero tracer");
  internal->connect_proof_tracer (tracer, antecedents, finalize_clauses);
  LOG_API_CALL_END ("connect proof tracer with stats");
}

void Solver::connect_proof_tracer (FileTracer *tracer, bool antecedents,
                                   bool finalize_clauses) {
  LOG_API_CALL_BEGIN ("connect proof tracer with file");
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
           "can only start proof tracing right after initialization");
  REQUIRE (tracer, "can not connect zero tracer");
  internal->connect_proof_tracer (tracer, antecedents, finalize_clauses);
  LOG_API_CALL_END ("connect proof tracer with file");
}

bool Solver::disconnect_proof_tracer (Tracer *tracer) {
  LOG_API_CALL_BEGIN ("disconnect proof tracer");
  REQUIRE_VALID_STATE ();
  REQUIRE (tracer, "can not disconnect zero tracer");
  bool res = internal->disconnect_proof_tracer (tracer);
  LOG_API_CALL_RETURNS ("connect proof tracer", res);
  return res;
}

bool Solver::disconnect_proof_tracer (StatTracer *tracer) {
  LOG_API_CALL_BEGIN ("disconnect proof tracer");
  REQUIRE_VALID_STATE ();
  REQUIRE (tracer, "can not disconnect zero tracer");
  bool res = internal->disconnect_proof_tracer (tracer);
  LOG_API_CALL_RETURNS ("disconnect proof tracer", res);
  return res;
}

bool Solver::disconnect_proof_tracer (FileTracer *tracer) {
  LOG_API_CALL_BEGIN ("disconnect proof tracer");
  REQUIRE_VALID_STATE ();
  REQUIRE (tracer, "can not disconnect zero tracer");
  bool res = internal->disconnect_proof_tracer (tracer);
  LOG_API_CALL_RETURNS ("disconnect proof tracer", res);
  return res;
}

/*------------------------------------------------------------------------*/

void Solver::conclude () {
  TRACE ("conclude");
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == UNSATISFIED || state () == SATISFIED ||
               state () == INCONCLUSIVE,
           "can only conclude in satisfied, unsatisfied or inconclusive state");
  if (state () == UNSATISFIED)
    internal->conclude_unsat ();
  else if (state () == SATISFIED)
    external->conclude_sat ();
  else if (state () == INCONCLUSIVE)
    external->conclude_unknown ();
  CADICAL_assert (state () == UNSATISFIED || state () == SATISFIED ||
          state () == INCONCLUSIVE);
  LOG_API_CALL_END ("conclude");
}

/*------------------------------------------------------------------------*/

void Solver::build (FILE *file, const char *prefix) {

  CADICAL_assert (file == stdout || file == stderr);

  Terminal *terminal;

  if (file == stdout)
    terminal = &tout;
  else if (file == stderr)
    terminal = &terr;
  else
    terminal = 0;

  const char *v = CaDiCaL::version ();
  const char *i = identifier ();
  const char *c = compiler ();
  const char *b = date ();
  const char *f = flags ();

  CADICAL_assert (v);

  fputs (prefix, file);
  if (terminal)
    terminal->magenta ();
  fputs ("Version ", file);
  if (terminal)
    terminal->normal ();
  fputs (v, file);
  if (i) {
    if (terminal)
      terminal->magenta ();
    fputc (' ', file);
    fputs (i, file);
    if (terminal)
      terminal->normal ();
  }
  fputc ('\n', file);

  if (c) {
    fputs (prefix, file);
    if (terminal)
      terminal->magenta ();
    fputs (c, file);
    if (f) {
      fputc (' ', file);
      fputs (f, file);
    }
    if (terminal)
      terminal->normal ();
    fputc ('\n', file);
  }

  if (b) {
    fputs (prefix, file);
    if (terminal)
      terminal->magenta ();
    fputs (b, file);
    if (terminal)
      terminal->normal ();
    fputc ('\n', file);
  }

  fflush (file);
}

const char *Solver::version () { return CaDiCaL::version (); }

const char *Solver::signature () { return CaDiCaL::signature (); }

void Solver::options () {
  REQUIRE_VALID_STATE ();
  internal->opts.print ();
}

void Solver::usage () { Options::usage (); }

void Solver::configurations () { Config::usage (); }

void Solver::statistics () {
  if (state () == DELETING)
    return;
  TRACE ("stats");
  REQUIRE_VALID_OR_SOLVING_STATE ();
  internal->print_statistics ();
  LOG_API_CALL_END ("stats");
}

void Solver::resources () {
  if (state () == DELETING)
    return;
  TRACE ("resources");
  REQUIRE_VALID_OR_SOLVING_STATE ();
  internal->print_resource_usage ();
  LOG_API_CALL_END ("resources");
}

/*------------------------------------------------------------------------*/

const char *Solver::read_dimacs (File *file, int &vars, int strict,
                                 bool *incremental, vector<int> *cubes) {
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
           "can only read DIMACS file right after initialization");
  Parser *parser = new Parser (this, file, incremental, cubes);
  const char *err = parser->parse_dimacs (vars, strict);
  delete parser;
  return err;
}

const char *Solver::read_dimacs (FILE *external_file, const char *name,
                                 int &vars, int strict) {
  LOG_API_CALL_BEGIN ("read_dimacs", name);
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
           "can only read DIMACS file right after initialization");
  File *file = File::read (internal, external_file, name);
  CADICAL_assert (file);
  const char *err = read_dimacs (file, vars, strict);
  delete file;
  LOG_API_CALL_RETURNS ("read_dimacs", name, err);
  return err;
}

const char *Solver::read_dimacs (const char *path, int &vars, int strict) {
  LOG_API_CALL_BEGIN ("read_dimacs", path);
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
           "can only read DIMACS file right after initialization");
  File *file = File::read (internal, path);
  if (!file)
    return internal->error_message.init ("failed to read DIMACS file '%s'",
                                         path);
  const char *err = read_dimacs (file, vars, strict);
  delete file;
  LOG_API_CALL_RETURNS ("read_dimacs", path, err);
  return err;
}

const char *Solver::read_dimacs (FILE *external_file, const char *name,
                                 int &vars, int strict, bool &incremental,
                                 vector<int> &cubes) {
  LOG_API_CALL_BEGIN ("read_dimacs", name);
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
           "can only read DIMACS file right after initialization");
  File *file = File::read (internal, external_file, name);
  CADICAL_assert (file);
  const char *err = read_dimacs (file, vars, strict, &incremental, &cubes);
  delete file;
  LOG_API_CALL_RETURNS ("read_dimacs", name, err);
  return err;
}

const char *Solver::read_dimacs (const char *path, int &vars, int strict,
                                 bool &incremental, vector<int> &cubes) {
  LOG_API_CALL_BEGIN ("read_dimacs", path);
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
           "can only read DIMACS file right after initialization");
  File *file = File::read (internal, path);
  if (!file)
    return internal->error_message.init ("failed to read DIMACS file '%s'",
                                         path);
  const char *err = read_dimacs (file, vars, strict, &incremental, &cubes);
  delete file;
  LOG_API_CALL_RETURNS ("read_dimacs", path, err);
  return err;
}

const char *Solver::read_solution (const char *path) {
  LOG_API_CALL_BEGIN ("solution", path);
  REQUIRE_VALID_STATE ();
  File *file = File::read (internal, path);
  if (!file)
    return internal->error_message.init (
        "failed to read solution file '%s'", path);
  Parser *parser = new Parser (this, file, 0, 0);
  const char *err = parser->parse_solution ();
  delete parser;
  delete file;
  if (!err)
    external->check_assignment (&External::sol);
  LOG_API_CALL_RETURNS ("read_solution", path, err);
  return err;
}

/*------------------------------------------------------------------------*/

void Solver::dump_cnf () {
  TRACE ("dump");
  REQUIRE_INITIALIZED ();
  internal->dump ();
  LOG_API_CALL_END ("dump");
}

/*------------------------------------------------------------------------*/

ExternalPropagator *Solver::get_propagator () {
  return external->propagator;
}

bool Solver::observed (int lit) {
  TRACE ("observed", lit);
  REQUIRE_VALID_OR_SOLVING_STATE ();
  REQUIRE_VALID_LIT (lit);
  bool res = external->observed (lit);
  LOG_API_CALL_RETURNS ("observed", lit, res);
  return res;
}

bool Solver::is_witness (int lit) {
  TRACE ("is_witness", lit);
  REQUIRE_VALID_OR_SOLVING_STATE ();
  REQUIRE_VALID_LIT (lit);
  bool res = external->is_witness (lit);
  LOG_API_CALL_RETURNS ("is_witness", lit, res);
  return res;
}

bool Solver::is_decision (int lit) {
  TRACE ("is_decision", lit);
  REQUIRE_VALID_OR_SOLVING_STATE ();
  REQUIRE_VALID_LIT (lit);
  bool res = external->is_decision (lit);
  LOG_API_CALL_RETURNS ("is_decision", lit, res);
  return res;
}

void Solver::force_backtrack (size_t new_level) {
  TRACE ("force_backtrack", new_level);
  REQUIRE_VALID_OR_SOLVING_STATE ();
  external->force_backtrack (new_level);
}

/*------------------------------------------------------------------------*/

bool Solver::traverse_clauses (ClauseIterator &it) const {
  LOG_API_CALL_BEGIN ("traverse_clauses");
  REQUIRE_VALID_STATE ();
  bool res = external->traverse_all_frozen_units_as_clauses (it) &&
             internal->traverse_clauses (it) &&
             internal->traverse_constraint (it);
  LOG_API_CALL_RETURNS ("traverse_clauses", res);
  return res;
}

bool Solver::traverse_witnesses_backward (WitnessIterator &it) const {
  LOG_API_CALL_BEGIN ("traverse_witnesses_backward");
  REQUIRE_VALID_STATE ();
  bool res = external->traverse_all_non_frozen_units_as_witnesses (it) &&
             external->traverse_witnesses_backward (it);
  LOG_API_CALL_RETURNS ("traverse_witnesses_backward", res);
  return res;
}

bool Solver::traverse_witnesses_forward (WitnessIterator &it) const {
  LOG_API_CALL_BEGIN ("traverse_witnesses_forward");
  REQUIRE_VALID_STATE ();
  bool res = external->traverse_witnesses_forward (it) &&
             external->traverse_all_non_frozen_units_as_witnesses (it);
  LOG_API_CALL_RETURNS ("traverse_witnesses_forward", res);
  return res;
}

/*------------------------------------------------------------------------*/

class ClauseCounter : public ClauseIterator {
public:
  int vars;
  int64_t clauses;
  ClauseCounter () : vars (0), clauses (0) {}
  bool clause (const vector<int> &c) {
    for (const auto &lit : c) {
      CADICAL_assert (lit != INT_MIN);
      int idx = abs (lit);
      if (idx > vars)
        vars = idx;
    }
    clauses++;
    return true;
  }
};

class ClauseWriter : public ClauseIterator {
  File *file;

public:
  ClauseWriter (File *f) : file (f) {}
  bool clause (const vector<int> &c) {
    for (const auto &lit : c) {
      if (!file->put (lit))
        return false;
      if (!file->put (' '))
        return false;
    }
    return file->put ("0\n");
  }
};

const char *Solver::write_dimacs (const char *path, int min_max_var) {
  LOG_API_CALL_BEGIN ("write_dimacs", path, min_max_var);
  REQUIRE_VALID_STATE ();
#ifndef CADICAL_QUIET
  const double start = internal->time ();
#endif
  internal->restore_clauses ();
  ClauseCounter counter;
  (void) traverse_clauses (counter);
  LOG ("found maximal variable %d and %" PRId64 " clauses", counter.vars,
       counter.clauses);
  File *file = File::write (internal, path);
  const char *res = 0;
  if (file) {
    int actual_max_vars = max (min_max_var, counter.vars);
    MSG ("writing %s'p cnf %d %" PRId64 "'%s header", tout.green_code (),
         actual_max_vars, counter.clauses, tout.normal_code ());
    file->put ("p cnf ");
    file->put (actual_max_vars);
    file->put (' ');
    file->put (counter.clauses);
    file->put ('\n');
    ClauseWriter writer (file);
    if (!traverse_clauses (writer))
      res = internal->error_message.init (
          "writing to DIMACS file '%s' failed", path);
    delete file;
  } else
    res = internal->error_message.init (
        "failed to open DIMACS file '%s' for writing", path);
#ifndef CADICAL_QUIET
  if (!res) {
    const double end = internal->time ();
    MSG ("wrote %" PRId64 " clauses in %.2f seconds %s time",
         counter.clauses, end - start,
         internal->opts.realtime ? "real" : "process");
  }
#endif
  LOG_API_CALL_RETURNS ("write_dimacs", path, min_max_var, res);
  return res;
}

/*------------------------------------------------------------------------*/

struct WitnessWriter : public WitnessIterator {
  File *file;
  int64_t witnesses;
  WitnessWriter (File *f) : file (f), witnesses (0) {}
  bool write (const vector<int> &a) {
    for (const auto &lit : a) {
      if (!file->put (lit))
        return false;
      if (!file->put (' '))
        return false;
    }
    return file->put ('0');
  }
  bool witness (const vector<int> &c, const vector<int> &w, int64_t) {
    if (!write (c))
      return false;
    if (!file->put (' '))
      return false;
    if (!write (w))
      return false;
    if (!file->put ('\n'))
      return false;
    witnesses++;
    return true;
  }
};

const char *Solver::write_extension (const char *path) {
  LOG_API_CALL_BEGIN ("write_extension", path);
  REQUIRE_VALID_STATE ();
  const char *res = 0;
#ifndef CADICAL_QUIET
  const double start = internal->time ();
#endif
  File *file = File::write (internal, path);
  WitnessWriter writer (file);
  if (file) {
    if (!traverse_witnesses_backward (writer))
      res = internal->error_message.init (
          "writing to DIMACS file '%s' failed", path);
    delete file;
  } else
    res = internal->error_message.init (
        "failed to open extension file '%s' for writing", path);
#ifndef CADICAL_QUIET
  if (!res) {
    const double end = internal->time ();
    MSG ("wrote %" PRId64 " witnesses in %.2f seconds %s time",
         writer.witnesses, end - start,
         internal->opts.realtime ? "real" : "process");
  }
#endif
  LOG_API_CALL_RETURNS ("write_extension", path, res);
  return res;
}

/*------------------------------------------------------------------------*/

struct ClauseCopier : public ClauseIterator {
  Solver &dst;

public:
  ClauseCopier (Solver &d) : dst (d) {}
  bool clause (const vector<int> &c) {
    for (const auto &lit : c)
      dst.add (lit);
    dst.add (0);
    return true;
  }
};

struct WitnessCopier : public WitnessIterator {
  External *dst;

public:
  WitnessCopier (External *d) : dst (d) {}
  bool witness (const vector<int> &c, const vector<int> &w, int64_t id) {
    dst->push_external_clause_and_witness_on_extension_stack (c, w, id);
    return true;
  }
};

void Solver::copy (Solver &other) const {
  REQUIRE_READY_STATE ();
  REQUIRE (other.state () & CONFIGURING, "target solver already modified");
  internal->opts.copy (other.internal->opts);
  ClauseCopier clause_copier (other);
  traverse_clauses (clause_copier);
  WitnessCopier witness_copier (other.external);
  traverse_witnesses_forward (witness_copier);
  external->copy_flags (*other.external);
}

/*------------------------------------------------------------------------*/

void Solver::section (const char *title) {
  if (state () == DELETING)
    return;
#ifdef CADICAL_QUIET
  (void) title;
#endif
  REQUIRE_INITIALIZED ();
  SECTION (title);
}

void Solver::message (const char *fmt, ...) {
  if (state () == DELETING)
    return;
#ifdef CADICAL_QUIET
  (void) fmt;
#else
  REQUIRE_INITIALIZED ();
  va_list ap;
  va_start (ap, fmt);
  internal->vmessage (fmt, ap);
  va_end (ap);
#endif
}

void Solver::message () {
  if (state () == DELETING)
    return;
  REQUIRE_INITIALIZED ();
#ifndef CADICAL_QUIET
  internal->message ();
#endif
}

void Solver::verbose (int level, const char *fmt, ...) {
  if (state () == DELETING)
    return;
  REQUIRE_VALID_OR_SOLVING_STATE ();
#ifdef CADICAL_QUIET
  (void) level;
  (void) fmt;
#else
  va_list ap;
  va_start (ap, fmt);
  internal->vverbose (level, fmt, ap);
  va_end (ap);
#endif
}

void Solver::error (const char *fmt, ...) {
  if (state () == DELETING)
    return;
  REQUIRE_INITIALIZED ();
  va_list ap;
  va_start (ap, fmt);
  internal->verror (fmt, ap);
  va_end (ap);
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
