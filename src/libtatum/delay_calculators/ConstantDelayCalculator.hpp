#include "DelayCalculator.hpp"
#include "Time.hpp"

class ConstantDelayCalculator : public DelayCalculator {
    public:
        ConstantDelayCalculator(Time delay): delay_(delay) {}
        Time min_edge_delay(const TimingGraph& tg, EdgeId edge_id) const { return delay_; }
        Time max_edge_delay(const TimingGraph& tg, EdgeId edge_id) const { return delay_; }

    private:
        Time delay_;
};
