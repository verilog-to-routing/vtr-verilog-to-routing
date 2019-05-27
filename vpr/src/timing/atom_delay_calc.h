#ifndef VPR_ATOM_DELAY_CALC_H
#define VPR_ATOM_DELAY_CALC_H

#include "atom_netlist.h"
#include "atom_lookup.h"
#include "vtr_hash.h"
#include "DelayType.h"

//Delay Calculator for timing edges which are internal to atoms
class AtomDelayCalc {
  public:
    AtomDelayCalc(const AtomNetlist& netlist, const AtomLookup& netlist_lookup);
    float atom_combinational_delay(const AtomPinId src_pin, const AtomPinId sink_pin, const DelayType delay_type) const;
    float atom_setup_time(const AtomPinId clk_pin, const AtomPinId input_pin) const;
    float atom_hold_time(const AtomPinId clk_pin, const AtomPinId input_pin) const;
    float atom_clock_to_q_delay(const AtomPinId clk_pin, const AtomPinId output_pin, const DelayType delay_type) const;

  private:
    const t_pb_graph_pin* find_pb_graph_pin(const AtomPinId atom_pin) const;

  private:
    const AtomNetlist& netlist_;
    const AtomLookup& netlist_lookup_;
};

#include "atom_delay_calc.inl"

#endif
