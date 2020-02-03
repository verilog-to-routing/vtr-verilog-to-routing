#include "odin_memory.hpp"

namespace odin {
uintptr_t min_address = std::numeric_limits<uintptr_t>::max();
uintptr_t max_address = std::numeric_limits<uintptr_t>::min();

char* strdup(const char* in) {
    size_t len = (in) ? strlen(in) : 0;
    char* to_return = (char*)calloc(len + 1, sizeof(char));

    if (to_return) {
        if (in) {
            memcpy(to_return, in, len);
        }

        to_return[len] = '\0';
    }

    return to_return;
}
} // namespace odin
