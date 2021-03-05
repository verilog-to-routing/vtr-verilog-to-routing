// Put this above guard so that TRUE/FALSE are undef'ed
// even if this file was already included earlier.
#ifndef VTR_LOGIC_H
#define VTR_LOGIC_H

#ifdef FALSE
#    undef FALSE
#endif
#define FALSE FALSE

#ifdef TRUE
#    undef TRUE
#endif
#define TRUE TRUE

constexpr int FALSE = 0;
constexpr int TRUE = 1;

namespace vtr {

/**
 * @brief This class represents the different supported logic values
 */
enum class LogicValue {
    FALSE = 0,
    TRUE = 1,
    DONT_CARE = 2,
    UNKOWN = 3
};

} // namespace vtr

#endif
