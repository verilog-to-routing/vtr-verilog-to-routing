#include <cmath>
#include "atom_delay_calc.h"
/*
 * AtomDelayCalc
 */
inline AtomDelayCalc::AtomDelayCalc(const AtomNetlist& netlist, const AtomLookup& netlist_lookup)
    : netlist_(netlist)
    , netlist_lookup_(netlist_lookup) {}

inline float AtomDelayCalc::atom_combinational_delay(const AtomPinId src_pin, const AtomPinId sink_pin, const DelayType delay_type) const {
    VTR_ASSERT_MSG(netlist_.pin_block(src_pin) == netlist_.pin_block(sink_pin), "Combinational primitive delay must be between pins on the same block");

    VTR_ASSERT_MSG(   netlist_.port_type(netlist_.pin_port(src_pin)) == PortType::INPUT 
                   && netlist_.port_type(netlist_.pin_port(sink_pin)) == PortType::OUTPUT,
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
            if (delay_type == DelayType::MAX) {
                delay = src_gpin->pin_timing_del_max[i];
            } else {
                VTR_ASSERT(delay_type == DelayType::MIN);
                delay = src_gpin->pin_timing_del_min[i];
            }
            break;
        }
    }

    VTR_ASSERT_MSG(!std::isnan(delay), "Must have a valid delay");

    return delay;
}

inline float AtomDelayCalc::atom_setup_time(const AtomPinId /*clock_pin*/, const AtomPinId pin) const {
    auto port_type = netlist_.port_type(netlist_.pin_port(pin));
    VTR_ASSERT(port_type == PortType::OUTPUT || port_type == PortType::INPUT);

    const t_pb_graph_pin* gpin = find_pb_graph_pin(pin);
    VTR_ASSERT(gpin->type == PB_PIN_SEQUENTIAL);

    return gpin->tsu;
}

inline float AtomDelayCalc::atom_hold_time(const AtomPinId /*clock_pin*/, const AtomPinId pin) const {
    auto port_type = netlist_.port_type(netlist_.pin_port(pin));
    VTR_ASSERT(port_type == PortType::OUTPUT || port_type == PortType::INPUT);

    const t_pb_graph_pin* gpin = find_pb_graph_pin(pin);
    VTR_ASSERT(gpin->type == PB_PIN_SEQUENTIAL);

    return gpin->thld;
}

inline float AtomDelayCalc::atom_clock_to_q_delay(const AtomPinId /*clock_pin*/, const AtomPinId pin, const DelayType delay_type) const {
    auto port_type = netlist_.port_type(netlist_.pin_port(pin));
    VTR_ASSERT(port_type == PortType::OUTPUT || port_type == PortType::INPUT);

    const t_pb_graph_pin* gpin = find_pb_graph_pin(pin);
    VTR_ASSERT(gpin->type == PB_PIN_SEQUENTIAL);

    if (delay_type == DelayType::MAX) {
        return gpin->tco_max;
    } else {
        VTR_ASSERT(delay_type == DelayType::MIN);
        return gpin->tco_min;
    }
}

inline const t_pb_graph_pin* AtomDelayCalc::find_pb_graph_pin(const AtomPinId atom_pin) const {
    //Note that the netlist lookup already considers any pin rotations applied during clustering,
    //which were considered when the look-up was created (i.e. in read_netlist).
    //
    //Therefore, despite the fact that we look-up the delays on the atom netlist (which does not
    //reflect the current pin rotation state), we do get actual post-rotation physical pin here.
    //
    //This means we will get the 'correct' delay (considering any applied rotations).
    const t_pb_graph_pin* gpin = netlist_lookup_.atom_pin_pb_graph_pin(atom_pin);
    VTR_ASSERT(gpin != nullptr);

    return gpin;
}

