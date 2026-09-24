/**
 * @file
 * @author  Haydar Cakan
 * @date    March 2026
 * @brief   Implementation of device size estimation before packing.
 *
 * Non-RAM molecules are grouped by logical block type, and the number of
 * physical instances required for each type is estimated using a lightweight
 * pin-capacity and cluster placement feasibility check. RAM atoms are grouped
 * into sibling-feasible equivalence classes and assigned to the minimum-area
 * physical RAM type. The resulting RAM groups are stored in ram_groups_ and
 * exposed via ram_groups() for reuse by RamMapper, avoiding redundant work.
 *
 * Note: For the cluster feasibility check, we refer to the terms primitive and
 *       atom. In this context, an atom is the smallest netlist element (AtomBlockId)
 *       to be placed inside a cluster, while a primitive (t_pb_graph_node)
 *       represents a placement site within a cluster. Cluster placement
 *       assigns atoms to compatible primitive sites inside a cluster.
 */

#include "device_size_estimate.h"

#include <limits>
#include <queue>
#include <unordered_set>

#include "cluster_placement.h"
#include "cluster_util.h"
#include "globals.h"
#include "prepack.h"
#include "pugixml.hpp"
#include "setup_grid.h"
#include "vpr_types.h"
#include "vpr_utils.h"
#include "vpr_error.h"
#include "vtr_log.h"
#include "vtr_math.h"
#include "vtr_time.h"
#include "vtr_util.h"

#ifdef VTR_ENABLE_CAPNPROTO
#include "mmap_file.h"
#include "rr_graph_uxsdcxx_capnp.h"
#include <capnp/message.h>
#endif

/**
 * @brief Attempts to place a molecule into the current cluster.
 *
 * Tries each candidate primitive site (from the candidate queue built using
 * primitives_list) until one succeeds.
 *
 * @param cluster_stats    Current cluster placement state.
 * @param mol_id           Molecule to be placed.
 * @param primitives_list  Candidate primitive locations (also used to build the queue).
 * @param prepacker        Used to build candidate primitive sites and to check
 *                         whether the molecule atoms can be placed at each site.
 * @return                 True if the molecule was placed successfully, false otherwise.
 */
static bool can_place_molecule(t_intra_cluster_placement_stats* cluster_stats,
                               PackMoleculeId mol_id,
                               std::vector<t_pb_graph_node*>& primitives_list,
                               const Prepacker& prepacker) {
    LazyPopUniquePriorityQueue<t_pb_graph_node*, std::tuple<float, int, int>>
        candidate_queue = build_primitive_candidate_queue(cluster_stats, mol_id, primitives_list, prepacker);

    while (!candidate_queue.empty()) {
        t_pb_graph_node* root = candidate_queue.pop().first;

        // Reset the placement state for this attempt. primitives_list stores
        // the tentative primitive assignments for the current molecule, so it
        // must be cleared before trying a new root candidate.
        std::fill(primitives_list.begin(), primitives_list.end(), nullptr);

        if (try_start_root_placement(cluster_stats, mol_id, root, primitives_list, prepacker)) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Opens a fresh cluster and tries to place the given molecule into it
 *        by iterating over all modes of the logical type.
 *
 * @param logical_type     Logical block type whose modes are tried when opening
 *                         the new cluster.
 * @param mol_id           Molecule to place in the new cluster.
 * @param primitives_list  Scratch/output vector. Must be pre-sized to
 *                         mol.atom_block_ids.size(). It is cleared before each
 *                         placement attempt. On success, entry i stores the
 *                         primitive location assigned to mol.atom_block_ids[i].
 * @param prepacker        Used to build candidate primitive sites and to check
 *                         whether the molecule atoms can be placed at each site.
 * @return                 Cluster placement stats for the first mode that
 *                         accepts the molecule, or nullptr if no mode works.
 *                         The caller takes ownership and must call
 *                         free_cluster_placement_stats() when done.
 */
static t_intra_cluster_placement_stats* open_cluster_for_molecule(t_logical_block_type_ptr logical_type,
                                                                  PackMoleculeId mol_id,
                                                                  std::vector<t_pb_graph_node*>& primitives_list,
                                                                  const Prepacker& prepacker) {
    int num_modes = logical_type->pb_graph_head->pb_type->num_modes;

    // Types with no explicit modes are treated as having a single mode (mode 0).
    int modes_to_try = std::max(1, num_modes);
    for (int mode = 0; mode < modes_to_try; mode++) {
        t_intra_cluster_placement_stats* stats = alloc_and_load_cluster_placement_stats(logical_type, mode);
        std::fill(primitives_list.begin(), primitives_list.end(), nullptr);
        if (can_place_molecule(stats, mol_id, primitives_list, prepacker))
            return stats;
        free_cluster_placement_stats(stats);
    }
    return nullptr;
}

/**
 * @brief Estimates the number of clusters needed for a single logical
 *        block type using pin-capacity and placement feasibility checks.
 *
 * Molecules are packed greedily into clusters. A new cluster is opened
 * when either adding the molecule would exceed pin capacity (based on
 * unique external nets) or when the molecule cannot be placed into the
 * current cluster. When opening a new cluster, all modes of the logical
 * type are tried in order.
 *
 * @param logical_type  The logical block type to simulate packing for; its pb_type
 *                      determines the pin capacities and available primitive sites.
 * @param mol_ids       Molecules to pack, pre-filtered to belong to this logical type.
 * @param prepacker     Used to query molecule structure, placement feasibility,
 *                      and external net counts.
 * @param atom_nlist    Used to compute the external nets each molecule contributes,
 *                      needed for the pin-capacity check.
 * @return              Number of clusters needed to fit all molecules.
 */
static size_t count_clusters_for_type(t_logical_block_type_ptr logical_type,
                                      const std::vector<PackMoleculeId>& mol_ids,
                                      const Prepacker& prepacker,
                                      const AtomNetlist& atom_nlist) {
    const int input_pin_capacity = logical_type->pb_type->num_input_pins;
    const int output_pin_capacity = logical_type->pb_type->num_output_pins;

    std::unordered_set<AtomNetId> cur_input_nets;
    std::unordered_set<AtomNetId> cur_output_nets;
    t_intra_cluster_placement_stats* cluster_stats = nullptr;
    size_t cluster_count = 0;

    for (PackMoleculeId mol_id : mol_ids) {
        const t_pack_molecule& mol = prepacker.get_molecule(mol_id);
        std::vector<t_pb_graph_node*> primitives_list(mol.atom_block_ids.size(), nullptr);

        // Count new unique external nets this molecule would add.
        // Clock nets are excluded from pin-capacity checks.
        t_molecule_external_nets external_nets = prepacker.calc_molecule_external_nets(mol_id, atom_nlist);
        int new_input_nets = 0, new_output_nets = 0;
        for (AtomNetId net : external_nets.ext_input_nets) {
            if (!cur_input_nets.count(net))
                ++new_input_nets;
        }
        for (AtomNetId net : external_nets.ext_output_nets) {
            if (!cur_output_nets.count(net))
                ++new_output_nets;
        }

        bool pin_overflow = (input_pin_capacity > 0 && static_cast<int>(cur_input_nets.size()) + new_input_nets > input_pin_capacity) || (output_pin_capacity > 0 && static_cast<int>(cur_output_nets.size()) + new_output_nets > output_pin_capacity);

        // Pin capacity exceeded or no cluster open yet, open a new one.
        if (!cluster_stats || pin_overflow) {
            if (cluster_stats)
                free_cluster_placement_stats(cluster_stats);
            cluster_stats = open_cluster_for_molecule(logical_type, mol_id, primitives_list, prepacker);
            ++cluster_count;
            cur_input_nets.clear();
            cur_output_nets.clear();
            if (!cluster_stats) {
                // Molecule cannot fit in any mode of a fresh cluster; skip it and count as one cluster.
                continue;
            }
        } else {
            // Try to place in the current cluster; if that fails, open a new one.
            bool placed = can_place_molecule(cluster_stats, mol_id, primitives_list, prepacker);
            if (!placed) {
                free_cluster_placement_stats(cluster_stats);
                cluster_stats = open_cluster_for_molecule(logical_type, mol_id, primitives_list, prepacker);
                ++cluster_count;
                cur_input_nets.clear();
                cur_output_nets.clear();
                if (!cluster_stats) {
                    // Molecule cannot fit in any mode of a fresh cluster; skip it and count as one cluster.
                    continue;
                }
            }
        }

        // Commit the successful placement by marking the assigned primitives as used
        // and updating the cluster placement state for subsequent molecule placements.
        for (t_pb_graph_node* prim : primitives_list) {
            if (prim)
                commit_primitive(cluster_stats, prim);
        }

        // Record the external nets used by this molecule in the current cluster.
        cur_input_nets.insert(external_nets.ext_input_nets.begin(), external_nets.ext_input_nets.end());
        cur_output_nets.insert(external_nets.ext_output_nets.begin(), external_nets.ext_output_nets.end());
    }

    if (cluster_stats)
        free_cluster_placement_stats(cluster_stats);

    return cluster_count;
}

/**
 * @brief Returns true if any output pin of the given primitive can reach a
 *        root block pin via forward pb_graph edge traversal.
 *
 * Used during type selection to skip candidate types where the primitive's
 * output is entirely internal (e.g. OCT's inpad on Stratix 10, whose output
 * feeds only oct_block.rzqin and never reaches OCT's external core_out port).
 *
 * @param prim  A primitive pb_graph_node (leaf node with a blif_model).
 * @return      True if a forward path to a root block pin exists, false otherwise.
 */
static bool primitive_has_external_output(const t_pb_graph_node* prim) {
    std::unordered_set<const t_pb_graph_pin*> visited;
    std::queue<const t_pb_graph_pin*> bfs_queue;

    for (int port_idx = 0; port_idx < prim->num_output_ports; port_idx++) {
        for (int pin_idx = 0; pin_idx < prim->num_output_pins[port_idx]; pin_idx++) {
            bfs_queue.push(&prim->output_pins[port_idx][pin_idx]);
        }
    }

    while (!bfs_queue.empty()) {
        const t_pb_graph_pin* cur_pin = bfs_queue.front();
        bfs_queue.pop();
        if (visited.find(cur_pin) != visited.end())
            continue;
        visited.insert(cur_pin);

        if (cur_pin->is_root_block_pin())
            return true;

        for (t_pb_graph_edge* edge : cur_pin->output_edges) {
            for (int sink_idx = 0; sink_idx < edge->num_output_pins; sink_idx++) {
                bfs_queue.push(edge->output_pins[sink_idx]);
            }
        }
    }
    return false;
}

std::map<t_logical_block_type_ptr, size_t> DeviceSizeEstimator::estimate_resource_requirement(const Prepacker& prepacker, bool store_ram_groups) {
    vtr::ScopedStartFinishTimer timer("Estimate Resource Requirement");
    const auto& atom_ctx = g_vpr_ctx.atom();

    // Group RAM atoms and assign to minimum-area types.
    // If requested, results are stored in ram_groups_ for later use by RamMapper.
    vtr::vector<LogicalRamGroupId, LogicalRamGroup> ram_groups = group_ram_atoms(atom_ctx.netlist(), prepacker);
    assign_ram_groups_by_min_area(ram_groups, false /*is_fixed_device*/);
    if (store_ram_groups)
        ram_groups_ = ram_groups;

    // Group non-RAM molecules by their logical block type.
    vtr::vector<LogicalModelId, std::vector<t_logical_block_type_ptr>>
        primitive_candidate_block_types = identify_primitive_candidate_block_types();

    std::unordered_map<t_logical_block_type_ptr, std::vector<PackMoleculeId>> logical_type_molecules;
    for (PackMoleculeId mol_id : prepacker.molecules()) {
        const t_pack_molecule& mol = prepacker.get_molecule(mol_id);
        AtomBlockId root_atom = mol.atom_block_ids[mol.root];
        const t_pb_graph_node* prim = prepacker.get_expected_lowest_cost_pb_gnode(root_atom);

        // Skip RAMs, they are handled separately via logical ram groups.
        if (!prim || !prim->pb_type || !prim->pb_type->is_primitive())
            continue;
        if (prim->pb_type->class_type == MEMORY_CLASS)
            continue;

        LogicalModelId root_model_id = atom_ctx.netlist().block_model(root_atom);

        // Select the first candidate type whose primitive has a forward path to the
        // complex block's external output pins. This filters out types like OCT on
        // S10, where the inpad primitive connects only to internal pins, making it
        // a poor basis for resource estimation. Falls back to the first candidate
        // if none qualifies (e.g. outpad atoms with no output pins).
        t_logical_block_type_ptr mapped_type = nullptr;
        for (t_logical_block_type_ptr cand : primitive_candidate_block_types[root_model_id]) {
            std::vector<t_logical_block_type> candidate_logical_type = {*cand};
            const t_pb_graph_node* current_type_prim = prepacker.get_expected_lowest_cost_primitive_for_atom_block(root_atom, candidate_logical_type);
            if (current_type_prim && primitive_has_external_output(current_type_prim)) {
                mapped_type = cand;
                break;
            }
        }
        if (!mapped_type)
            mapped_type = primitive_candidate_block_types[root_model_id][0];

        logical_type_molecules[mapped_type].push_back(mol_id);
    }

    // Estimate instance counts for non-RAM types using pin capacity and cluster placement.
    std::map<t_logical_block_type_ptr, size_t> num_type_instances;
    for (auto& [logical_type, mol_ids] : logical_type_molecules) {
        num_type_instances[logical_type] = count_clusters_for_type(logical_type, mol_ids, prepacker, atom_ctx.netlist());
    }

    // Estimate instance counts for RAM types using logical RAM groups.
    for (LogicalRamGroupId ram_group_id : ram_groups.keys()) {
        const LogicalRamGroup& ram_group = ram_groups[ram_group_id];
        t_logical_block_type_ptr logical_type = ram_group.pre_assigned_type ? ram_group.pre_assigned_type : ram_group.candidate_types[0];
        size_t inferred_number_of_instances = std::ceil(vtr::safe_ratio<float>(ram_group.total_memory_slices, ram_group.candidate_capacity.at(logical_type)));
        num_type_instances[logical_type] += inferred_number_of_instances;
    }

    return num_type_instances;
}

/**
 * @brief Reads device grid dimensions from an RR graph file (.xml or .bin)
 *        without loading the full graph into memory.
 *
 * TODO: The grid width and height can be explicitly stored in the rr graph
 *       file. That would be a more cleaner solution instead of inferring
 *       the dimension from the grid locations as in this function.
 * 
 * @return {width, height} of the device grid encoded in the RR graph file.
 */
static std::pair<size_t, size_t> read_rr_graph_grid_dims(const std::string& filename) {
    int max_x = -1, max_y = -1;

    if (vtr::check_file_name_extension(filename, ".xml")) {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(filename.c_str());
        if (!result) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Failed to parse RR graph file '%s' to read grid dimensions: %s",
                            filename.c_str(), result.description());
        }

        pugi::xml_node grid = doc.child("rr_graph").child("grid");
        if (!grid) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "RR graph file '%s' is missing the <grid> element.",
                            filename.c_str());
        }

        for (pugi::xml_node loc : grid.children("grid_loc")) {
            max_x = std::max(max_x, loc.attribute("x").as_int());
            max_y = std::max(max_y, loc.attribute("y").as_int());
        }
#ifdef VTR_ENABLE_CAPNPROTO
    } else if (vtr::check_file_name_extension(filename, ".bin")) {
        MmapFile f(filename.c_str());
        ::capnp::ReaderOptions opts;
        opts.traversalLimitInWords = std::numeric_limits<uint64_t>::max();
        ::capnp::FlatArrayMessageReader reader(f.getData(), opts);
        auto rr_graph = reader.getRoot<ucap::RrGraph>();
        for (auto loc : rr_graph.getGrid().getGridLocs()) {
            max_x = std::max(max_x, (int)loc.getX());
            max_y = std::max(max_y, (int)loc.getY());
        }
#endif
    } else {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "RR graph file '%s' has an unrecognized extension. Expected .xml or .bin.",
                        filename.c_str());
    }

    if (max_x < 0 || max_y < 0) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "RR graph file '%s' has no grid_loc entries.",
                        filename.c_str());
    }

    return {(size_t)(max_x + 1), (size_t)(max_y + 1)};
}

DeviceSizeEstimator::DeviceSizeEstimator(t_vpr_setup& vpr_setup,
                                         const t_arch& arch,
                                         const Prepacker& prepacker,
                                         bool always_estimate_resource_requirement) {
    vtr::ScopedStartFinishTimer timer("Estimate Device Size");
    const std::string& device_layout = vpr_setup.PackerOpts.device_layout;
    const t_packer_opts& packer_opts = vpr_setup.PackerOpts;
    auto& device_ctx = g_vpr_ctx.mutable_device();

    // If RR graph is provided, use the device size from RR graph file.
    bool rr_graph_provided = !vpr_setup.RoutingArch.read_rr_graph_filename.empty();
    if (rr_graph_provided) {
        VTR_LOG("Using the device size provided in RR graph.\n");
        auto [width, height] = read_rr_graph_grid_dims(vpr_setup.RoutingArch.read_rr_graph_filename);
        VTR_LOG("RR graph grid dimensions: %zu x %zu.\n", width, height);

        if (device_layout != "auto") {
            std::map<t_logical_block_type_ptr, size_t> num_type_instances;
            DeviceGrid fixed_grid = create_device_grid(device_layout, arch.grid_layouts,
                                                       num_type_instances,
                                                       packer_opts.target_device_utilization);
            if (width != fixed_grid.width() || height != fixed_grid.height()) {
                VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                                "Fixed device layout '%s' (width: %zu, height: %zu) does not match "
                                "the RR graph grid dimensions (width: %zu, height: %zu).",
                                device_layout.c_str(),
                                fixed_grid.width(), fixed_grid.height(), width, height);
            }
        }

        if (vpr_setup.device_width > 0 && width != static_cast<size_t>(vpr_setup.device_width)) {
            VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                            "Fixed device width %d (from --device_width) does not match "
                            "the RR graph grid width %zu.",
                            vpr_setup.device_width, width);
        }

        // Fix the device grid to the size implied by rr graph to prevent
        // resizing during and after packing.
        device_ctx.grid = create_device_grid(device_layout, arch.grid_layouts, width, height);
        device_ctx.grid.set_fixed_by_rr_graph(true);
        if (always_estimate_resource_requirement)
            estimated_num_type_instances_ = estimate_resource_requirement(prepacker, /*store_ram_groups=*/false);
        return;
    }

    // If device size is fixed, create device grid without estimation.
    if (device_layout != "auto") {
        VTR_LOG("Device is fixed to %s.\n", device_layout.c_str());
        std::map<t_logical_block_type_ptr, size_t> num_type_instances;
        device_ctx.grid = create_device_grid(device_layout, arch.grid_layouts,
                                             num_type_instances,
                                             packer_opts.target_device_utilization);
        if (always_estimate_resource_requirement)
            estimated_num_type_instances_ = estimate_resource_requirement(prepacker, /*store_ram_groups=*/false);
        return;
    }

    VTR_ASSERT(device_layout == "auto");

    if (vpr_setup.device_width > 0) {
        VTR_LOG("Device layout 'auto' with fixed width %d.\n", vpr_setup.device_width);
        device_ctx.grid = create_device_grid(device_layout, arch.grid_layouts,
                                             {}, packer_opts.target_device_utilization,
                                             vpr_setup.device_width);
        report_device_grid_stats(device_ctx.grid);
        if (always_estimate_resource_requirement)
            estimated_num_type_instances_ = estimate_resource_requirement(prepacker, /*store_ram_groups=*/false);
        return;
    }

    VTR_LOG("Device layout '%s' selected. Need to estimate device size.\n", device_layout.c_str());

    std::map<t_logical_block_type_ptr, size_t> num_type_instances = estimate_resource_requirement(prepacker);
    estimated_num_type_instances_ = num_type_instances;
    VTR_LOG("Estimated resource requirements:\n");
    for (auto& [type_ptr, count] : num_type_instances)
        VTR_LOG("  %s: %zu requested instances\n", type_ptr->name.c_str(), count);

    device_ctx.grid = create_device_grid(device_layout, arch.grid_layouts,
                                         num_type_instances,
                                         packer_opts.target_device_utilization,
                                         vpr_setup.device_width);

    size_t num_grid_tiles = count_grid_tiles(device_ctx.grid);
    VTR_LOG("FPGA size estimated to %zu x %zu: %zu grid tiles (%s)\n",
            device_ctx.grid.width(), device_ctx.grid.height(),
            num_grid_tiles, device_ctx.grid.name().c_str());
}
