#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

struct NameVal {
  const char *name;
  int val;
};

/*------------------------------------------------------------------------*/

// These are dummy configurations, which require additional code.

static NameVal default_config[1]; // With '-pedantic' just '[]' or
static NameVal plain_config[1];   // '[0]' gave a warning.

/*------------------------------------------------------------------------*/

// Here we have the pre-defined default configurations.

static NameVal sat_config[] = {
    {"elimeffort", 10},
    {"stabilizeonly", 1},
    {"subsumeeffort", 60},
};

static NameVal unsat_config[] = {
    {"stabilize", 0},
    {"walk", 0},
};

/*------------------------------------------------------------------------*/

#define CONFIGS \
\
  CONFIG (default, "set default advanced internal options") \
  CONFIG (plain, "disable all internal preprocessing options") \
  CONFIG (sat, "set internal options to target satisfiable instances") \
  CONFIG (unsat, "set internal options to target unsatisfiable instances")

static const char *configs[] = {
#define CONFIG(N, D) #N,
    CONFIGS
#undef CONFIG
};

static size_t num_configs = sizeof configs / sizeof *configs;

/*------------------------------------------------------------------------*/

bool Config::has (const char *name) {
#define CONFIG(N, D) \
  if (!strcmp (name, #N)) \
    return true;
  CONFIGS
#undef CONFIG
  return false;
}

bool Config::set (Options &opts, const char *name) {
  if (!strcmp (name, "default")) {
    opts.reset_default_values ();
    return true;
  }
  if (!strcmp (name, "plain")) {
    opts.disable_preprocessing ();
    return true;
  }
#define CONFIG(N, D) \
  do { \
    if (strcmp (name, #N)) \
      break; \
    const NameVal *BEGIN = N##_config; \
    const NameVal *END = BEGIN + sizeof N##_config / sizeof (NameVal); \
    for (const NameVal *P = BEGIN; P != END; P++) { \
      CADICAL_assert (Options::has (P->name)); \
      opts.set (P->name, P->val); \
    } \
    return true; \
  } while (0);
  CONFIGS
#undef CONFIG
  return false;
}

/*------------------------------------------------------------------------*/

void Config::usage () {
#define CONFIG(N, D) printf ("  %-14s " D "\n", "--" #N);
  CONFIGS
#undef CONFIG
}

/*------------------------------------------------------------------------*/

const char **Config::begin () { return configs; }
const char **Config::end () { return &configs[num_configs]; }

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
