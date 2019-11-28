#include "catch.hpp"

#include "read_xml_arch_file.h"
#include "rr_metadata.h"
#include "rr_graph_writer.h"
#include "arch_util.h"
#include "vpr_api.h"
#include <cstring>
#include <vector>

namespace {

using Catch::Matchers::Equals;

static constexpr const char kArchFile[] = "test_read_arch_metadata.xml";
static constexpr const char kRrGraphFile[] = "test_read_rrgraph_metadata.xml";

TEST_CASE("read_arch_metadata", "[vpr]") {
    t_arch arch;
    std::vector<t_physical_tile_type> physical_tile_types;
    std::vector<t_logical_block_type> logical_block_types;

    XmlReadArch(kArchFile, /*timing_enabled=*/false,
                &arch, physical_tile_types, logical_block_types);

    bool found_perimeter_meta = false;
    bool found_single_meta = false;
    for (const auto& grid_def : arch.grid_layouts) {
        for (const auto& loc_def : grid_def.loc_defs) {
            if (loc_def.block_type == "io") {
                REQUIRE(loc_def.meta != nullptr);
                REQUIRE(loc_def.meta->has("type"));
                auto* value = loc_def.meta->one("type");
                REQUIRE(value != nullptr);
                CHECK_THAT(value->as_string(), Equals("io"));
                found_perimeter_meta = true;
            }

            if (loc_def.block_type == "clb" && loc_def.x.start_expr == "5" && loc_def.y.start_expr == "5") {
                REQUIRE(loc_def.meta != nullptr);
                REQUIRE(loc_def.meta->has("single"));
                auto* value = loc_def.meta->one("single");
                REQUIRE(value != nullptr);
                CHECK_THAT(value->as_string(), Equals("clb"));
                found_single_meta = true;
            }
        }
    }

    CHECK(found_perimeter_meta);
    CHECK(found_single_meta);

    bool found_pb_type = false;
    bool found_mode = false;
    bool found_direct = false;

    for (const auto& type : logical_block_types) {
        if (strcmp("io", type.name) == 0) {
            found_pb_type = true;
            REQUIRE(type.pb_type != nullptr);
            REQUIRE(type.pb_type->meta.has("pb_type_type"));
            auto* pb_type_value = type.pb_type->meta.one("pb_type_type");
            REQUIRE(pb_type_value != nullptr);
            CHECK_THAT(pb_type_value->as_string(), Equals("pb_type = io"));

            REQUIRE(type.pb_type->num_modes > 0);
            REQUIRE(type.pb_type->modes != nullptr);

            for (int imode = 0; imode < type.pb_type->num_modes; ++imode) {
                if (strcmp("inpad", type.pb_type->modes[imode].name) == 0) {
                    found_mode = true;
                    const auto* mode = &type.pb_type->modes[imode];

                    REQUIRE(mode->meta.has("mode"));
                    auto* mode_value = mode->meta.one("mode");
                    REQUIRE(mode_value != nullptr);
                    CHECK_THAT(mode_value->as_string(), Equals("inpad"));

                    CHECK(mode->num_interconnect > 0);
                    REQUIRE(mode->interconnect != nullptr);

                    for (int iint = 0; iint < mode->num_interconnect; ++iint) {
                        if (strcmp("inpad", mode->interconnect[iint].name) == 0) {
                            found_direct = true;
                            REQUIRE(mode->interconnect[iint].meta.has("interconnect"));
                            auto* interconnect_value = mode->interconnect[iint].meta.one("interconnect");
                            REQUIRE(interconnect_value != nullptr);
                            CHECK_THAT(interconnect_value->as_string(), Equals("inpad_iconnect"));
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

    free_type_descriptors(logical_block_types);
    free_type_descriptors(physical_tile_types);
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
        auto* value = node_meta.second.one("node");
        REQUIRE(value != nullptr);
        CHECK_THAT(value->as_string(), Equals("test node"));
    }

    for (const auto& edge_meta : device_ctx.rr_edge_metadata) {
        CHECK(std::get<0>(edge_meta.first) == src_inode);
        CHECK(std::get<1>(edge_meta.first) == sink_inode);
        CHECK(std::get<2>(edge_meta.first) == switch_id);

        REQUIRE(edge_meta.second.has("edge"));
        auto* value = edge_meta.second.one("edge");
        REQUIRE(value != nullptr);
        CHECK_THAT(value->as_string(), Equals("test edge"));
    }
    vpr_free_all(arch, vpr_setup);
}

} // namespace
