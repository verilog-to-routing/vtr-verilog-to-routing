#pragma once

#include "tatum/util/tatum_linear_map.hpp"
#include "tatum/Time.hpp"
#include "tatum/TimingGraph.hpp"
#include "tatum/delay_calc/DelayCalculator.hpp"

namespace tatum {

/** 
 * An exmaple DelayCalculator implementation which takes 
 * a vector of fixed pre-calculated edge delays
 *
 * \see DelayCalculator
 */
class FixedDelayCalculator : public DelayCalculator {
    public:

        ///Initializes the edge delays
        ///\param max_edge_delays A container specifying the maximum delay for every combinational edge
        ///\param setup_times A container specifying the setup times delay for every sequential capture edge
        FixedDelayCalculator(const tatum::util::linear_map<EdgeId,Time>& max_edge_delays, 
                             const tatum::util::linear_map<EdgeId,Time>& setup_times)
            : max_edge_delays_(max_edge_delays)
            , setup_times_(setup_times)
            , min_edge_delays_(max_edge_delays)
            , hold_times_(setup_times) { }

        ///Initializes the edge delays
        ///\param max_edge_delays A container specifying the maximum delay for every combinational edge
        ///\param setup_times A container specifying the setup times delay for every sequential edge
        ///\param min_edge_delays A container specifying the minimum delay for every combinational edge
        ///\param hold_times A container specifying the hold times delay for every sequential capture edge
        FixedDelayCalculator(const tatum::util::linear_map<EdgeId,Time>& max_edge_delays, 
                             const tatum::util::linear_map<EdgeId,Time>& setup_times,
                             const tatum::util::linear_map<EdgeId,Time>& min_edge_delays, 
                             const tatum::util::linear_map<EdgeId,Time>& hold_times)
            : max_edge_delays_(max_edge_delays)
            , setup_times_(setup_times)
            , min_edge_delays_(min_edge_delays)
            , hold_times_(hold_times) { }
            


        Time max_edge_delay(const TimingGraph& /*tg*/, EdgeId edge_id) const override { return max_edge_delays_[edge_id]; }

        Time min_edge_delay(const TimingGraph& /*tg*/, EdgeId edge_id) const override { return min_edge_delays_[edge_id]; }

        Time setup_time(const TimingGraph& /*tg*/, EdgeId edge_id) const override { return setup_times_[edge_id]; }

        Time hold_time(const TimingGraph& /*tg*/, EdgeId edge_id) const override { return hold_times_[edge_id]; }
 
    private:
        tatum::util::linear_map<EdgeId,Time> max_edge_delays_;
        tatum::util::linear_map<EdgeId,Time> setup_times_;

        tatum::util::linear_map<EdgeId,Time> min_edge_delays_;
        tatum::util::linear_map<EdgeId,Time> hold_times_;
};

} //namepsace
