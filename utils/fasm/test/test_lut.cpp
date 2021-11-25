#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "lut.h"

namespace {

using Catch::Matchers::Equals;

TEST_CASE("default_lut", "[fasm]") {
    for(size_t num_inputs = 0; num_inputs < 10; ++num_inputs) {
        fasm::Lut lut(num_inputs);

        const LogicVec &table = lut.table();
        CHECK(table.size() == (1 << num_inputs));

        for(const vtr::LogicValue & value : table) {
            CHECK(value == vtr::LogicValue::FALSE);
        }
    }
}


TEST_CASE("const_true", "[fasm]") {
    for(size_t num_inputs = 0; num_inputs < 10; ++num_inputs) {
        fasm::Lut lut(num_inputs);
        lut.SetConstant(vtr::LogicValue::TRUE);

        const LogicVec &table = lut.table();
        CHECK(table.size() == (1 << num_inputs));

        for(const vtr::LogicValue & value : table) {
            CHECK(value == vtr::LogicValue::TRUE);
        }
    }
}

TEST_CASE("const_false", "[fasm]") {
    for(size_t num_inputs = 0; num_inputs < 10; ++num_inputs) {
        {
            fasm::Lut lut(num_inputs);
            lut.SetConstant(vtr::LogicValue::FALSE);

            const LogicVec &table = lut.table();
            CHECK(table.size() == (1 << num_inputs));

            for(const vtr::LogicValue & value : table) {
                CHECK(value == vtr::LogicValue::FALSE);
            }
        }
    }
}

TEST_CASE("wire", "[fasm]") {
    for(size_t num_inputs = 0; num_inputs < 10; ++num_inputs) {
        for(size_t input_pin = 0; input_pin < num_inputs; ++input_pin) {
            fasm::Lut lut(num_inputs);
            lut.CreateWire(input_pin);

            const LogicVec &table = lut.table();
            CHECK(table.size() == (1 << num_inputs));
            for(size_t i = 0; i < table.size(); ++i) {
                if(((1 << input_pin) & i) != 0) {
                    CHECK(table[i] == vtr::LogicValue::TRUE);
                } else {
                    CHECK(table[i] == vtr::LogicValue::FALSE);
                }
            }
        }
    }
}

TEST_CASE("lut_output", "[fasm]") {
    fasm::LutOutputDefinition lut_def("TEST[31:0]");

    CHECK_THAT(lut_def.fasm_feature, Equals("TEST"));
    CHECK(lut_def.num_inputs == 5);
    CHECK(lut_def.start_bit == 0);
    CHECK(lut_def.end_bit == 31);

    CHECK_THAT(lut_def.CreateConstant(vtr::LogicValue::TRUE), Equals("TEST[31:0]=32'b11111111111111111111111111111111"));
    CHECK_THAT(lut_def.CreateConstant(vtr::LogicValue::FALSE), Equals("TEST[31:0]=32'b00000000000000000000000000000000"));
    CHECK_THAT(lut_def.CreateWire(0), Equals("TEST[31:0]=32'b10101010101010101010101010101010"));
    CHECK_THAT(lut_def.CreateWire(1), Equals("TEST[31:0]=32'b11001100110011001100110011001100"));
}

TEST_CASE("lut_table_output", "[fasm]") {
    fasm::LutOutputDefinition lut_def("TEST[7:4]");

    CHECK_THAT(lut_def.fasm_feature, Equals("TEST"));
    CHECK(lut_def.num_inputs == 2);
    CHECK(lut_def.start_bit == 4);
    CHECK(lut_def.end_bit == 7);

    LogicVec vec(4, vtr::LogicValue::FALSE);

    CHECK_THAT(lut_def.CreateInit(vec), Equals("TEST[7:4]=4'b0000"));

    vec[0] = vtr::LogicValue::TRUE;
    CHECK_THAT(lut_def.CreateInit(vec), Equals("TEST[7:4]=4'b0001"));

    vec[3] = vtr::LogicValue::TRUE;
    CHECK_THAT(lut_def.CreateInit(vec), Equals("TEST[7:4]=4'b1001"));
}

} // namespace
