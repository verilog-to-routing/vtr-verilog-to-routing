#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_string.hpp"

#include "read_xml_arch_file.h"
#include "physical_types.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

using Catch::Matchers::ContainsSubstring;

static constexpr const char kBaseArchFile[] = "test_read_arch_metadata.xml";
static constexpr const char kSwitchArchFile[] = "test_read_arch_switch_delay.xml";

// The switch definition in kBaseArchFile which is replaced by each test
static constexpr const char kBaseSwitch[] = R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15" Tdel="58e-12" mux_trans_size="2.630740" buf_size="27.645901"/>)";

/**
 * @brief Reads kBaseArchFile with the definition of switch "0" replaced by switch_xml.
 *
 * @return The architecture switch named "0".
 */
static t_arch_switch_inf read_arch_switch(const std::string& switch_xml) {
    std::ifstream base_file(kBaseArchFile);
    std::stringstream buffer;
    buffer << base_file.rdbuf();
    std::string arch_xml = buffer.str();

    size_t pos = arch_xml.find(kBaseSwitch);
    REQUIRE(pos != std::string::npos);
    arch_xml.replace(pos, std::string(kBaseSwitch).size(), switch_xml);

    {
        std::ofstream switch_file(kSwitchArchFile);
        switch_file << arch_xml;
    }

    t_arch arch;
    std::vector<t_physical_tile_type> physical_tile_types;
    std::vector<t_logical_block_type> logical_block_types;
    try {
        xml_read_arch(kSwitchArchFile, /*timing_enabled=*/false,
                      &arch, physical_tile_types, logical_block_types, /*device_model_warnings=*/true);
    } catch (...) {
        std::remove(kSwitchArchFile);
        throw;
    }
    std::remove(kSwitchArchFile);

    for (const t_arch_switch_inf& arch_switch : arch.switches) {
        if (arch_switch.name == "0") {
            return arch_switch;
        }
    }
    FAIL("Switch '0' not found");
    return t_arch_switch_inf();
}

TEST_CASE("read_arch_switch_delay", "[vpr]") {
    SECTION("Tdel attribute is used as both min and max") {
        t_arch_switch_inf sw = read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15" Tdel="58e-12"/>)");
        REQUIRE(sw.fixed_Tdel());
        REQUIRE(sw.Tdel_min() == 58e-12f);
        REQUIRE(sw.Tdel_max() == 58e-12f);
    }

    SECTION("Tdel child with delay is used as both min and max") {
        t_arch_switch_inf sw = read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15">
                <Tdel num_inputs="12" delay="8.0e-11"/>
                <Tdel num_inputs="15" delay="8.4e-11"/>
            </switch>)");
        REQUIRE_FALSE(sw.fixed_Tdel());
        REQUIRE(sw.Tdel_min(12) == 8.0e-11f);
        REQUIRE(sw.Tdel_max(12) == 8.0e-11f);
        REQUIRE(sw.Tdel_min(15) == 8.4e-11f);
        REQUIRE(sw.Tdel_max(15) == 8.4e-11f);
    }

    SECTION("Fixed min and max delay") {
        t_arch_switch_inf sw = read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15">
                <Tdel min="56e-12" max="58e-12"/>
            </switch>)");
        REQUIRE(sw.fixed_Tdel());
        REQUIRE(sw.Tdel_min() == 56e-12f);
        REQUIRE(sw.Tdel_max() == 58e-12f);
    }

    SECTION("Fixed delay given with delay and without num_inputs") {
        t_arch_switch_inf sw = read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15">
                <Tdel delay="58e-12"/>
            </switch>)");
        REQUIRE(sw.fixed_Tdel());
        REQUIRE(sw.Tdel_min() == 58e-12f);
        REQUIRE(sw.Tdel_max() == 58e-12f);
    }

    SECTION("Fan-in dependent min and max delays") {
        t_arch_switch_inf sw = read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15">
                <Tdel num_inputs="12" min="7.5e-11" max="8.0e-11"/>
                <Tdel num_inputs="15" min="7.9e-11" max="8.4e-11"/>
            </switch>)");
        REQUIRE_FALSE(sw.fixed_Tdel());
        REQUIRE(sw.Tdel_min(12) == 7.5e-11f);
        REQUIRE(sw.Tdel_max(12) == 8.0e-11f);
        REQUIRE(sw.Tdel_min(15) == 7.9e-11f);
        REQUIRE(sw.Tdel_max(15) == 8.4e-11f);
    }

    SECTION("Error: min without max") {
        REQUIRE_THROWS_WITH(read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15">
                <Tdel min="56e-12"/>
            </switch>)"),
                            ContainsSubstring("min without max"));
    }

    SECTION("Error: delay together with min or max") {
        REQUIRE_THROWS_WITH(read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15">
                <Tdel delay="58e-12" max="58e-12"/>
            </switch>)"),
                            ContainsSubstring("EITHER delay OR min/max"));
        REQUIRE_THROWS_WITH(read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15">
                <Tdel delay="58e-12" min="56e-12"/>
            </switch>)"),
                            ContainsSubstring("EITHER delay OR min/max"));
    }

    SECTION("Error: max without min") {
        REQUIRE_THROWS_WITH(read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15">
                <Tdel max="58e-12"/>
            </switch>)"),
                            ContainsSubstring("max without min"));
    }

    SECTION("Error: neither delay nor min and max") {
        REQUIRE_THROWS_WITH(read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15">
                <Tdel num_inputs="12"/>
            </switch>)"),
                            ContainsSubstring("should specify delay, or both min and max"));
    }

    SECTION("Error: min larger than max") {
        REQUIRE_THROWS_WITH(read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15">
                <Tdel min="60e-12" max="58e-12"/>
            </switch>)"),
                            ContainsSubstring("should not be larger than max"));
    }

    SECTION("Error: negative min") {
        REQUIRE_THROWS_WITH(read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15">
                <Tdel min="-1e-12" max="58e-12"/>
            </switch>)"),
                            ContainsSubstring("should not be negative"));
    }

    SECTION("Error: Tdel child without num_inputs mixed with other Tdel children") {
        REQUIRE_THROWS_WITH(read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15">
                <Tdel min="56e-12" max="58e-12"/>
                <Tdel num_inputs="12" min="7.5e-11" max="8.0e-11"/>
            </switch>)"),
                            ContainsSubstring("must be the only Tdel node"));
    }

    SECTION("Error: two Tdel children without num_inputs") {
        REQUIRE_THROWS_WITH(read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15">
                <Tdel min="56e-12" max="58e-12"/>
                <Tdel min="56e-12" max="58e-12"/>
            </switch>)"),
                            ContainsSubstring("must be the only Tdel node"));
    }

    SECTION("Error: duplicate num_inputs") {
        REQUIRE_THROWS_WITH(read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15">
                <Tdel num_inputs="12" min="7.5e-11" max="8.0e-11"/>
                <Tdel num_inputs="12" delay="8.0e-11"/>
            </switch>)"),
                            ContainsSubstring("has already been specified by another Tdel node"));
    }

    SECTION("Error: Tdel attribute together with Tdel children") {
        REQUIRE_THROWS_WITH(read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15" Tdel="58e-12">
                <Tdel min="56e-12" max="58e-12"/>
            </switch>)"),
                            ContainsSubstring("EITHER a Tdel property OR as a child"));
    }

    SECTION("Error: unknown Tdel attribute") {
        REQUIRE_THROWS(read_arch_switch(R"(<switch type="mux" name="0" R="551" Cin=".77e-15" Cout="4e-15">
                <Tdel mni="56e-12" max="58e-12"/>
            </switch>)"));
    }
}

} // namespace
