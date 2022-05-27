#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_string.hpp"

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

    auto type_str = arch.strings.intern_string(vtr::string_view("type"));
    auto pb_type_type = arch.strings.intern_string(vtr::string_view("pb_type_type"));
    auto single = arch.strings.intern_string(vtr::string_view("single"));
    auto mode_str = arch.strings.intern_string(vtr::string_view("mode"));
    auto interconnect_str = arch.strings.intern_string(vtr::string_view("interconnect"));

    bool found_perimeter_meta = false;
    bool found_single_meta = false;
    for (const auto& grid_def : arch.grid_layouts) {
        for (const auto& loc_def : grid_def.loc_defs) {
            if (loc_def.block_type == "io") {
                REQUIRE(loc_def.meta != nullptr);
                REQUIRE(loc_def.meta->has(type_str));
                auto* value = loc_def.meta->one(type_str);
                REQUIRE(value != nullptr);
                CHECK_THAT(value->as_string().get(&arch.strings), Equals("io"));
                found_perimeter_meta = true;
            }

            if (loc_def.block_type == "clb" && loc_def.x.start_expr == "5" && loc_def.y.start_expr == "5") {
                REQUIRE(loc_def.meta != nullptr);
                REQUIRE(loc_def.meta->has(single));
                auto* value = loc_def.meta->one(single);
                REQUIRE(value != nullptr);
                CHECK_THAT(value->as_string().get(&arch.strings), Equals("clb"));
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
            REQUIRE(type.pb_type->meta.has(pb_type_type));
            auto* pb_type_value = type.pb_type->meta.one(pb_type_type);
            REQUIRE(pb_type_value != nullptr);
            CHECK_THAT(pb_type_value->as_string().get(&arch.strings), Equals("pb_type = io"));

            REQUIRE(type.pb_type->num_modes > 0);
            REQUIRE(type.pb_type->modes != nullptr);

            for (int imode = 0; imode < type.pb_type->num_modes; ++imode) {
                if (strcmp("inpad", type.pb_type->modes[imode].name) == 0) {
                    found_mode = true;
                    const auto* mode = &type.pb_type->modes[imode];

                    REQUIRE(mode->meta.has(mode_str));
                    auto* mode_value = mode->meta.one(mode_str);
                    REQUIRE(mode_value != nullptr);
                    CHECK_THAT(mode_value->as_string().get(&arch.strings), Equals("inpad"));

                    CHECK(mode->num_interconnect > 0);
                    REQUIRE(mode->interconnect != nullptr);

                    for (int iint = 0; iint < mode->num_interconnect; ++iint) {
                        if (strcmp("inpad", mode->interconnect[iint].name) == 0) {
                            found_direct = true;
                            REQUIRE(mode->interconnect[iint].meta.has(interconnect_str));
                            auto* interconnect_value = mode->interconnect[iint].meta.one(interconnect_str);
                            REQUIRE(interconnect_value != nullptr);
                            CHECK_THAT(interconnect_value->as_string().get(&arch.strings), Equals("inpad_iconnect"));
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
    /* TODO: All the inode should use RRNodeId */
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
            "100"};
        vpr_init(sizeof(argv) / sizeof(argv[0]), argv,
                 &options, &vpr_setup, &arch);
        vpr_setup.RouterOpts.read_rr_edge_metadata = true;
        vpr_create_device(vpr_setup, arch);

        const auto& device_ctx = g_vpr_ctx.device();
        const auto& rr_graph = device_ctx.rr_graph;

        for (const RRNodeId& inode : device_ctx.rr_graph.nodes()) {
            if ((rr_graph.node_type(inode) == CHANX || rr_graph.node_type(inode) == CHANY) && rr_graph.num_edges(inode) > 0) {
                src_inode = size_t(inode);
                break;
            }
        }

        REQUIRE(src_inode >= 0);
        sink_inode = size_t(rr_graph.edge_sink_node(RRNodeId(src_inode), 0));
        switch_id = rr_graph.edge_switch(RRNodeId(src_inode), 0);

        vpr::add_rr_node_metadata(src_inode, vtr::string_view("node"), vtr::string_view("test node"));
        vpr::add_rr_edge_metadata(src_inode, sink_inode, switch_id, vtr::string_view("edge"), vtr::string_view("test edge"));

        write_rr_graph(kRrGraphFile);
        vpr_free_all(arch, vpr_setup);

        auto& atom_ctx = g_vpr_ctx.mutable_atom();
        free_pack_molecules(atom_ctx.list_of_pack_molecules.release());
        atom_ctx.atom_molecules.clear();
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
        "--reorder_rr_graph_nodes_seed",
        "1",
        "--reorder_rr_graph_nodes_algorithm",
        "random_shuffle", // Tests node reordering with metadata
        "--read_rr_graph",
        kRrGraphFile,
    };

    vpr_init(sizeof(argv) / sizeof(argv[0]), argv,
             &options, &vpr_setup, &arch);
    vpr_setup.RouterOpts.read_rr_edge_metadata = true;
    vpr_create_device(vpr_setup, arch);

    const auto& device_ctx = g_vpr_ctx.device();

    // recompute ordering from 'random_shuffle'
    std::vector<int> src_order(device_ctx.rr_graph.num_nodes()); // new id -> old id
    std::iota(src_order.begin(), src_order.end(), 0);            // Initialize to [0, 1, 2 ...]
    std::mt19937 g(1);
    std::shuffle(src_order.begin(), src_order.end(), g);

    CHECK(device_ctx.rr_graph_builder.rr_node_metadata_size() == 1);
    CHECK(device_ctx.rr_graph_builder.rr_edge_metadata_size() == 1);

    auto node = arch.strings.intern_string(vtr::string_view("node"));
    auto edge = arch.strings.intern_string(vtr::string_view("edge"));

    for (const auto& node_meta : device_ctx.rr_graph.rr_node_metadata_data()) {
        CHECK(src_order[node_meta.first] == src_inode);
        REQUIRE(node_meta.second.has(node));
        auto* value = node_meta.second.one(node);
        REQUIRE(value != nullptr);
        CHECK_THAT(value->as_string().get(&arch.strings), Equals("test node"));
    }

    for (const auto& edge_meta : device_ctx.rr_graph.rr_edge_metadata_data()) {
        CHECK(src_order[std::get<0>(edge_meta.first)] == src_inode);
        CHECK(src_order[std::get<1>(edge_meta.first)] == sink_inode);
        CHECK(std::get<2>(edge_meta.first) == switch_id);

        REQUIRE(edge_meta.second.has(edge));
        auto* value = edge_meta.second.one(edge);
        REQUIRE(value != nullptr);
        CHECK_THAT(value->as_string().get(&arch.strings), Equals("test edge"));
    }
    vpr_free_all(arch, vpr_setup);

    auto& atom_ctx = g_vpr_ctx.mutable_atom();
    free_pack_molecules(atom_ctx.list_of_pack_molecules.release());
    atom_ctx.atom_molecules.clear();
}

} // namespace
