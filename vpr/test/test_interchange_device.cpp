#include "catch2/catch_test_macros.hpp"

#include "read_fpga_interchange_arch.h"
#include "arch_util.h"
#include "vpr_api.h"
#include <cstring>
#include <unordered_set>
#include <vector>

namespace {

static constexpr const char kArchFile[] = "testarch.device";

TEST_CASE("read_interchange_models", "[vpr]") {
    t_arch arch;
    std::vector<t_physical_tile_type> physical_tile_types;
    std::vector<t_logical_block_type> logical_block_types;

    FPGAInterchangeReadArch(kArchFile, /*timing_enabled=*/true, &arch, physical_tile_types, logical_block_types);

    std::unordered_set<std::string> models = {"IB", "OB", "DFF", "GND", "VCC"};

    // Check that there are exactly the expected models
    for (auto* model = arch.models; model != nullptr; model = model->next) {
        std::string name = model->name;
        REQUIRE(models.find(name) != models.end());
        models.erase(name);
    }

    REQUIRE(models.size() == 0);

    std::unordered_set<std::string> lib_models = {MODEL_INPUT, MODEL_OUTPUT, MODEL_LATCH, MODEL_NAMES};

    // Check that there are exactly the expected models
    for (auto* model = arch.model_library; model != nullptr; model = model->next) {
        std::string name = model->name;
        REQUIRE(lib_models.find(name) != lib_models.end());
        lib_models.erase(name);
    }

    REQUIRE(lib_models.size() == 0);
}

TEST_CASE("read_interchange_layout", "[vpr]") {
    t_arch arch;
    std::vector<t_physical_tile_type> physical_tile_types;
    std::vector<t_logical_block_type> logical_block_types;

    FPGAInterchangeReadArch(kArchFile, /*timing_enabled=*/true, &arch, physical_tile_types, logical_block_types);

    auto& gd = arch.grid_layouts[0];
    REQUIRE(gd.grid_type == GridDefType::FIXED);
    REQUIRE(gd.height == 10);
    REQUIRE(gd.width == 10);

    std::unordered_map<std::string, bool> tile_types({{"NULL", false}, {"PWR", false}, {"IOB", false}, {"CLB", false}});
    for (auto& loc : gd.loc_defs) {
        auto ret = tile_types.find(loc.block_type);
        REQUIRE(ret != tile_types.end());
        REQUIRE(loc.priority == 1);

        ret->second = true;
    }

    for (auto type : tile_types) {
        CHECK(type.second);
    }
}

TEST_CASE("read_interchange_luts", "[vpr]") {
    t_arch arch;
    std::vector<t_physical_tile_type> physical_tile_types;
    std::vector<t_logical_block_type> logical_block_types;

    FPGAInterchangeReadArch(kArchFile, /*timing_enabled=*/true, &arch, physical_tile_types, logical_block_types);

    std::unordered_set<std::string> lut_cell_pins = {"A0", "A1", "A2", "A3"};
    std::unordered_set<std::string> lut_bel_pins = {"I0", "I1", "I2", "I3"};

    REQUIRE(arch.lut_cells.size() == 1);
    REQUIRE(arch.lut_bels.size() == 1);

    auto lut_cell = arch.lut_cells[0];
    REQUIRE(lut_cell.name == std::string("LUT"));
    REQUIRE(lut_cell.init_param == std::string("INIT"));
    for (auto lut_pin : lut_cell_pins)
        CHECK(std::find(lut_cell.inputs.begin(), lut_cell.inputs.end(), lut_pin) != lut_cell.inputs.end());

    auto lut_bel = arch.lut_bels[0];
    REQUIRE(lut_bel.name == std::string("LUT"));
    REQUIRE(lut_bel.output_pin == std::string("O"));
    for (auto lut_pin : lut_bel_pins)
        CHECK(std::find(lut_bel.input_pins.begin(), lut_bel.input_pins.end(), lut_pin) != lut_bel.input_pins.end());
}

TEST_CASE("read_interchange_pin_packages", "[vpr]") {
    t_arch arch;
    std::vector<t_physical_tile_type> physical_tile_types;
    std::vector<t_logical_block_type> logical_block_types;

    FPGAInterchangeReadArch(kArchFile, /*timing_enabled=*/true, &arch, physical_tile_types, logical_block_types);

    // The device architecture file contains 35 perimetral PADs
    REQUIRE(arch.pad_bels.size() == 35);

    int ipad = 0;
    for (auto pad_bel : arch.pad_bels) {
        REQUIRE(pad_bel.name == std::string("A") + std::to_string(ipad++));
        REQUIRE(pad_bel.bel_name == std::string("PAD"));
    }
}

TEST_CASE("read_interchange_tiles", "[vpr]") {
    t_arch arch;
    std::vector<t_physical_tile_type> physical_tile_types;
    std::vector<t_logical_block_type> logical_block_types;

    FPGAInterchangeReadArch(kArchFile, /*timing_enabled=*/true, &arch, physical_tile_types, logical_block_types);

    std::unordered_set<std::string> ptypes = {"NULL", "IOB", "PWR", "CLB"};

    // Check that there are exactly the expected models
    for (auto ptype : physical_tile_types) {
        std::string name = ptype.name;
        REQUIRE(ptypes.find(name) != ptypes.end());
        ptypes.erase(name);

        if (name == std::string("IOB")) {
            CHECK(ptype.is_input_type);
            CHECK(ptype.is_output_type);
        }
    }

    REQUIRE(ptypes.size() == 0);
}

TEST_CASE("read_interchange_pb_types", "[vpr]") {
    t_arch arch;
    std::vector<t_physical_tile_type> physical_tile_types;
    std::vector<t_logical_block_type> logical_block_types;

    FPGAInterchangeReadArch(kArchFile, /*timing_enabled=*/true, &arch, physical_tile_types, logical_block_types);

    std::unordered_set<std::string> ltypes = {"NULL", "IOPAD", "SLICE", "POWER"};

    std::unordered_map<std::string, PORTS> slice_ports = {
        {"L0", PORTS::IN_PORT},
        {"L1", PORTS::IN_PORT},
        {"L2", PORTS::IN_PORT},
        {"L3", PORTS::IN_PORT},
        {"R", PORTS::IN_PORT},
        {"C", PORTS::IN_PORT},
        {"D", PORTS::IN_PORT},
        {"O", PORTS::OUT_PORT},
        {"Q", PORTS::OUT_PORT}};

    // Check that there are exactly the expected models
    for (auto ltype : logical_block_types) {
        std::string name = ltype.name;
        REQUIRE(ltypes.find(name) != ltypes.end());
        ltypes.erase(name);

        if (ltype.pb_type == nullptr) {
            REQUIRE(name == std::string("NULL"));
            continue;
        }

        bool check_pb_type = name == std::string("SLICE");
        size_t num_visited = 0;
        for (auto iport = 0; iport < ltype.pb_type->num_ports; iport++) {
            auto port = ltype.pb_type->ports[iport];

            REQUIRE(port.name != nullptr);

            if (!check_pb_type)
                continue;

            auto res = slice_ports.find(std::string(port.name));
            REQUIRE(res != slice_ports.end());
            REQUIRE(res->second == port.type);
            num_visited++;
        }

        REQUIRE((num_visited == slice_ports.size() || !check_pb_type));
    }

    REQUIRE(ltypes.size() == 0);
}

} // namespace
