#ifndef _contract_hpp_INCLUDED
#define _contract_hpp_INCLUDED

#include "global.h"

ABC_NAMESPACE_CXX_HEADER_START

/*------------------------------------------------------------------------*/
#ifndef CADICAL_NCONTRACTS
/*------------------------------------------------------------------------*/

// If the user violates API contracts while calling functions declared in
// 'cadical.hpp' and implemented in 'solver.cpp' then an error is reported.
// Currently we also force aborting the program.  In the future it might be
// better to allow the user to provide a call back function, which then can
// for instance throw a C++ exception or execute a 'longjmp' in 'C' etc.

#define CONTRACT_VIOLATED(...) \
  do { \
    fatal_message_start (); \
    fprintf (stderr, \
             "invalid API usage of '%s' in '%s': ", __PRETTY_FUNCTION__, \
             __FILE__); \
    fprintf (stderr, __VA_ARGS__); \
    fputc ('\n', stderr); \
    fflush (stderr); \
    abort (); \
  } while (0)

/*------------------------------------------------------------------------*/

namespace CaDiCaL {

// It would be much easier to just write 'REQUIRE (this, "not initialized")'
// which however produces warnings due to the '-Wnonnull' check. Note, that
// 'this' is always assumed to be non zero in modern C++.  Much worse, if we
// use instead 'this != 0' or something similar like 'this != nullptr' then
// optimization silently removes this check ('gcc-7.4.0' at least) even
// though of course a zero pointer might be used as 'this' if the user did
// not initialize it.  The only solution I found is to disable optimization
// for this check. It does not seem to be necessary for 'clang++' though
// ('clang++-6.0.0' at least).  The alternative is to not check that the
// user forgot to initialize the solver pointer, but as long this works we
// keep this ugly hack. It also forces the function not to be inlined.
// The actual code I is in 'contract.cpp'.
//
void require_solver_pointer_to_be_non_zero (const void *ptr,
                                            const char *function_name,
                                            const char *file_name);
#define REQUIRE_NON_ZERO_THIS() \
  do { \
    require_solver_pointer_to_be_non_zero (this, __PRETTY_FUNCTION__, \
                                           __FILE__); \
  } while (0)

} // namespace CaDiCaL

/*------------------------------------------------------------------------*/

// These are common shortcuts for 'Solver' API contracts (requirements).

#define REQUIRE(COND, ...) \
  do { \
    if ((COND)) \
      break; \
    CONTRACT_VIOLATED (__VA_ARGS__); \
  } while (0)

#define REQUIRE_INITIALIZED() \
  do { \
    REQUIRE_NON_ZERO_THIS (); \
    REQUIRE (external, "external solver not initialized"); \
    REQUIRE (internal, "internal solver not initialized"); \
  } while (0)

#define REQUIRE_VALID_STATE() \
  do { \
    REQUIRE_INITIALIZED (); \
    REQUIRE (this->state () & VALID, "solver in invalid state"); \
  } while (0)

#define REQUIRE_READY_STATE() \
  do { \
    REQUIRE_VALID_STATE (); \
    REQUIRE (state () != ADDING, \
             "clause incomplete (terminating zero not added)"); \
  } while (0)

#define REQUIRE_VALID_OR_SOLVING_STATE() \
  do { \
    REQUIRE_INITIALIZED (); \
    REQUIRE (this->state () & (VALID | SOLVING), \
             "solver neither in valid nor solving state"); \
  } while (0)

#define REQUIRE_VALID_LIT(LIT) \
  do { \
    REQUIRE ((int) (LIT) && ((int) (LIT)) != INT_MIN, \
             "invalid literal '%d'", (int) (LIT)); \
    REQUIRE (external->is_valid_input ((int) (LIT)), \
             "extension variable %d defined by the solver", (int) (LIT)); \
  } while (0)

#define REQUIRE_STEADY_STATE() \
  do { \
    REQUIRE_INITIALIZED (); \
    REQUIRE (this->state () & STEADY, "solver is not in steady state"); \
  } while (0)

/*------------------------------------------------------------------------*/
#else // CADICAL_NCONTRACTS
/*------------------------------------------------------------------------*/

#define REQUIRE(...) \
  do { \
  } while (0)
#define REQUIRE_INITIALIZED() \
  do { \
  } while (0)
#define REQUIRE_VALID_STATE() \
  do { \
  } while (0)
#define REQUIRE_READY_STATE() \
  do { \
  } while (0)
#define REQUIRE_VALID_OR_SOLVING_STATE() \
  do { \
  } while (0)
#define REQUIRE_VALID_LIT(...) \
  do { \
  } while (0)
#define REQUIRE_STEADY_STATE() \
  do { \
  } while (0)

/*------------------------------------------------------------------------*/
#endif
/*------------------------------------------------------------------------*/

ABC_NAMESPACE_CXX_HEADER_END

#endif
