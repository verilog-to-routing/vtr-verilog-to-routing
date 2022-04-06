#include "catch2/catch_test_macros.hpp"

#include "vpr_api.h"
#include "timing_place_lookup.h"

#include <fstream>

namespace {

static constexpr const char kArchFile[] = "test_post_verilog_arch.xml";
static constexpr const char kCircuitFile[] = "unconnected.eblif";

void do_vpr_flow(const char* input_unc_opt, const char* output_unc_opt) {
    // Minimal setup
    auto options = t_options();
    auto arch = t_arch();
    auto vpr_setup = t_vpr_setup();

    // Command line arguments
    const char* argv[] = {
        "test_vpr",
        kArchFile,
        kCircuitFile,
        "--route_chan_width", "100",
        "--gen_post_synthesis_netlist", "on",
        "--post_synth_netlist_unconn_inputs", input_unc_opt,
        "--post_synth_netlist_unconn_outputs", output_unc_opt};

    vpr_init(sizeof(argv) / sizeof(argv[0]), argv,
             &options, &vpr_setup, &arch);

    bool flow_succeeded = vpr_flow(vpr_setup, arch);

    free_routing_structs();
    vpr_free_all(arch, vpr_setup);

    REQUIRE(flow_succeeded == true);
}

void compare_files(const std::string& output_fname, const std::string& golden_fname) {
    printf("Comparing '%s' vs. '%s'\n", output_fname.c_str(), golden_fname.c_str());

    std::ifstream output_file(output_fname);
    std::ifstream golden_file(golden_fname);

    REQUIRE(output_file.good());
    REQUIRE(golden_file.good());

    auto read_lines = [](std::ifstream& fp) {
        std::vector<std::string> lines;
        while (fp.good()) {
            std::string line;
            std::getline(fp, line);
            lines.push_back(line);
        }

        return lines;
    };

    auto output_data = read_lines(output_file);
    auto golden_data = read_lines(golden_file);

    REQUIRE(!output_data.empty());
    REQUIRE(!golden_data.empty());
    REQUIRE(output_data.size() == golden_data.size());

    // Skip the first line as it contains a comment with build SHA
    output_data.erase(output_data.begin());
    golden_data.erase(golden_data.begin());

    REQUIRE(output_data == golden_data);
}

TEST_CASE("post_verilog", "[vpr]") {
    do_vpr_flow("unconnected", "unconnected");
    compare_files("unconnected_post_synthesis.v", "test_post_verilog_i_unconnected_o_unconnected.golden.v");

    do_vpr_flow("unconnected", "nets");
    compare_files("unconnected_post_synthesis.v", "test_post_verilog_i_unconnected_o_nets.golden.v");

    do_vpr_flow("vcc", "unconnected");
    compare_files("unconnected_post_synthesis.v", "test_post_verilog_i_vcc_o_unconnected.golden.v");

    do_vpr_flow("gnd", "unconnected");
    compare_files("unconnected_post_synthesis.v", "test_post_verilog_i_gnd_o_unconnected.golden.v");

    do_vpr_flow("nets", "unconnected");
    compare_files("unconnected_post_synthesis.v", "test_post_verilog_i_nets_o_unconnected.golden.v");
}

} // namespace
