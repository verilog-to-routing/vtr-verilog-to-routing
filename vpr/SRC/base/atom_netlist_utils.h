#ifndef ATOM_NETLIST_UTILS_H
#define ATOM_NETLIST_UTILS_H
#include <cstdio>
#include "atom_netlist.h"

/*
 *
 * Useful utilities for working with the AtomNetlist class
 *
 */

/*
 * Modifies the netlist by absorbing buffer LUTs
 */
void absorb_buffer_luts(AtomNetlist& netlist);

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
                       bool should_sweep_constant_primary_outputs);

//Sweeps blocks that have no fanout
size_t sweep_blocks(AtomNetlist& netlist);

//Sweeps nets with no drivers and/or no sinks
size_t sweep_nets(AtomNetlist& netlist);

//Sweeps primary-inputs with no fanout
size_t sweep_inputs(AtomNetlist& netlist);

//Sweeps primary-outputs with no fanin
size_t sweep_outputs(AtomNetlist& netlist);

size_t sweep_constant_primary_outputs(AtomNetlist& netlist);

/*
 * Truth-table operations
 */
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

#endif
