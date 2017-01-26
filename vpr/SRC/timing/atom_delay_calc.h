#ifndef VPR_ATOM_DELAY_CALC_H
#define VPR_ATOM_DELAY_CALC_H

#include "atom_netlist.h"
#include "atom_map.h"
#include "vtr_hash.h"

//Delay Calculator for timing edges which are internal to atoms
class AtomDelayCalc {
    public:
        AtomDelayCalc(const AtomNetlist& netlist, const AtomMap& netlist_map);
        float atom_combinational_delay(const AtomPinId src_pin, const AtomPinId sink_pin) const;
        float atom_setup_time(const AtomPinId clk_pin, const AtomPinId input_pin) const;
        float atom_clock_to_q_delay(const AtomPinId clk_pin, const AtomPinId output_pin) const;
    private:
        const t_pb_graph_pin* find_pb_graph_pin(const AtomPinId atom_pin) const;
    private:
        const AtomNetlist& netlist_;
        const AtomMap& netlist_map_;
};

//A version of AtomDelayCalc which caches edge delays for improved performance.
//
// If an edge delay changes it must be invalidated before the new delay value is returned.
class CachingAtomDelayCalc {
    public:
        CachingAtomDelayCalc(const AtomNetlist& netlist, const AtomMap& netlist_map);
        float atom_combinational_delay(const AtomPinId src_pin, const AtomPinId sink_pin) const;
        float atom_setup_time(const AtomPinId clk_pin, const AtomPinId input_pin) const;
        float atom_clock_to_q_delay(const AtomPinId clk_pin, const AtomPinId output_pin) const;

        void invalidate_delay(const AtomPinId src_pin, const AtomPinId sink_pin);
        void invalidate_all_delays();
    private:
        struct KeyHasher {
            size_t operator()(const std::tuple<AtomPinId,AtomPinId> key) const {
                size_t hash = 0;
                vtr::hash_combine(hash, std::get<0>(key));
                vtr::hash_combine(hash, std::get<1>(key));
                return hash;
            }
        };
        using Cache = std::unordered_map<std::tuple<AtomPinId,AtomPinId>,float,KeyHasher>;
    private:
        AtomDelayCalc raw_delay_calc_;

        mutable Cache delay_cache_;

};

#include "atom_delay_calc.inl"

#endif
