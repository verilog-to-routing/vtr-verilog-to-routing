#include "catch.hpp"

#include "read_xml_arch_file.h"
#include "arch_util.h"
#include "vpr_api.h"
#include <cstring>

namespace {

using Catch::Matchers::Equals;

static constexpr const char kArchFile[] = "test_read_arch_metadata.xml";
static constexpr const char kRrGraphFile[] = "test_read_rrgraph_metadata.xml";

TEST_CASE("read_arch_metadata", "[vpr]") {
    t_arch arch;
    t_type_descriptor * types;
    int num_types;

    XmlReadArch(kArchFile, /*timing_enabled=*/false,
        &arch, &types, &num_types);

    bool found_perimeter_meta = false;
    bool found_single_meta = false;
    for(const auto & grid_def : arch.grid_layouts) {
        for(const auto & loc_def : grid_def.loc_defs) {
            if(loc_def.block_type == "io") {
                REQUIRE(loc_def.meta != nullptr);
                REQUIRE(loc_def.meta->has("type"));
                CHECK_THAT(loc_def.meta->one("type")->as_string(), Equals("io"));
                found_perimeter_meta = true;
            }

            if(loc_def.block_type == "clb" && loc_def.x.start_expr == "5" && loc_def.y.start_expr == "5") {
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

    for(int i = 0; i < num_types; ++i) {
        if(strcmp("io", types[i].name) == 0) {
            found_pb_type = true;
            REQUIRE(types[i].pb_type != nullptr);
            REQUIRE(types[i].pb_type->meta.has("pb_type_type"));
            CHECK_THAT(types[i].pb_type->meta.one("pb_type_type")->as_string(), Equals("pb_type = io"));

            REQUIRE(types[i].pb_type->num_modes > 0);
            REQUIRE(types[i].pb_type->modes != nullptr);

            for(int imode = 0; imode < types[i].pb_type->num_modes; ++imode) {
                if(strcmp("inpad", types[i].pb_type->modes[imode].name) == 0) {
                    found_mode = true;
                    const auto * mode = &types[i].pb_type->modes[imode];

                    REQUIRE(mode->meta.has("mode"));
                    CHECK_THAT(mode->meta.one("mode")->as_string(), Equals("inpad"));

                    CHECK(mode->num_interconnect > 0);
                    REQUIRE(mode->interconnect != nullptr);

                    for(int iint = 0; iint < mode->num_interconnect; ++iint) {
                        if(strcmp("inpad", mode->interconnect[iint].name) == 0) {
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
    const char *argv[] = {
        "test_vpr",
        kArchFile,
        "wire.eblif",
        "--read_rr_graph",
        kRrGraphFile,
        "--route_chan_width",
        "100",
    };

    t_vpr_setup vpr_setup;
    t_arch arch;
    t_options options;
    vpr_init(sizeof(argv)/sizeof(argv[0]), argv,
              &options, &vpr_setup, &arch);
    vpr_create_device(vpr_setup, arch);

    const auto& device_ctx = g_vpr_ctx.device();
    CHECK(device_ctx.rr_node_metadata.size() == 1);
    for(const auto & node_meta: device_ctx.rr_node_metadata) {
        CHECK(node_meta.first->type() == SINK);
        CHECK(node_meta.first->xlow() == 0);
        CHECK(node_meta.first->xhigh() == 0);
        CHECK(node_meta.first->ylow() == 1);
        CHECK(node_meta.first->yhigh() == 1);
        REQUIRE(node_meta.second.has("node"));
        CHECK_THAT(node_meta.second.one("node")->as_string(), Equals("test node"));
    }

    CHECK(device_ctx.rr_edge_metadata.size() == 1);
    for(const auto & edge_meta: device_ctx.rr_edge_metadata) {
        CHECK(edge_meta.first->type() == CHANY);
        CHECK(edge_meta.first->xlow() == 1);
        CHECK(edge_meta.first->xhigh() == 1);
        CHECK(edge_meta.first->ylow() == 1);
        CHECK(edge_meta.first->yhigh() == 1);

        CHECK(edge_meta.second.size() == 1);
        constexpr int kSinkNode = 559;
        constexpr int kSwitchId = 2;

        auto iter = edge_meta.second.find(std::make_pair(kSinkNode, kSwitchId));
        REQUIRE(iter != edge_meta.second.end());
        REQUIRE(iter->second.has("edge"));
        CHECK_THAT(iter->second.one("edge")->as_string(), Equals("test edge"));
    }
    vpr_free_all(arch, vpr_setup);
}

} // namespace
