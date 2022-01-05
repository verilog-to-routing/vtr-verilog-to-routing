#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "parameters.h"

namespace {

using Catch::Matchers::Equals;

TEST_CASE("parameters", "[fasm]") {
    fasm::Parameters params;

    params.AddParameter("A", "B");
    params.AddParameter("INIT_0", "INIT[31:0]");
    params.AddParameter("INIT_1", "INIT[63:32]");

    // Unmatched features returns "".
    CHECK_THAT(params.EmitFasmFeature("C", "0"), Equals(""));

    CHECK_THAT(params.EmitFasmFeature("A", "0"), Equals("B=1'b0\n"));
    CHECK_THAT(params.EmitFasmFeature("INIT_0", "10100000000000000000000000000001"), Equals("INIT[31:0]=32'b10100000000000000000000000000001\n"));
    CHECK_THAT(params.EmitFasmFeature("INIT_1", "00010000000000000000000000001001"), Equals("INIT[63:32]=32'b00010000000000000000000000001001\n"));
}

} // namespace
