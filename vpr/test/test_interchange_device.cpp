#include "catch.hpp"

#include "read_fpga_interchange_arch.h"
#include "arch_util.h"
#include "vpr_api.h"
#include <cstring>
#include <unordered_set>
#include <vector>

namespace {

using Catch::Matchers::Equals;

static constexpr const char kArchFile[] = "testarch.device";

TEST_CASE("read_interchange_models", "[vpr]") {
    t_arch arch;
    std::vector<t_physical_tile_type> physical_tile_types;
    std::vector<t_logical_block_type> logical_block_types;

    FPGAInterchangeReadArch(kArchFile, /*timing_enabled=*/true, &arch, physical_tile_types, logical_block_types);

    std::unordered_set<std::string> models = {"IB", "OB", "LUT", "DFF", "GND", "VCC"};

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

} // namespace
