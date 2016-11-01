#include "DelayCalculator.hpp"
#include "Time.hpp"

namespace tatum {

/**
 * An example implementation of a DelayCalculator, which always 
 * returns a (run-time specified) constant delay.
 *
 * \see DelayCalculator
 */
class ConstantDelayCalculator {
    public:
        ///\param delay The constant delay which will be returned for every edge.
        ConstantDelayCalculator(Time delay): delay_(delay) {}

        Time min_edge_delay(const TimingGraph& /*tg*/, EdgeId /*edge_id*/) const { return delay_; }
        Time max_edge_delay(const TimingGraph& /*tg*/, EdgeId /*edge_id*/) const { return delay_; }

    private:
        Time delay_;
};

} //namepsace
