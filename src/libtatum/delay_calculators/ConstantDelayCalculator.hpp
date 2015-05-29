#include "Time.hpp"
#include "DelayCalculator.hpp"

class ConstantDelayCalculator : public DelayCalculator {
    public:
        ConstantDelayCalculator(Time value = Time(1.)): value_(value_) {}
        Time min_edge_delay(const TimingGraph& tg, EdgeId edge_id) const { return value_; };
        Time max_edge_delay(const TimingGraph& tg, EdgeId edge_id) const { return value_; };

    private:
        Time value_:
}
