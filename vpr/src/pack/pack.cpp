#include <cstdio>
#include <cstring>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include <stdlib.h>
using namespace std;

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_math.h"

#include "vpr_error.h"
#include "vpr_types.h"

#include "read_xml_arch_file.h"
#include "globals.h"
#include "atom_netlist.h"
#include "prepack.h"
#include "pack_types.h"
#include "pack.h"
#include "read_blif.h"
#include "cluster.h"
#include "SetupGrid.h"

/* #define DUMP_PB_GRAPH 1 */
/* #define DUMP_BLIF_INPUT 1 */

static std::unordered_set<AtomNetId> alloc_and_load_is_clock(bool global_clocks);
static bool try_size_device_grid(const t_arch& arch, const std::map<t_type_ptr, size_t>& num_type_instances, float target_device_utilization, std::string device_layout_name);

static t_ext_pin_util_targets parse_target_external_pin_util(std::vector<std::string> specs);
static std::string target_external_pin_util_to_string(const t_ext_pin_util_targets& ext_pin_utils);

static t_pack_high_fanout_thresholds parse_high_fanout_thresholds(std::vector<std::string> specs);
static std::string high_fanout_thresholds_to_string(const t_pack_high_fanout_thresholds& hf_thresholds);

bool try_pack(t_packer_opts* packer_opts,
              const t_analysis_opts* analysis_opts,
              const t_arch* arch,
              const t_model* user_models,
              const t_model* library_models,
              float interc_delay,
              vector<t_lb_type_rr_node>* lb_type_rr_graphs) {
    std::unordered_set<AtomNetId> is_clock;
    std::multimap<AtomBlockId, t_pack_molecule*> atom_molecules;                     //The molecules associated with each atom block
    std::unordered_map<AtomBlockId, t_pb_graph_node*> expected_lowest_cost_pb_gnode; //The molecules associated with each atom block
    const t_model* cur_model;
    int num_models;
    t_pack_patterns* list_of_packing_patterns;
    int num_packing_patterns;
    t_pack_molecule *list_of_pack_molecules, *cur_pack_molecule;
    VTR_LOG("Begin packing '%s'.\n", packer_opts->blif_file_name.c_str());

    /* determine number of models in the architecture */
    num_models = 0;
    cur_model = user_models;
    while (cur_model) {
        num_models++;
        cur_model = cur_model->next;
    }
    cur_model = library_models;
    while (cur_model) {
        num_models++;
        cur_model = cur_model->next;
    }

    is_clock = alloc_and_load_is_clock(packer_opts->global_clocks);

    auto& atom_ctx = g_vpr_ctx.atom();

    size_t num_p_inputs = 0;
    size_t num_p_outputs = 0;
    for (auto blk_id : atom_ctx.nlist.blocks()) {
        auto type = atom_ctx.nlist.block_type(blk_id);
        if (type == AtomBlockType::INPAD) {
            ++num_p_inputs;
        } else if (type == AtomBlockType::OUTPAD) {
            ++num_p_outputs;
        }
    }

    VTR_LOG("\n");
    VTR_LOG("After removing unused inputs...\n");
    VTR_LOG("\ttotal blocks: %zu, total nets: %zu, total inputs: %zu, total outputs: %zu\n",
            atom_ctx.nlist.blocks().size(), atom_ctx.nlist.nets().size(), num_p_inputs, num_p_outputs);

    VTR_LOG("Begin prepacking.\n");
    list_of_packing_patterns = alloc_and_load_pack_patterns(&num_packing_patterns);
    list_of_pack_molecules = alloc_and_load_pack_molecules(list_of_packing_patterns,
                                                           atom_molecules,
                                                           expected_lowest_cost_pb_gnode,
                                                           num_packing_patterns);
    VTR_LOG("Finish prepacking.\n");

    if (packer_opts->auto_compute_inter_cluster_net_delay) {
        packer_opts->inter_cluster_net_delay = interc_delay;
        VTR_LOG("Using inter-cluster delay: %g\n", packer_opts->inter_cluster_net_delay);
    }

    t_ext_pin_util_targets target_external_pin_util = parse_target_external_pin_util(packer_opts->target_external_pin_util);
    t_pack_high_fanout_thresholds high_fanout_thresholds = parse_high_fanout_thresholds(packer_opts->high_fanout_threshold);

    VTR_LOG("Packing with pin utilization targets: %s\n", target_external_pin_util_to_string(target_external_pin_util).c_str());
    VTR_LOG("Packing with high fanout thresholds: %s\n", high_fanout_thresholds_to_string(high_fanout_thresholds).c_str());

    bool allow_unrelated_clustering = false;
    if (packer_opts->allow_unrelated_clustering == e_unrelated_clustering::ON) {
        allow_unrelated_clustering = true;
    } else if (packer_opts->allow_unrelated_clustering == e_unrelated_clustering::OFF) {
        allow_unrelated_clustering = false;
    }

    bool balance_block_type_util = false;
    if (packer_opts->balance_block_type_utilization == e_balance_block_type_util::ON) {
        balance_block_type_util = true;
    } else if (packer_opts->balance_block_type_utilization == e_balance_block_type_util::OFF) {
        balance_block_type_util = false;
    }

    int pack_iteration = 1;

    while (true) {
        //Cluster the netlist
        auto num_type_instances = do_clustering(
            *packer_opts,
            *analysis_opts,
            arch, list_of_pack_molecules, num_models,
            is_clock,
            atom_molecules,
            expected_lowest_cost_pb_gnode,
            allow_unrelated_clustering,
            balance_block_type_util,
            lb_type_rr_graphs,
            target_external_pin_util,
            high_fanout_thresholds);

        //Try to size/find a device
        bool fits_on_device = try_size_device_grid(*arch, num_type_instances, packer_opts->target_device_utilization, packer_opts->device_layout);

        if (fits_on_device) {
            break; //Done
        } else if (pack_iteration == 1) {
            //1st pack attempt was unsucessful (i.e. not dense enough) and we have control of unrelated clustering
            //
            //Turn it on to increase packing density
            if (packer_opts->allow_unrelated_clustering == e_unrelated_clustering::AUTO) {
                VTR_ASSERT(allow_unrelated_clustering == false);
                allow_unrelated_clustering = true;
            }
            if (packer_opts->balance_block_type_utilization == e_balance_block_type_util::AUTO) {
                VTR_ASSERT(balance_block_type_util == false);
                balance_block_type_util = true;
            }
            VTR_LOG("Packing failed to fit on device. Re-packing with: unrelated_logic_clustering=%s balance_block_type_util=%s\n",
                    (allow_unrelated_clustering ? "true" : "false"),
                    (balance_block_type_util ? "true" : "false"));
        } else {
            //Unable to pack densely enough: Give Up

            //No suitable device found
            std::string resource_reqs;
            std::string resource_avail;
            auto& grid = g_vpr_ctx.device().grid;
            for (auto iter = num_type_instances.begin(); iter != num_type_instances.end(); ++iter) {
                if (iter != num_type_instances.begin()) {
                    resource_reqs += ", ";
                    resource_avail += ", ";
                }

                resource_reqs += std::string(iter->first->name) + ": " + std::to_string(iter->second);
                resource_avail += std::string(iter->first->name) + ": " + std::to_string(grid.num_instances(iter->first));
            }

            VPR_THROW(VPR_ERROR_OTHER, "Failed to find device which satisifies resource requirements required: %s (available %s)", resource_reqs.c_str(), resource_avail.c_str());
        }

        //Reset clustering for re-packing
        g_vpr_ctx.mutable_clustering().clb_nlist = ClusteredNetlist();
        for (auto blk : g_vpr_ctx.atom().nlist.blocks()) {
            g_vpr_ctx.mutable_atom().lookup.set_atom_clb(blk, ClusterBlockId::INVALID());
            g_vpr_ctx.mutable_atom().lookup.set_atom_pb(blk, nullptr);
        }
        for (auto net : g_vpr_ctx.atom().nlist.nets()) {
            g_vpr_ctx.mutable_atom().lookup.set_atom_clb_net(net, ClusterNetId::INVALID());
        }

        ++pack_iteration;
    }

    /*free list_of_pack_molecules*/
    free_list_of_pack_patterns(list_of_packing_patterns, num_packing_patterns);

    cur_pack_molecule = list_of_pack_molecules;
    while (cur_pack_molecule != nullptr) {
        cur_pack_molecule = list_of_pack_molecules->next;
        delete list_of_pack_molecules;
        list_of_pack_molecules = cur_pack_molecule;
    }

    VTR_LOG("\n");
    VTR_LOG("Netlist conversion complete.\n");
    VTR_LOG("\n");

    return true;
}

float get_arch_switch_info(short switch_index, int switch_fanin, float& Tdel_switch, float& R_switch, float& Cout_switch) {
    /* Fetches delay, resistance and output capacitance of the architecture switch at switch_index.
     * Returns the total delay through the switch. Used to calculate inter-cluster net delay. */

    /* The intrinsic delay may depend on fanin to the switch. If the delay map of a
     * switch from the architecture file has multiple (#inputs, delay) entries, we
     * interpolate/extrapolate to get the delay at 'switch_fanin'. */
    auto& device_ctx = g_vpr_ctx.device();

    Tdel_switch = device_ctx.arch_switch_inf[switch_index].Tdel(switch_fanin);
    R_switch = device_ctx.arch_switch_inf[switch_index].R;
    Cout_switch = device_ctx.arch_switch_inf[switch_index].Cout;

    /* The delay through a loaded switch is its intrinsic (unloaded)
     * delay plus the product of its resistance and output capacitance. */
    return Tdel_switch + R_switch * Cout_switch;
}

std::unordered_set<AtomNetId> alloc_and_load_is_clock(bool global_clocks) {
    /* Looks through all the atom blocks to find and mark all the clocks, by setting
     * the corresponding entry by adding the clock to is_clock.
     * global_clocks is used
     * only for an error check.                                                */

    int num_clocks = 0;
    std::unordered_set<AtomNetId> is_clock;

    /* Want to identify all the clock nets.  */
    auto& atom_ctx = g_vpr_ctx.atom();

    for (auto blk_id : atom_ctx.nlist.blocks()) {
        for (auto pin_id : atom_ctx.nlist.block_clock_pins(blk_id)) {
            auto net_id = atom_ctx.nlist.pin_net(pin_id);
            if (!is_clock.count(net_id)) {
                is_clock.insert(net_id);
                num_clocks++;
            }
        }
    }

    /* If we have multiple clocks and we're supposed to declare them global, *
     * print a warning message, since it looks like this circuit may have    *
     * locally generated clocks.                                             */

    if (num_clocks > 1 && global_clocks) {
        VTR_LOG_WARN(
            "All %d clocks will be treated as global.\n", num_clocks);
    }

    return (is_clock);
}

static bool try_size_device_grid(const t_arch& arch, const std::map<t_type_ptr, size_t>& num_type_instances, float target_device_utilization, std::string device_layout_name) {
    auto& device_ctx = g_vpr_ctx.mutable_device();

    //Build the device
    auto grid = create_device_grid(device_layout_name, arch.grid_layouts, num_type_instances, target_device_utilization);

    /*
     *Report on the device
     */
    VTR_LOG("FPGA sized to %zu x %zu (%s)\n", grid.width(), grid.height(), grid.name().c_str());

    bool fits_on_device = true;

    float device_utilization = calculate_device_utilization(grid, num_type_instances);
    VTR_LOG("Device Utilization: %.2f (target %.2f)\n", device_utilization, target_device_utilization);
    std::map<t_type_ptr, float> type_util;
    for (int i = 0; i < device_ctx.num_block_types; ++i) {
        auto type = &device_ctx.block_types[i];
        auto itr = num_type_instances.find(type);
        if (itr == num_type_instances.end()) continue;

        float num_instances = itr->second;
        float util = 0.;
        if (num_instances != 0) {
            util = num_instances / device_ctx.grid.num_instances(type);
        }
        type_util[type] = util;

        if (util > 1.) {
            fits_on_device = false;
        }
        VTR_LOG("\tBlock Utilization: %.2f Type: %s\n", util, type->name);
    }
    VTR_LOG("\n");

    return fits_on_device;
}

static t_ext_pin_util_targets parse_target_external_pin_util(std::vector<std::string> specs) {
    t_ext_pin_util_targets targets(1., 1.);

    if (specs.size() == 1 && specs[0] == "auto") {
        //No user-specified pin utilizations, infer them automatically.
        //
        //We set a pin utilization target based on the block type, with
        //the logic block having a lower utilization target and other blocks
        //(e.g. hard blocks) having no limit.

        auto& device_ctx = g_vpr_ctx.device();
        auto& grid = device_ctx.grid;
        t_type_ptr logic_block_type = infer_logic_block_type(grid);

        //Allowing 100% pin utilization of the logic block type can harm
        //routability, since it may allow a few (typically outlier) clusters to
        //use a very large number of pins -- causing routability issues. These
        //clusters can cause failed routings where only a handful of routing
        //resource nodes remain overused (and do not resolve) These can be
        //avoided by putting a (soft) limit on the number of input pins which
        //can be used, effectively clipping off the most egregeous outliers.
        //
        //Experiments show that limiting input utilization produces better quality
        //than limiting output utilization (limiting input utilization implicitly
        //also limits output utilization).
        //
        //For relatively high pin utilizations (e.g. > 70%) this has little-to-no
        //impact on the number of clusters required. As a result we set a default
        //input pin utilization target which is high, but less than 100%.
        if (logic_block_type != nullptr) {
            constexpr float LOGIC_BLOCK_TYPE_AUTO_INPUT_UTIL = 0.8;
            constexpr float LOGIC_BLOCK_TYPE_AUTO_OUTPUT_UTIL = 1.0;

            t_ext_pin_util logic_block_ext_pin_util(LOGIC_BLOCK_TYPE_AUTO_INPUT_UTIL, LOGIC_BLOCK_TYPE_AUTO_OUTPUT_UTIL);

            targets.set_block_pin_util(logic_block_type->name, logic_block_ext_pin_util);
        } else {
            VTR_LOG_WARN("Unable to identify logic block type to apply default pin utilization targets to; this may result in denser packing than desired\n");
        }

    } else {
        //Process user specified overrides

        bool default_set = false;
        std::set<std::string> seen_block_types;

        for (auto spec : specs) {
            t_ext_pin_util target_ext_pin_util(1., 1.);

            auto block_values = vtr::split(spec, ":");
            std::string block_type;
            std::string values;
            if (block_values.size() == 2) {
                block_type = block_values[0];
                values = block_values[1];
            } else if (block_values.size() == 1) {
                values = block_values[0];
            } else {
                std::stringstream msg;
                msg << "In valid block pin utilization specification '" << spec << "' (expected at most one ':' between block name and values";
                VPR_THROW(VPR_ERROR_PACK, msg.str().c_str());
            }

            auto elements = vtr::split(values, ",");
            if (elements.size() == 1) {
                target_ext_pin_util.input_pin_util = vtr::atof(elements[0]);
            } else if (elements.size() == 2) {
                target_ext_pin_util.input_pin_util = vtr::atof(elements[0]);
                target_ext_pin_util.output_pin_util = vtr::atof(elements[1]);
            } else {
                std::stringstream msg;
                msg << "Invalid conversion from '" << spec << "' to external pin util (expected either a single float value, or two float values separted by a comma)";
                VPR_THROW(VPR_ERROR_PACK, msg.str().c_str());
            }

            if (target_ext_pin_util.input_pin_util < 0. || target_ext_pin_util.input_pin_util > 1.) {
                std::stringstream msg;
                msg << "Out of range target input pin utilization '" << target_ext_pin_util.input_pin_util << "' (expected within range [0.0, 1.0])";
                VPR_THROW(VPR_ERROR_PACK, msg.str().c_str());
            }
            if (target_ext_pin_util.output_pin_util < 0. || target_ext_pin_util.output_pin_util > 1.) {
                std::stringstream msg;
                msg << "Out of range target output pin utilization '" << target_ext_pin_util.output_pin_util << "' (expected within range [0.0, 1.0])";
                VPR_THROW(VPR_ERROR_PACK, msg.str().c_str());
            }

            if (block_type.empty()) {
                //Default value
                if (default_set) {
                    std::stringstream msg;
                    msg << "Only one default pin utilization should be specified";
                    VPR_THROW(VPR_ERROR_PACK, msg.str().c_str());
                }
                targets.set_default_pin_util(target_ext_pin_util);
                default_set = true;
            } else {
                if (seen_block_types.count(block_type)) {
                    std::stringstream msg;
                    msg << "Only one pin utilization should be specified for block type '" << block_type << "'";
                    VPR_THROW(VPR_ERROR_PACK, msg.str().c_str());
                }

                targets.set_block_pin_util(block_type, target_ext_pin_util);
                seen_block_types.insert(block_type);
            }
        }
    }

    return targets;
}

static std::string target_external_pin_util_to_string(const t_ext_pin_util_targets& ext_pin_utils) {
    std::stringstream ss;

    auto& device_ctx = g_vpr_ctx.device();

    for (int itype = 0; itype < device_ctx.num_block_types; ++itype) {
        if (is_empty_type(&device_ctx.block_types[itype])) continue;

        auto blk_name = device_ctx.block_types[itype].name;

        ss << blk_name << ":";

        auto pin_util = ext_pin_utils.get_pin_util(blk_name);
        ss << pin_util.input_pin_util << ',' << pin_util.output_pin_util;

        if (itype != device_ctx.num_block_types - 1) {
            ss << " ";
        }
    }

    return ss.str();
}

static t_pack_high_fanout_thresholds parse_high_fanout_thresholds(std::vector<std::string> specs) {
    t_pack_high_fanout_thresholds high_fanout_thresholds(128);

    if (specs.size() == 1 && specs[0] == "auto") {
        //No user-specified high fanout thresholds, infer them automatically.
        //
        //We set the high fanout threshold a based on the block type, with
        //the logic block having a lower threshold than other blocks.
        //(Since logic blocks are the ones which tend to be too densely
        //clustered.)

        auto& device_ctx = g_vpr_ctx.device();
        auto& grid = device_ctx.grid;
        t_type_ptr logic_block_type = infer_logic_block_type(grid);

        if (logic_block_type != nullptr) {
            constexpr float LOGIC_BLOCK_TYPE_HIGH_FANOUT_THRESHOLD = 32;

            high_fanout_thresholds.set(logic_block_type->name, LOGIC_BLOCK_TYPE_HIGH_FANOUT_THRESHOLD);
        } else {
            VTR_LOG_WARN("Unable to identify logic block type to apply default packer high fanout thresholds; this may result in denser packing than desired\n");
        }
    } else {
        //Process user specified overrides

        bool default_set = false;
        std::set<std::string> seen_block_types;

        for (auto spec : specs) {
            auto block_values = vtr::split(spec, ":");
            std::string block_type;
            std::string value;
            if (block_values.size() == 1) {
                value = block_values[0];
            } else if (block_values.size() == 2) {
                block_type = block_values[0];
                value = block_values[1];
            } else {
                std::stringstream msg;
                msg << "In valid block high fanout threshold specification '" << spec << "' (expected at most one ':' between block name and value";
                VPR_THROW(VPR_ERROR_PACK, msg.str().c_str());
            }

            int threshold = vtr::atoi(value);

            if (block_type.empty()) {
                //Default value
                if (default_set) {
                    std::stringstream msg;
                    msg << "Only one default high fanout threshold should be specified";
                    VPR_THROW(VPR_ERROR_PACK, msg.str().c_str());
                }
                high_fanout_thresholds.set_default(threshold);
                default_set = true;
            } else {
                if (seen_block_types.count(block_type)) {
                    std::stringstream msg;
                    msg << "Only one high fanout threshold should be specified for block type '" << block_type << "'";
                    VPR_THROW(VPR_ERROR_PACK, msg.str().c_str());
                }

                high_fanout_thresholds.set(block_type, threshold);
                seen_block_types.insert(block_type);
            }
        }
    }

    return high_fanout_thresholds;
}

static std::string high_fanout_thresholds_to_string(const t_pack_high_fanout_thresholds& hf_thresholds) {
    std::stringstream ss;

    auto& device_ctx = g_vpr_ctx.device();

    for (int itype = 0; itype < device_ctx.num_block_types; ++itype) {
        if (is_empty_type(&device_ctx.block_types[itype])) continue;

        auto blk_name = device_ctx.block_types[itype].name;

        ss << blk_name << ":";

        auto threshold = hf_thresholds.get_threshold(blk_name);
        ss << threshold;

        if (itype != device_ctx.num_block_types - 1) {
            ss << " ";
        }
    }

    return ss.str();
}
