#ifndef TATUM_ASSERT_H
#define TATUM_ASSERT_H
#include <cstdio> //fprintf, stderr
#include <cstdlib> //abort
/*
 * The header defines useful assertion macros for TATUM projects.
 *
 * Three types of assertions are defined:
 *      TATUM_ASSERT_OPT  - low overhead assertions that should always be enabled
 *      TATUM_ASSERT      - medium overhead assertions that may be enabled
 *      TATUM_ASSERT_SAFE - high overhead assertions typically enabled only for debugging
 * Each of the above assertions also have a *_MSG variants (e.g. TATUM_ASSERT_MSG(expr, msg))
 * which takes an additional argument specifying additional message text to be shown when 
 * the assertion fails.
 *
 * The macro TATUM_ASSERT_LEVEL specifies the level of assertion checking desired:
 *
 *      TATUM_ASSERT_LEVEL == 3: TATUM_ASSERT_OPT, TATUM_ASSERT, TATUM_ASSERT_SAFE enabled
 *      TATUM_ASSERT_LEVEL == 2: TATUM_ASSERT_OPT, TATUM_ASSERT enabled
 *      TATUM_ASSERT_LEVEL == 1: TATUM_ASSERT_OPT enabled
 *      TATUM_ASSERT_LEVEL == 0: No assertion checking enabled
 * Note that an assertion levels beyond 3 are currently treated the same as level 3 
 */

//Set a default assertion level if none is specified
#ifndef TATUM_ASSERT_LEVEL
# define TATUM_ASSERT_LEVEL 2
#endif

//Enable the assertions based on the specified level
#if TATUM_ASSERT_LEVEL >= 3
# define TATUM_ASSERT_SAFE_ENABLED
#endif

#if TATUM_ASSERT_LEVEL >= 2
#define TATUM_ASSERT_ENABLED
#endif

#if TATUM_ASSERT_LEVEL >= 1
# define TATUM_ASSERT_OPT_ENABLED
#endif

//Define the user assertion macros
#ifdef TATUM_ASSERT_SAFE_ENABLED
# define TATUM_ASSERT_SAFE(expr) TATUM_ASSERT_IMPL(expr, nullptr)
# define TATUM_ASSERT_SAFE_MSG(expr, msg) TATUM_ASSERT_IMPL(expr, msg)
#else
# define TATUM_ASSERT_SAFE(expr) static_cast<void>(0)
# define TATUM_ASSERT_SAFE_MSG(expr, msg) static_cast<void>(0)
#endif

#ifdef TATUM_ASSERT_ENABLED
# define TATUM_ASSERT(expr) TATUM_ASSERT_IMPL(expr, nullptr)
# define TATUM_ASSERT_MSG(expr, msg) TATUM_ASSERT_IMPL(expr, msg)
#else
# define TATUM_ASSERT(expr) static_cast<void>(0)
# define TATUM_ASSERT_MSG(expr, msg) static_cast<void>(0)
#endif

#ifdef TATUM_ASSERT_OPT_ENABLED
# define TATUM_ASSERT_OPT(expr) TATUM_ASSERT_IMPL(expr, nullptr)
# define TATUM_ASSERT_OPT_MSG(expr, msg) TATUM_ASSERT_IMPL(expr, msg)
#else
# define TATUM_ASSERT_OPT(expr) static_cast<void>(0)
# define TATUM_ASSERT_OPT_MSG(expr, msg) static_cast<void>(0)
#endif


//Define the assertion implementation macro
// We wrap the check in a do {} while() to ensure the function-like
// macro can be always be followed by a ';'
#define TATUM_ASSERT_IMPL(expr, msg) do {\
        if(!(expr)) { \
            tatum::util::Assert::handle_assert(#expr, __FILE__, __LINE__, TATUM_ASSERT_FUNCTION, msg); \
        } \
    } while(false)

//Figure out what macro to use to get the name of the current function
// We default to __func__ which is defined in C99
//
// g++ > 2.6 define __PRETTY_FUNC__ which includes class/namespace/overload
// information, so we prefer to use it if possible
#define TATUM_ASSERT_FUNCTION __func__
#ifdef __GNUC__
# ifdef __GNUC_MINOR__
#  if __GNUC__ >= 2 && __GNUC_MINOR__ > 6
#   undef TATUM_ASSERT_FUNCTION
#   define TATUM_ASSERT_FUNCTION __PRETTY_FUNCTION__
#  endif
# endif
#endif

namespace tatum { namespace util {
    class Assert {
        
        public:
            static void handle_assert(const char* expr, const char* file, unsigned int line, const char* function, const char* msg) {
                fprintf(stderr, "%s:%d", file, line);
                if(function) {
                    fprintf(stderr, " %s:", function);
                }
                fprintf(stderr, " Assertion '%s' failed", expr);
                if(msg) {
                    fprintf(stderr, " (%s)", msg);
                }
                fprintf(stderr, ".\n");
                std::abort();
            }
    };
}} //namespace

#endif //TATUM_ASSERT_H
