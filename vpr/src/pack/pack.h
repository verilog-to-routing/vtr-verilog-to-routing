#pragma once

#include <map>
#include <string>
#include <unordered_set>
#include <vector>

class AtomNetId;
class FlatPlacementInfo;
class PreClusterTimingManager;
class Prepacker;
struct t_analysis_opts;
struct t_ap_opts;
struct t_arch;
struct t_lb_type_rr_node;
struct t_packer_opts;
struct t_logical_block_type;
using t_logical_block_type_ptr = const t_logical_block_type*;

/**
 * @brief Try to pack the atom netlist into legal clusters on the given
 *        architecture. Will return true if successful, false otherwise.
 *
 *  @param packer_opts
 *              Options passed by the user to configure the packing algorithm.
 *  @param analysis_opts
 *              Options passed by the user to configure how analysis is
 *              performed in the packer.
 *  @param arch
 *              The architecture to create clusters for.
 *  @param lb_type_rr_graphs
 *  @param prepacker
 *              The prepacker used to form atoms into molecules prior to packing.
 *  @param pre_cluster_timing_manager
 *              Manager object to store the pre-computed timing delay calculations.
 *              Used to inform the packer of timing critical paths.
 *  @param flat_placement_info
 *              Flat (primitive-level) placement information that may be
 *              provided by the user as a hint for packing. Will be invalid if
 *              there is no flat placement information provided.
 */
bool try_pack(const t_packer_opts& packer_opts,
              const t_analysis_opts& analysis_opts,
              const t_ap_opts& ap_opts,
              const t_arch& arch,
              std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
              const Prepacker& prepacker,
              const PreClusterTimingManager& pre_cluster_timing_manager,
              const FlatPlacementInfo& flat_placement_info);

/**
 * @brief Try to fit the block type instances on the given architecture. Will
 *        return true if successful, false otherwise.
 *
 *  @param arch
 *              The architecture to try to fit the given instances.
 *  @param num_type_instances
 *              The mapping from each logical block type to the number of instances
 *              of the corresponding logical block type.
 *  @param type_util
 *              The utilization of each logical block type on the architecture to
 *              be filled.
 *  @param target_device_utilization
 *              The target device utilization.
 *  @param device_layout_name
 *              The device layout name for that architecture.
 */
bool try_size_device_grid(const t_arch& arch,
                          const std::map<t_logical_block_type_ptr, size_t>& num_type_instances,
                          std::map<t_logical_block_type_ptr, float>& type_util,
                          float target_device_utilization,
                          const std::string& device_layout_name);

std::unordered_set<AtomNetId> alloc_and_load_is_clock();
