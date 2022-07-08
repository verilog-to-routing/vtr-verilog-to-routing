#ifndef ARCH_CHECK_H
#define ARCH_CHECK_H

/**
 *  This file includes all the definitions of functions which purpose is to
 *  check the correctness of the architecture's internal data structures.
 *
 *  All new functions corresponding to the architecture checking should end up here.
 */

#include "arch_types.h"
#include "arch_util.h"

#include "physical_types_util.h"

#include "vtr_util.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Checks whether the model has correct clock port specifications
 *
 * @param model model definition
 * @param file architecture file
 * @param line line in the architecture file that generates the failure
 */
bool check_model_clocks(t_model* model, const char* file, uint32_t line);

/**
 * @brief Checks the correctness of the combinational sinks in the model inputs to outputs connections
 *
 * @param model model definition
 * @param file architecture file
 * @param line line in the architecture file that generates the failure
 */
bool check_model_combinational_sinks(const t_model* model, const char* file, uint32_t line);

/**
 * @brief Checks whether the I/O ports can have timing specifications based on their connectivity.
 *        A port can have timing specs whether it is clocked or is combinationally connected to a
 *        corresponding I/O port.
 *        If the check fails, a warning is printed in the output log.
 *
 * @param model model definition
 * @param file architecture file
 * @param line line in the architecture file that generates the failure
 */
void warn_model_missing_timing(const t_model* model, const char* file, uint32_t line);

/**
 * @brief Checks the consistency of the mappings between a logical block and the corresponding physical tile.
 *
 * @param physical_tile physical tile type
 * @param sub_tile sub tile to check
 * @param logical_block logical block type
 */
void check_port_direct_mappings(t_physical_tile_type_ptr physical_tile, t_sub_tile* sub_tile, t_logical_block_type_ptr logical_block);

/**
 * @brief Checks the timing consistency between tha pb_type and the corresponding model.
 *
 * @param pb_type pb type to check
 * @param arch architecture data structure
 */
bool check_leaf_pb_model_timing_consistency(const t_pb_type* pb_type, const t_arch& arch);

/**
 * @brief Checks that each model has at least one corresponding pb type. This function also updates the port indices of the models
 *        based on their type: e.g. clock, input, output.
 *
 * @param arch architecture data structure
 */
void check_models(t_arch* arch);
#ifdef __cplusplus
}
#endif

#endif
