#pragma once

#include <cstdio>
#include <set>
#include "atom_netlist.h"

/**
 * @file
 * @brief Useful utilities for working with the AtomNetlist class
 */

class LogicalModels;

/**
 * @brief Walk through the netlist detecting constant generators
 *
 * @note  Initial constant generators (e.g. vcc/gnd) should have already
 *        been marked on the netlist.
 */
int mark_constant_generators(AtomNetlist& netlist, e_const_gen_inference const_gen_inference_method, const LogicalModels& models, int verbosity);

///@brief Modifies the netlist by absorbing buffer LUTs
void absorb_buffer_luts(AtomNetlist& netlist, const LogicalModels& models, int verbosity);

/**
 * @brief Modifies the netlist by merging all constant generators of the same value
 *        into a single constant generator.
 *
 * Synthesis tools may create a separate constant generator (a LUT with no inputs)
 * for each signal tied to a constant, for example each constant primary output. Since
 * these generators are functionally identical, all generators of the same value are
 * merged into the generator with the highest fanout (e.g. gnd / vcc), and the redundant
 * generators are removed. The names of the removed nets are kept as aliases of the
 * merged net.
 *
 *  @param netlist    The netlist to modify.
 *  @param verbosity  The verbosity of the log messages.
 *
 *  @return The number of constant generators which were removed.
 */
size_t merge_constant_generators(AtomNetlist& netlist, int verbosity);

/*
 * Modify the netlist by sweeping away unused nets/blocks/inputs
 */

/**
 * @brief Repeatedly sweeps the netlist removing blocks and nets
 *        until nothing more can be swept. If sweep_ios is true also sweeps
 *        primary-inputs and primary-outputs
 */
size_t sweep_iterative(AtomNetlist& netlist,
                       bool should_sweep_dangling_ios,
                       bool should_sweep_dangling_blocks,
                       bool should_sweep_dangling_nets,
                       bool should_sweep_constant_primary_outputs,
                       e_const_gen_inference const_gen_inference_method,
                       const LogicalModels& models,
                       int verbosity);

///@brief Sweeps blocks that have no fanout
size_t sweep_blocks(AtomNetlist& netlist, const LogicalModels& models, int verbosity);

///@brief Sweeps nets with no drivers and/or no sinks
size_t sweep_nets(AtomNetlist& netlist, int verbosity);

///@brief Sweeps primary-inputs with no fanout
size_t sweep_inputs(AtomNetlist& netlist, const LogicalModels& models, int verbosity);

///@brief Sweeps primary-outputs with no fanin
size_t sweep_outputs(AtomNetlist& netlist, int verbosity);

size_t sweep_constant_primary_outputs(AtomNetlist& netlist, int verbosity);

/*
 * Truth-table operations
 */

/**
 * @brief Determine whether a truth table encodes the logic functions 'On' set (returns true)
 *        or 'Off' set (returns false)
 */
bool truth_table_encodes_on_set(const AtomNetlist::TruthTable& truth_table);

/**
 * @brief Get the constant value encoded by a truth table, if any.
 *
 * An empty truth table, or a single entry of '0', encodes a constant zero.
 * A single entry of '1' encodes a constant one. For example, in BLIF:
 *
 *      .names gnd      .names gnd2     .names vcc
 *                      0               1
 *
 * @note An empty truth table encodes a constant zero regardless of the number
 *       of inputs of the block. It is up to the caller to check the block's
 *       inputs if required.
 *
 *  @param truth_table  The truth table to inspect.
 *
 *  @return vtr::LogicValue::FALSE or vtr::LogicValue::TRUE if the truth table
 *          encodes a constant zero or one respectively, vtr::LogicValue::UNKNOWN
 *          otherwise.
 */
vtr::LogicValue truth_table_constant_value(const AtomNetlist::TruthTable& truth_table);

/**
 * Returns the truth table expanded to use num_inputs inputs.
 * Typical usage is to expand the truth table of a LUT which is logically smaller than
 * the one provided by the architecture (e.g. implement a 2-LUT in a 6-LUT)
 *
 *   @param truth_table   The truth table to expand
 *   @param num_inputs    The number of inputs to use
 */
AtomNetlist::TruthTable expand_truth_table(const AtomNetlist::TruthTable& truth_table, const size_t num_inputs);

/**
 * @brief Permutes the inputs of a truth table
 *
 *   @param truth_table   The truth table to expand
 *   @param num_inputs    The number of inputs to use
 *   @param permutation   A vector indices to permute, permutation[i] is the input pin where
 *                        the signal currently connected to input i should be placed
 */
AtomNetlist::TruthTable permute_truth_table(const AtomNetlist::TruthTable& truth_table, const size_t num_inputs, const std::vector<int>& permutation);

///@brief Converts a truth table to a lut mask (sequence of binary values representing minterms)
std::vector<vtr::LogicValue> truth_table_to_lut_mask(const AtomNetlist::TruthTable& truth_table, const size_t num_inputs);

/**
 * @brief Converts a logic cube (potnetially including don't cares) into
 *        a sequence of minterm numbers
 */
std::vector<size_t> cube_to_minterms(std::vector<vtr::LogicValue> cube);

/*
 * Print the netlist for debugging
 */
void print_netlist_as_blif(std::string filename, const AtomNetlist& netlist, const LogicalModels& models);
void print_netlist_as_blif(FILE* f, const AtomNetlist& netlist, const LogicalModels& models);

/*
 * Identify all clock nets
 */

/**
 * @brief Returns the set of nets which drive clock pins in the netlist
 *
 * @note The returned nets may be logically equivalent (e.g. driven by buffers
 *       connected to a common net)
 */
std::set<AtomNetId> find_netlist_physical_clock_nets(const AtomNetlist& netlist, const LogicalModels& models);

/**
 * @brief Returns the set of pins which logically drive unique clocks in the netlist
 *
 * @note VPR currently has limited understanding of logic operating on clocks,
 *       so logically unique should be viewed as true only to the extent of VPR's
 *       understanding
 */
std::set<AtomPinId> find_netlist_logical_clock_drivers(const AtomNetlist& netlist, const LogicalModels& models);

///@brief Prints out information about netlist clocks
void print_netlist_clock_info(const AtomNetlist& netlist, const LogicalModels& models);
