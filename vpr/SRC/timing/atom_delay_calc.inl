#include <cmath>
#include "atom_delay_calc.h"
/*
 * AtomDelayCalc
 */
inline AtomDelayCalc::AtomDelayCalc(const AtomNetlist& netlist, const AtomMap& netlist_map)
    : netlist_(netlist)
    , netlist_map_(netlist_map) {}

inline float AtomDelayCalc::atom_combinational_delay(const AtomPinId src_pin, const AtomPinId sink_pin) const {
    VTR_ASSERT_MSG(netlist_.pin_block(src_pin) == netlist_.pin_block(sink_pin), "Combinational primitive delay must be between pins on the same block");

    VTR_ASSERT_MSG(   netlist_.port_type(netlist_.pin_port(src_pin)) == AtomPortType::INPUT 
                   && netlist_.port_type(netlist_.pin_port(sink_pin)) == AtomPortType::OUTPUT,
                   "Combinational connections must go from primitive input to output");

    //Determine the combinational delay from the pb_graph_pin.
    //  By convention the delay is annotated on the corresponding input pg_graph_pin
    const t_pb_graph_pin* src_gpin = find_pb_graph_pin(src_pin);
    const t_pb_graph_pin* sink_gpin = find_pb_graph_pin(sink_pin);
    VTR_ASSERT(src_gpin->num_pin_timing > 0);

    //Find the annotation on the source that matches the sink
    float delay = NAN;
    for(int i = 0; i < src_gpin->num_pin_timing; ++i) {
        const t_pb_graph_pin* timing_sink_gpin = src_gpin->pin_timing[i]; 

        if(timing_sink_gpin == sink_gpin) {
            delay = src_gpin->pin_timing_del_max[i];
            break;
        }
    }

    VTR_ASSERT_MSG(!std::isnan(delay), "Must have a valid delay");

    return delay;
}

inline float AtomDelayCalc::atom_setup_time(const AtomPinId /*clock_pin*/, const AtomPinId input_pin) const {
    VTR_ASSERT(netlist_.port_type(netlist_.pin_port(input_pin)) == AtomPortType::INPUT);

    const t_pb_graph_pin* gpin = find_pb_graph_pin(input_pin);
    VTR_ASSERT(gpin->type == PB_PIN_SEQUENTIAL);

    return gpin->tsu_tco;
}

inline float AtomDelayCalc::atom_clock_to_q_delay(const AtomPinId /*clock_pin*/, const AtomPinId output_pin) const {
    VTR_ASSERT(netlist_.port_type(netlist_.pin_port(output_pin)) == AtomPortType::OUTPUT);

    const t_pb_graph_pin* gpin = find_pb_graph_pin(output_pin);
    VTR_ASSERT(gpin->type == PB_PIN_SEQUENTIAL);

    return gpin->tsu_tco;
}

inline const t_pb_graph_pin* AtomDelayCalc::find_pb_graph_pin(const AtomPinId atom_pin) const {
    const t_pb_graph_pin* gpin = netlist_map_.atom_pin_pb_graph_pin(atom_pin);
    VTR_ASSERT(gpin != nullptr);

    return gpin;
}

/*
 * CachingAtomDelayCalc
 */
inline CachingAtomDelayCalc::CachingAtomDelayCalc(const AtomNetlist& netlist, const AtomMap& netlist_map)
    : raw_delay_calc_(netlist, netlist_map) {}

inline void CachingAtomDelayCalc::invalidate_delay(const AtomPinId src_pin, const AtomPinId sink_pin) {
    auto key = std::make_tuple(src_pin,sink_pin);
    delay_cache_.erase(key);
}

inline void CachingAtomDelayCalc::invalidate_all_delays() {
    delay_cache_.clear();
}

inline float CachingAtomDelayCalc::atom_combinational_delay(const AtomPinId src_pin, const AtomPinId sink_pin) const {
    //Look-up
    auto key = std::make_tuple(src_pin,sink_pin);
    auto iter = delay_cache_.find(key);

    if(iter != delay_cache_.end()) {
        //Hit
        return iter->second;
    }

    //Miss

    //Calculate
    float comb_delay = raw_delay_calc_.atom_combinational_delay(src_pin, sink_pin);

    //Save in cache
    delay_cache_[key] = comb_delay;

    return comb_delay;
}

inline float CachingAtomDelayCalc::atom_setup_time(const AtomPinId clock_pin, const AtomPinId input_pin) const {
    //Look-up
    auto key = std::make_tuple(clock_pin,input_pin);
    auto iter = delay_cache_.find(key);

    if(iter != delay_cache_.end()) {
        //Hit
        return iter->second;
    }

    //Miss

    //Calculate
    float tsu = raw_delay_calc_.atom_setup_time(clock_pin, input_pin);

    //Save in cache
    delay_cache_[key] = tsu;

    return tsu;
}

inline float CachingAtomDelayCalc::atom_clock_to_q_delay(const AtomPinId clock_pin, const AtomPinId output_pin) const {
    //Look-up
    auto key = std::make_tuple(clock_pin,output_pin);
    auto iter = delay_cache_.find(key);

    if(iter != delay_cache_.end()) {
        //Hit
        return iter->second;
    }

    //Miss

    //Calculate
    float tcq = raw_delay_calc_.atom_clock_to_q_delay(clock_pin, output_pin);

    //Save in cache
    delay_cache_[key] = tcq;

    return tcq;
}

