#include "read_circuit.h"
#include "read_blif.h"
#include "read_interchange_netlist.h"
#include "atom_netlist.h"
#include "atom_netlist_utils.h"
#include "echo_files.h"
#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_path.h"
#include "vtr_time.h"

static void process_circuit(AtomNetlist& netlist,
                            e_const_gen_inference const_gen_inference_method,
                            bool should_absorb_buffers,
                            bool should_sweep_dangling_primary_ios,
                            bool should_sweep_dangling_nets,
                            bool should_sweep_dangling_blocks,
                            bool should_sweep_constant_primary_outputs,
                            int verbosity);

static void show_circuit_stats(const AtomNetlist& netlist);

AtomNetlist read_and_process_circuit(e_circuit_format circuit_format, t_vpr_setup& vpr_setup, t_arch& arch) {
    // Options
    const char* circuit_file = vpr_setup.PackerOpts.circuit_file_name.c_str();
    const t_model* user_models = vpr_setup.user_models;
    const t_model* library_models = vpr_setup.library_models;
    e_const_gen_inference const_gen_inference = vpr_setup.NetlistOpts.const_gen_inference;
    bool should_absorb_buffers = vpr_setup.NetlistOpts.absorb_buffer_luts;
    bool should_sweep_dangling_primary_ios = vpr_setup.NetlistOpts.sweep_dangling_primary_ios;
    bool should_sweep_dangling_nets = vpr_setup.NetlistOpts.sweep_dangling_nets;
    bool should_sweep_dangling_blocks = vpr_setup.NetlistOpts.sweep_dangling_blocks;
    bool should_sweep_constant_primary_outputs = vpr_setup.NetlistOpts.sweep_constant_primary_outputs;
    int verbosity = vpr_setup.NetlistOpts.netlist_verbosity;

    if (circuit_format == e_circuit_format::AUTO) {
        auto name_ext = vtr::split_ext(circuit_file);

        VTR_LOGV(verbosity, "Circuit file: %s\n", circuit_file);
        if (name_ext[1] == ".blif") {
            circuit_format = e_circuit_format::BLIF;
        } else if (name_ext[1] == ".eblif") {
            circuit_format = e_circuit_format::EBLIF;
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ATOM_NETLIST, "Failed to determine file format for '%s' expected .blif or .eblif extension",
                            circuit_file);
        }
    }

    AtomNetlist netlist;
    {
        vtr::ScopedStartFinishTimer t("Load circuit");

        switch (circuit_format) {
            case e_circuit_format::BLIF:
            case e_circuit_format::EBLIF:
                netlist = read_blif(circuit_format, circuit_file, user_models, library_models);
                break;
            case e_circuit_format::FPGA_INTERCHANGE:
                netlist = read_interchange_netlist(circuit_file, arch);
                break;
            default:
                VPR_FATAL_ERROR(VPR_ERROR_ATOM_NETLIST,
                                "Unable to identify circuit file format for '%s'. Expect [blif|eblif|fpga-interchange]!\n",
                                circuit_file);
                break;
        }
    }

    if (isEchoFileEnabled(E_ECHO_ATOM_NETLIST_ORIG)) {
        print_netlist_as_blif(getEchoFileName(E_ECHO_ATOM_NETLIST_ORIG), netlist);
    }

    process_circuit(netlist,
                    const_gen_inference,
                    should_absorb_buffers,
                    should_sweep_dangling_primary_ios,
                    should_sweep_dangling_nets,
                    should_sweep_dangling_blocks,
                    should_sweep_constant_primary_outputs,
                    verbosity);

    if (isEchoFileEnabled(E_ECHO_ATOM_NETLIST_CLEANED)) {
        print_netlist_as_blif(getEchoFileName(E_ECHO_ATOM_NETLIST_CLEANED), netlist);
    }

    show_circuit_stats(netlist);

    return netlist;
}

static void process_circuit(AtomNetlist& netlist,
                            e_const_gen_inference const_gen_inference_method,
                            bool should_absorb_buffers,
                            bool should_sweep_dangling_primary_ios,
                            bool should_sweep_dangling_nets,
                            bool should_sweep_dangling_blocks,
                            bool should_sweep_constant_primary_outputs,
                            int verbosity) {
    {
        vtr::ScopedStartFinishTimer t("Clean circuit");

        //Clean-up lut buffers
        if (should_absorb_buffers) {
            absorb_buffer_luts(netlist, verbosity);
        }

        //Remove the special 'unconn' net
        AtomNetId unconn_net_id = netlist.find_net("unconn");
        if (unconn_net_id) {
            VTR_LOGV_WARN(verbosity > 1, "Removing special net 'unconn' (assumed it represented explicitly unconnected pins)\n");
            netlist.remove_net(unconn_net_id);
        }

        //Also remove the 'unconn' block driver, if it exists
        AtomBlockId unconn_blk_id = netlist.find_block("unconn");
        if (unconn_blk_id) {
            VTR_LOGV_WARN(verbosity > 1, "Removing special block 'unconn' (assumed it represented explicitly unconnected pins)\n");
            netlist.remove_block(unconn_blk_id);
        }

        //Sweep unused logic/nets/inputs/outputs
        sweep_iterative(netlist,
                        should_sweep_dangling_primary_ios,
                        should_sweep_dangling_nets,
                        should_sweep_dangling_blocks,
                        should_sweep_constant_primary_outputs,
                        const_gen_inference_method,
                        verbosity);
    }

    {
        vtr::ScopedStartFinishTimer t("Compress circuit");

        //Compress the netlist to clean-out invalid entries
        netlist.remove_and_compress();
    }
    {
        vtr::ScopedStartFinishTimer t("Verify circuit");

        netlist.verify();
    }
}

static void show_circuit_stats(const AtomNetlist& netlist) {
    // Count the block statistics
    std::map<std::string, size_t> block_type_counts;
    std::map<std::string, size_t> lut_size_counts;
    for (auto blk_id : netlist.blocks()) {
        // For each model, count the number of occurrences in the netlist.
        const t_model* blk_model = netlist.block_model(blk_id);
        ++block_type_counts[blk_model->name];
        // If this block is a LUT, also count the occurences of this size of LUT
        // for more logging information.
        if (blk_model->name == std::string(MODEL_NAMES)) {
            // May have zero (no input LUT) or one input port
            auto in_ports = netlist.block_input_ports(blk_id);
            VTR_ASSERT(in_ports.size() <= 1 && "Expected number of input ports for LUT to be 0 or 1");
            size_t lut_size = 0;
            if (in_ports.size() == 1) {
                // Use the number of pins in the input port to determine the
                // size of the LUT.
                auto port_id = *in_ports.begin();
                lut_size = netlist.port_pins(port_id).size();
            }
            ++lut_size_counts[std::to_string(lut_size) + "-LUT"];
        }
    }

    // Count the net statistics
    std::map<std::string, double> net_stats;
    for (auto net_id : netlist.nets()) {
        double fanout = netlist.net_sinks(net_id).size();

        net_stats["Max Fanout"] = std::max(net_stats["Max Fanout"], fanout);

        if (net_stats.count("Min Fanout")) {
            net_stats["Min Fanout"] = std::min(net_stats["Min Fanout"], fanout);
        } else {
            net_stats["Min Fanout"] = fanout;
        }

        net_stats["Avg Fanout"] += fanout;
    }
    net_stats["Avg Fanout"] /= netlist.nets().size();

    // Determine the maximum length of a type name for nice formatting
    size_t max_block_type_len = 0;
    for (const auto& kv : block_type_counts) {
        max_block_type_len = std::max(max_block_type_len, kv.first.size());
    }
    size_t max_lut_size_len = 0;
    for (const auto& kv : lut_size_counts) {
        max_lut_size_len = std::max(max_lut_size_len, kv.first.size());
    }
    size_t max_net_type_len = 0;
    for (const auto& kv : net_stats) {
        max_net_type_len = std::max(max_net_type_len, kv.first.size());
    }

    // Print the statistics
    VTR_LOG("Circuit Statistics:\n");
    VTR_LOG("  Blocks: %zu\n", netlist.blocks().size());
    for (const auto& kv : block_type_counts) {
        VTR_LOG("    %-*s: %7zu\n", max_block_type_len, kv.first.c_str(), kv.second);
        // If this block is a LUT, print the different sizes of LUTs in the
        // design.
        if (kv.first == std::string(MODEL_NAMES)) {
            for (const auto& lut_kv : lut_size_counts) {
                VTR_LOG("        %-*s: %7zu\n", max_lut_size_len, lut_kv.first.c_str(), lut_kv.second);
            }
        }
    }
    VTR_LOG("  Nets  : %zu\n", netlist.nets().size());
    for (const auto& kv : net_stats) {
        VTR_LOG("    %-*s: %7.1f\n", max_net_type_len, kv.first.c_str(), kv.second);
    }
    VTR_LOG("  Netlist Clocks: %zu\n", find_netlist_logical_clock_drivers(netlist).size());

    if (netlist.blocks().empty()) {
        VTR_LOG_WARN("Netlist contains no blocks\n");
    }
}
