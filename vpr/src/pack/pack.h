#ifndef PACK_H
#define PACK_H

#include <unordered_set>
#include <vector>

class AtomNetId;
class FlatPlacementInfo;
struct t_analysis_opts;
struct t_arch;
struct t_det_routing_arch;
struct t_lb_type_rr_node;
struct t_packer_opts;

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
 *  @param routing_arch
 *  @param lb_type_rr_graphs
 *  @param flat_placement_info
 *              Flat (primitive-level) placement information that may be
 *              provided by the user as a hint for packing. Will be invalid if
 *              there is no flat placement information provided.
 */
bool try_pack(t_packer_opts* packer_opts,
              const t_analysis_opts* analysis_opts,
              const t_arch& arch,
              const t_det_routing_arch& routing_arch,
              std::vector<t_lb_type_rr_node>* lb_type_rr_graphs,
              const FlatPlacementInfo& flat_placement_info);

std::unordered_set<AtomNetId> alloc_and_load_is_clock();

#endif
