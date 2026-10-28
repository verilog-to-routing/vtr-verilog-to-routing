#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_floating_point.hpp"

#include "physical_types.h"

namespace {

using Catch::Matchers::WithinRel;

TEST_CASE("Fixed switch delay", "[archfpga]") {
    t_arch_switch_inf arch_switch;

    SECTION("Same min and max delay") {
        arch_switch.set_Tdel(t_arch_switch_inf::UNDEFINED_FANIN, 5e-11, 5e-11);

        REQUIRE(arch_switch.fixed_Tdel());
        REQUIRE(arch_switch.Tdel_min() == 5e-11f);
        REQUIRE(arch_switch.Tdel_max() == 5e-11f);
    }

    SECTION("Separate min and max delays") {
        arch_switch.set_Tdel(t_arch_switch_inf::UNDEFINED_FANIN, 4e-11, 6e-11);

        REQUIRE(arch_switch.fixed_Tdel());
        REQUIRE(arch_switch.Tdel_min() == 4e-11f);
        REQUIRE(arch_switch.Tdel_max() == 6e-11f);
        // A fixed delay does not depend on the fan-in
        REQUIRE(arch_switch.Tdel_min(7) == 4e-11f);
        REQUIRE(arch_switch.Tdel_max(7) == 6e-11f);
    }
}

TEST_CASE("Fan-in dependent switch delay", "[archfpga]") {
    t_arch_switch_inf arch_switch;

    SECTION("Listed fan-ins") {
        arch_switch.set_Tdel(10, 4e-11, 5e-11);
        arch_switch.set_Tdel(20, 6e-11, 8e-11);

        REQUIRE_FALSE(arch_switch.fixed_Tdel());
        REQUIRE(arch_switch.Tdel_min(10) == 4e-11f);
        REQUIRE(arch_switch.Tdel_max(10) == 5e-11f);
        REQUIRE(arch_switch.Tdel_min(20) == 6e-11f);
        REQUIRE(arch_switch.Tdel_max(20) == 8e-11f);
    }

    SECTION("Interpolated fan-in") {
        arch_switch.set_Tdel(10, 4e-11, 5e-11);
        arch_switch.set_Tdel(20, 6e-11, 8e-11);

        REQUIRE_THAT(arch_switch.Tdel_min(15), WithinRel(5e-11f, 1e-5f));
        REQUIRE_THAT(arch_switch.Tdel_max(15), WithinRel(6.5e-11f, 1e-5f));
    }

    SECTION("Same min and max delay stay the same when extrapolated") {
        arch_switch.set_Tdel(10, 5e-11, 5e-11);
        arch_switch.set_Tdel(20, 3e-11, 3e-11);

        // Extrapolates to a negative delay. The min delay is not clamped to 0
        // here, since it must stay equal to the max delay.
        for (int fanin : {0, 5, 30, 50}) {
            REQUIRE(arch_switch.Tdel_min(fanin) == arch_switch.Tdel_max(fanin));
        }
    }

    SECTION("Extrapolated min delay is clamped to the max delay") {
        // The gap between max and min shrinks with fan-in, so extrapolating
        // the min and max delays separately gives min > max at large fan-ins
        arch_switch.set_Tdel(10, 4e-11, 5e-11);
        arch_switch.set_Tdel(20, 5.5e-11, 6e-11);

        // Without clamping: min(50) = 10e-11 and max(50) = 9e-11
        REQUIRE_THAT(arch_switch.Tdel_max(50), WithinRel(9e-11f, 1e-5f));
        REQUIRE(arch_switch.Tdel_min(50) == arch_switch.Tdel_max(50));

        for (int fanin = 0; fanin <= 100; fanin++) {
            REQUIRE(arch_switch.Tdel_min(fanin) <= arch_switch.Tdel_max(fanin));
        }
    }

    SECTION("Extrapolated min delay is clamped to 0") {
        // Min delay goes negative at small fan-ins while the max delay does not
        arch_switch.set_Tdel(10, 1e-11, 5e-11);
        arch_switch.set_Tdel(20, 3e-11, 6e-11);

        // Without clamping: min(0) = -1e-11 and max(0) = 4e-11
        REQUIRE(arch_switch.Tdel_min(0) == 0.f);
        REQUIRE_THAT(arch_switch.Tdel_max(0), WithinRel(4e-11f, 1e-5f));
    }
}

} // namespace
