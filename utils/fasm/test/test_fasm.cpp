#include "catch.hpp"

#include "vpr_api.h"
#include "vtr_util.h"
#include "rr_metadata.h"
#include "fasm.h"
#include "arch_util.h"
#include "rr_graph_writer.h"
#include <sstream>
#include <fstream>

static constexpr const char kArchFile[] = "test_fasm_arch.xml";
static constexpr const char kRrGraphFile[] = "test_fasm_rrgraph.xml";

namespace {

using Catch::Matchers::Equals;

TEST_CASE("fasm_integration_test", "[fasm]") {
    {
        t_vpr_setup vpr_setup;
        t_arch arch;
        t_options options;
        const char *argv[] = {
            "test_vpr",
            kArchFile,
            "wire.eblif",
            "--route_chan_width",
            "100",
        };
        vpr_init(sizeof(argv)/sizeof(argv[0]), argv,
                &options, &vpr_setup, &arch);
        bool flow_succeeded = vpr_flow(vpr_setup, arch);
        REQUIRE(flow_succeeded == true);

        auto &device_ctx = g_vpr_ctx.mutable_device();
        for(size_t inode = 0; inode < device_ctx.rr_nodes.size(); ++inode) {
            for(t_edge_size iedge = 0; iedge < device_ctx.rr_nodes[inode].num_edges(); ++iedge) {
                auto sink_inode = device_ctx.rr_nodes[inode].edge_sink_node(iedge);
                auto switch_id = device_ctx.rr_nodes[inode].edge_switch(iedge);
                vpr::add_rr_edge_metadata(inode, sink_inode, switch_id,
                        "fasm_features", vtr::string_fmt("%d_%d_%zu",
                            inode, sink_inode, switch_id));
            }
        }

        write_rr_graph(kRrGraphFile, vpr_setup.Segments);
        vpr_free_all(arch, vpr_setup);
    }

    t_vpr_setup vpr_setup;
    t_arch arch;
    t_options options;
    const char *argv[] = {
        "test_vpr",
        kArchFile,
        "wire.eblif",
        "--route_chan_width",
        "100",
        "--read_rr_graph",
        kRrGraphFile,
    };

    vpr_init(sizeof(argv)/sizeof(argv[0]), argv,
              &options, &vpr_setup, &arch);

    vpr_setup.PackerOpts.doPacking    = STAGE_LOAD;
    vpr_setup.PlacerOpts.doPlacement  = STAGE_LOAD;
    vpr_setup.RouterOpts.doRouting    = STAGE_LOAD;
    vpr_setup.AnalysisOpts.doAnalysis = STAGE_SKIP;

    bool flow_succeeded = vpr_flow(vpr_setup, arch);
    REQUIRE(flow_succeeded == true);

    std::stringstream fasm_string;
    fasm::FasmWriterVisitor visitor(fasm_string);
    NetlistWalker nl_walker(visitor);
    nl_walker.walk();

    fasm_string.seekg(0);

    std::ofstream fasm_file("output.fasm", std::ios_base::out);
    while(fasm_string) {
        std::string line;
        std::getline(fasm_string, line);
        fasm_file << line << std::endl;
    }

    fasm_string.clear();
    fasm_string.seekg(0);

    std::set<std::tuple<int, int, short>> routing_edges;
    bool found_lut5 = false;
    bool found_lut6 = false;
    bool found_mux1 = false;
    bool found_mux2 = false;
    bool found_mux3 = false;
    while(fasm_string) {
        // Should see something like:
        // CLB.FLE0.N2_LUT5
        // CLB.FLE8.LUT5_1.LUT[31:0]=32'b00000000000000010000000000000000
        // CLB.FLE9.OUT_MUX.LUT
        // CLB.FLE9.DISABLE_FF
        // CLB.FLE9.LUT6[63:0]=64'b0000000000000000000000000000000100000000000000000000000000000000
        // 3634_3690_0
        std::string line;
        std::getline(fasm_string, line);

        if(line == "") {
            continue;
        }

        if(!found_mux1 && line.find(".OUT_MUX.LUT") != std::string::npos) {
            found_mux1 = true;
        }
        if(!found_mux2 && line.find(".DISABLE_FF") != std::string::npos) {
            found_mux2 = true;
        }
        if(!found_mux3 && line.find(".IN0") != std::string::npos) {
            found_mux3 = true;
        }

        if(line.find("CLB") != std::string::npos) {
            auto pos = line.find("LUT[");
            if(pos != std::string::npos) {
                CHECK_THAT(line.substr(pos), Equals(
                           "LUT[31:0]=32'b00000000000000010000000000000000"));
                found_lut5 = true;
            }

            pos = line.find("LUT6[");
            if(pos != std::string::npos) {
                CHECK_THAT(line.substr(pos), Equals("LUT6[63:0]=64'b0000000000000000000000000000000100000000000000000000000000000000"));
                found_lut6 = true;
            }
        } else {
            auto parts = vtr::split(line, "_");
            REQUIRE(parts.size() == 3);
            auto src_inode = vtr::atoi(parts[0]);
            auto sink_inode = vtr::atoi(parts[1]);
            auto switch_id = vtr::atoi(parts[2]);

            auto ret = routing_edges.insert(std::make_tuple(src_inode, sink_inode, switch_id));
            CHECK(ret.second == true);
        }
    }

    const auto & route_ctx = g_vpr_ctx.routing();
    for(const auto &trace : route_ctx.trace) {
        const t_trace *head = trace.head;
        while(head != nullptr) {
            const t_trace *next = head->next;

            if(next != nullptr) {
                const auto next_inode = next->index;
                auto iter = routing_edges.find(std::make_tuple(head->index, next_inode, head->iswitch));
                CHECK(iter != routing_edges.end());
            }

            head = next;
        }
    }

    CHECK(found_lut5);
    CHECK(found_lut6);
    CHECK(found_mux1);
    CHECK(found_mux2);
    CHECK(found_mux3);

    vpr_free_all(arch, vpr_setup);
}

} // namespace
