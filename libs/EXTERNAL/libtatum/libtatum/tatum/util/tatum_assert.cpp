#include "tatum_assert.hpp"

#include <cstdio>  //fprintf, stderr
#include <cstdlib> //abort

namespace tatum {
namespace assert {

void handle_assert(const char* expr, const char* file, unsigned int line, const char* function, const char* msg) {
    fprintf(stderr, "%s:%d", file, line);
    if (function) {
        fprintf(stderr, " %s:", function);
    }
    fprintf(stderr, " Assertion '%s' failed", expr);
    if (msg) {
        fprintf(stderr, " (%s)", msg);
    }
    fprintf(stderr, ".\n");
    std::abort();
}

} // namespace assert
} // namespace tatum
