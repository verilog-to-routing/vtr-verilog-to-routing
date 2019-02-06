#ifndef ATOM_NETLIST_UTILS_H
#define ATOM_NETLIST_UTILS_H
#include <cstdio>
#include <set>
#include "atom_netlist.h"

/*
 *
 * Useful utilities for working with the AtomNetlist class
 *
 */

//Walk through the netlist detecting constant generators
//
//Note that initial constant generators (e.g. vcc/gnd) should have already been marked on
//the netlist.
int mark_constant_generators(AtomNetlist& netlist, e_const_gen_inference const_gen_inference_method, int verbosity);

/*
 * Modifies the netlist by absorbing buffer LUTs
 */
void absorb_buffer_luts(AtomNetlist& netlist, int verbosity);

/*
 * Modify the netlist by sweeping away unused nets/blocks/inputs
 */
//Repeatedly sweeps the netlist removing blocks and nets
//until nothing more can be swept.
//If sweep_ios is true also sweeps primary-inputs and primary-outputs
size_t sweep_iterative(AtomNetlist& netlist,
                       bool should_sweep_dangling_ios,
                       bool should_sweep_dangling_blocks,
                       bool should_sweep_dangling_nets,
                       bool should_sweep_constant_primary_outputs,
                       int verbosity);

//Sweeps blocks that have no fanout
size_t sweep_blocks(AtomNetlist& netlist, int verbosity);

//Sweeps nets with no drivers and/or no sinks
size_t sweep_nets(AtomNetlist& netlist, int verbosity);

//Sweeps primary-inputs with no fanout
size_t sweep_inputs(AtomNetlist& netlist, int verbosity);

//Sweeps primary-outputs with no fanin
size_t sweep_outputs(AtomNetlist& netlist, int verbosity);

size_t sweep_constant_primary_outputs(AtomNetlist& netlist, int verbosity);

/*
 * Truth-table operations
 */
//Returns true if the specified block is a logical buffer
bool is_buffer(const AtomNetlist& netlist, const AtomBlockId blk);

//Deterimine whether a truth table encodes the logic functions 'On' set (returns true)
//or 'Off' set (returns false)
bool truth_table_encodes_on_set(const AtomNetlist::TruthTable& truth_table);

//Returns the truth table expanded to use num_inputs inputs.
//Typical usage is to expand the truth table of a LUT which is logically smaller than
//the one provided by the architecture (e.g. implement a 2-LUT in a 6-LUT)
//   truth_table: The truth table to expand
//   num_inputs : The number of inputs to use
AtomNetlist::TruthTable expand_truth_table(const AtomNetlist::TruthTable& truth_table, const size_t num_inputs);

//Permutes the inputs of a truth table
//   truth_table: The truth table to expand
//   num_inputs : The number of inputs to use
//   permutation: A vector indicies to permute, permutation[i] is the input pin where
//                the signal currently connected to input i should be placed
AtomNetlist::TruthTable permute_truth_table(const AtomNetlist::TruthTable& truth_table, const size_t num_inputs, const std::vector<int>& permutation);

//Convers a truth table to a lut mask (sequence of binary values representing minterms)
std::vector<vtr::LogicValue> truth_table_to_lut_mask(const AtomNetlist::TruthTable& truth_table, const size_t num_inputs);

//Convers a logic cube (potnetially including don't cares) into
//a sequence of minterm numbers
std::vector<size_t> cube_to_minterms(std::vector<vtr::LogicValue> cube);

/*
 * Print the netlist for debugging
 */
void print_netlist_as_blif(std::string filename, const AtomNetlist& netlist);
void print_netlist_as_blif(FILE* f, const AtomNetlist& netlist);

//Returns a user-friendly architectural identifier for the specified atom pin
std::string atom_pin_arch_name(const AtomNetlist& netlist, const AtomPinId pin);

/*
 * Identify all clock nets
 */

//Returns the set of nets which drive clock pins in the netlist
// Note the returned nets may be logically equivalent (e.g. driven by buffers
// connected to a common net)
std::set<AtomNetId> find_netlist_physical_clock_nets(const AtomNetlist& netlist);

//Returns the set of pins which logically drive unique clocks in the netlist
// Note that VPR currently has limited understanding of logic operating on clocks,
// so logically unique should be viewed as true only to the extent of VPR's
// understanding
std::set<AtomPinId> find_netlist_logical_clock_drivers(const AtomNetlist& netlist);

//Prints out information about netlist clocks
void print_netlist_clock_info(const AtomNetlist& netlist);
#endif
