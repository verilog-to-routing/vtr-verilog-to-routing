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

} // namespace
