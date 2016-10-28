#ifndef VTR_ASSERT_H
#define VTR_ASSERT_H
#include <cstdio> //fprintf, stderr
#include <cstdlib> //abort
/*
 * The header defines useful assertion macros for VTR projects.
 *
 * Three types of assertions are defined:
 *      VTR_ASSERT_OPT  - low overhead assertions that should always be enabled
 *      VTR_ASSERT      - medium overhead assertions that may be enabled
 *      VTR_ASSERT_SAFE - high overhead assertions typically enabled only for debugging
 * Each of the above assertions also have a *_MSG variants (e.g. VTR_ASSERT_MSG(expr, msg) 
 * which takes an additional argument specifying additional message text to be shown.
 * By convention the message should state the condition *being checked* (and not the failure condition),
 * since that the condition failed is obvious from the assertion failure itself.
 *
 * The macro VTR_ASSERT_LEVEL specifies the level of assertion checking desired:
 *
 *      VTR_ASSERT_LEVEL == 3: VTR_ASSERT_OPT, VTR_ASSERT, VTR_ASSERT_SAFE enabled
 *      VTR_ASSERT_LEVEL == 2: VTR_ASSERT_OPT, VTR_ASSERT enabled
 *      VTR_ASSERT_LEVEL == 1: VTR_ASSERT_OPT enabled
 *      VTR_ASSERT_LEVEL == 0: No assertion checking enabled
 * Note that an assertion levels beyond 3 are currently treated the same as level 3 
 */

//Set a default assertion level if none is specified
#ifndef VTR_ASSERT_LEVEL
# define VTR_ASSERT_LEVEL 2
#endif

//Enable the assertions based on the specified level
#if VTR_ASSERT_LEVEL >= 3
# define VTR_ASSERT_SAFE_ENABLED
#endif

#if VTR_ASSERT_LEVEL >= 2
#define VTR_ASSERT_ENABLED
#endif

#if VTR_ASSERT_LEVEL >= 1
# define VTR_ASSERT_OPT_ENABLED
#endif

//Define the user assertion macros
#ifdef VTR_ASSERT_SAFE_ENABLED
# define VTR_ASSERT_SAFE(expr) VTR_ASSERT_IMPL(expr, nullptr)
# define VTR_ASSERT_SAFE_MSG(expr, msg) VTR_ASSERT_IMPL(expr, msg)
#else
# define VTR_ASSERT_SAFE(expr) static_cast<void>(0)
# define VTR_ASSERT_SAFE_MSG(expr, msg) static_cast<void>(0)
#endif

#ifdef VTR_ASSERT_ENABLED
# define VTR_ASSERT(expr) VTR_ASSERT_IMPL(expr, nullptr)
# define VTR_ASSERT_MSG(expr, msg) VTR_ASSERT_IMPL(expr, msg)
#else
# define VTR_ASSERT(expr) static_cast<void>(0)
# define VTR_ASSERT_MSG(expr, msg) static_cast<void>(0)
#endif

#ifdef VTR_ASSERT_OPT_ENABLED
# define VTR_ASSERT_OPT(expr) VTR_ASSERT_IMPL(expr, nullptr)
# define VTR_ASSERT_OPT_MSG(expr, msg) VTR_ASSERT_IMPL(expr, msg)
#else
# define VTR_ASSERT_OPT(expr) static_cast<void>(0)
# define VTR_ASSERT_OPT_MSG(expr, msg) static_cast<void>(0)
#endif


//Define the assertion implementation macro
// We wrap the check in a do {} while() to ensure the function-like
// macro can be always be followed by a ';'
#define VTR_ASSERT_IMPL(expr, msg) do {\
        if(!(expr)) { \
            vtr::Assert::handle_assert(#expr, __FILE__, __LINE__, VTR_ASSERT_FUNCTION, msg); \
        } \
    } while(false)

//Figure out what macro to use to get the name of the current function
// We default to __func__ which is defined in C99
//
// g++ > 2.6 define __PRETTY_FUNC__ which includes class/namespace/overload
// information, so we prefer to use it if possible
#define VTR_ASSERT_FUNCTION __func__
#ifdef __GNUC__
# ifdef __GNUC_MINOR__
#  if __GNUC__ >= 2 && __GNUC_MINOR__ > 6
#   undef VTR_ASSERT_FUNCTION
#   define VTR_ASSERT_FUNCTION __PRETTY_FUNCTION__
#  endif
# endif
#endif

namespace vtr {
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
} //namespace

#endif //VTR_ASSERT_H
