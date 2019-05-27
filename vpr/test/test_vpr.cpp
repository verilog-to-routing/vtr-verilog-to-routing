#include "catch.hpp"

#include "read_xml_arch_file.h"
#include "rr_metadata.h"
#include "rr_graph_writer.h"
#include "arch_util.h"
#include "vpr_api.h"
#include <cstring>

namespace {

using Catch::Matchers::Equals;

static constexpr const char kArchFile[] = "test_read_arch_metadata.xml";
static constexpr const char kRrGraphFile[] = "test_read_rrgraph_metadata.xml";

TEST_CASE("read_arch_metadata", "[vpr]") {
    t_arch arch;
    t_type_descriptor* types;
    int num_types;

    XmlReadArch(kArchFile, /*timing_enabled=*/false,
                &arch, &types, &num_types);

    bool found_perimeter_meta = false;
    bool found_single_meta = false;
    for (const auto& grid_def : arch.grid_layouts) {
        for (const auto& loc_def : grid_def.loc_defs) {
            if (loc_def.block_type == "io") {
                REQUIRE(loc_def.meta != nullptr);
                REQUIRE(loc_def.meta->has("type"));
                CHECK_THAT(loc_def.meta->one("type")->as_string(), Equals("io"));
                found_perimeter_meta = true;
            }

            if (loc_def.block_type == "clb" && loc_def.x.start_expr == "5" && loc_def.y.start_expr == "5") {
                REQUIRE(loc_def.meta != nullptr);
                REQUIRE(loc_def.meta->has("single"));
                CHECK_THAT(loc_def.meta->one("single")->as_string(), Equals("clb"));
                found_single_meta = true;
            }
        }
    }

    CHECK(found_perimeter_meta);
    CHECK(found_single_meta);

    bool found_pb_type = false;
    bool found_mode = false;
    bool found_direct = false;

    for (int i = 0; i < num_types; ++i) {
        if (strcmp("io", types[i].name) == 0) {
            found_pb_type = true;
            REQUIRE(types[i].pb_type != nullptr);
            REQUIRE(types[i].pb_type->meta.has("pb_type_type"));
            CHECK_THAT(types[i].pb_type->meta.one("pb_type_type")->as_string(), Equals("pb_type = io"));

            REQUIRE(types[i].pb_type->num_modes > 0);
            REQUIRE(types[i].pb_type->modes != nullptr);

            for (int imode = 0; imode < types[i].pb_type->num_modes; ++imode) {
                if (strcmp("inpad", types[i].pb_type->modes[imode].name) == 0) {
                    found_mode = true;
                    const auto* mode = &types[i].pb_type->modes[imode];

                    REQUIRE(mode->meta.has("mode"));
                    CHECK_THAT(mode->meta.one("mode")->as_string(), Equals("inpad"));

                    CHECK(mode->num_interconnect > 0);
                    REQUIRE(mode->interconnect != nullptr);

                    for (int iint = 0; iint < mode->num_interconnect; ++iint) {
                        if (strcmp("inpad", mode->interconnect[iint].name) == 0) {
                            found_direct = true;
                            REQUIRE(mode->interconnect[iint].meta.has("interconnect"));
                            CHECK_THAT(mode->interconnect[iint].meta.one("interconnect")->as_string(), Equals("inpad_iconnect"));
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
    }

    CHECK(found_pb_type);
    CHECK(found_mode);
    CHECK(found_direct);

    free_type_descriptors(types, num_types);
    free_arch(&arch);
}

TEST_CASE("read_rr_graph_metadata", "[vpr]") {
    int src_inode = -1;
    int sink_inode = -1;
    short switch_id = -1;

    {
        t_vpr_setup vpr_setup;
        t_arch arch;
        t_options options;
        const char* argv[] = {
            "test_vpr",
            kArchFile,
            "wire.eblif",
            "--route_chan_width",
            "100",
        };
        vpr_init(sizeof(argv) / sizeof(argv[0]), argv,
                 &options, &vpr_setup, &arch);
        vpr_create_device(vpr_setup, arch);

        const auto& device_ctx = g_vpr_ctx.device();

        for (int inode = 0; inode < (int)device_ctx.rr_nodes.size(); ++inode) {
            if ((device_ctx.rr_nodes[inode].type() == CHANX || device_ctx.rr_nodes[inode].type() == CHANY) && device_ctx.rr_nodes[inode].num_edges() > 0) {
                src_inode = inode;
                break;
            }
        }

        REQUIRE(src_inode != -1);
        sink_inode = device_ctx.rr_nodes[src_inode].edge_sink_node(0);
        switch_id = device_ctx.rr_nodes[src_inode].edge_switch(0);

        vpr::add_rr_node_metadata(src_inode, "node", "test node");
        vpr::add_rr_edge_metadata(src_inode, sink_inode, switch_id, "edge", "test edge");

        write_rr_graph(kRrGraphFile, vpr_setup.Segments);
        vpr_free_all(arch, vpr_setup);
    }

    REQUIRE(src_inode != -1);
    REQUIRE(sink_inode != -1);
    REQUIRE(switch_id != -1);

    t_vpr_setup vpr_setup;
    t_arch arch;
    t_options options;
    const char* argv[] = {
        "test_vpr",
        kArchFile,
        "wire.eblif",
        "--route_chan_width",
        "100",
        "--read_rr_graph",
        kRrGraphFile,
    };

    vpr_init(sizeof(argv) / sizeof(argv[0]), argv,
             &options, &vpr_setup, &arch);
    vpr_create_device(vpr_setup, arch);

    const auto& device_ctx = g_vpr_ctx.device();
    CHECK(device_ctx.rr_node_metadata.size() == 1);
    CHECK(device_ctx.rr_edge_metadata.size() == 1);

    for (const auto& node_meta : device_ctx.rr_node_metadata) {
        CHECK(node_meta.first == src_inode);
        REQUIRE(node_meta.second.has("node"));
        REQUIRE(node_meta.second.one("node") != nullptr);
        CHECK_THAT(node_meta.second.one("node")->as_string(), Equals("test node"));
    }

    for (const auto& edge_meta : device_ctx.rr_edge_metadata) {
        CHECK(std::get<0>(edge_meta.first) == src_inode);
        CHECK(std::get<1>(edge_meta.first) == sink_inode);
        CHECK(std::get<2>(edge_meta.first) == switch_id);

        REQUIRE(edge_meta.second.has("edge"));
        REQUIRE(edge_meta.second.one("edge") != nullptr);
        CHECK_THAT(edge_meta.second.one("edge")->as_string(), Equals("test edge"));
    }
    vpr_free_all(arch, vpr_setup);
}

} // namespace
