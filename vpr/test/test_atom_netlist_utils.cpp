/**
 * @file
 * @brief Unit tests for the AtomNetlist utilities.
 *
 * These tests build small atom netlists directly (without reading a circuit)
 * and check that the netlist cleanup passes modify the netlist as intended.
 */
#include <string>
#include <unordered_set>

#include "catch2/catch_test_macros.hpp"

#include "atom_netlist.h"
#include "atom_netlist_utils.h"
#include "logic_types.h"

namespace {

/**
 * @brief Creates a constant generator (a LUT with no inputs) which drives a
 *        net with the same name.
 *
 *  @return The net driven by the constant generator.
 */
AtomNetId create_constant_generator(AtomNetlist& netlist,
                                    const LogicalModels& models,
                                    const std::string& name,
                                    vtr::LogicValue value) {
    AtomNetlist::TruthTable truth_table = {{value}};
    AtomBlockId blk_id = netlist.create_block(name, LogicalModels::MODEL_NAMES_ID, truth_table);

    const t_model& names_model = models.get_model(LogicalModels::MODEL_NAMES_ID);
    AtomPortId output_port_id = netlist.create_port(blk_id, names_model.outputs);
    AtomNetId net_id = netlist.create_net(name);
    netlist.create_pin(output_port_id, 0, net_id, PinType::DRIVER, /*is_const=*/true);

    return net_id;
}

/**
 * @brief Creates a primary output which is driven by the given net.
 */
void create_primary_output(AtomNetlist& netlist,
                           const LogicalModels& models,
                           const std::string& name,
                           AtomNetId net_id) {
    AtomBlockId blk_id = netlist.create_block(name, LogicalModels::MODEL_OUTPUT_ID);

    const t_model& outpad_model = models.get_model(LogicalModels::MODEL_OUTPUT_ID);
    AtomPortId input_port_id = netlist.create_port(blk_id, outpad_model.inputs);
    netlist.create_pin(input_port_id, 0, net_id, PinType::SINK);
}

TEST_CASE("test_merge_constant_generators", "[vpr_atom_netlist_utils]") {
    LogicalModels models;
    AtomNetlist netlist("test_netlist");

    // Create the constant-zero generators. gnd has the highest fanout, so it
    // should be the generator which is kept.
    AtomNetId gnd_net = create_constant_generator(netlist, models, "gnd", vtr::LogicValue::FALSE);
    create_primary_output(netlist, models, "out:gnd_0", gnd_net);
    create_primary_output(netlist, models, "out:gnd_1", gnd_net);
    AtomNetId zero_a_net = create_constant_generator(netlist, models, "zero_a", vtr::LogicValue::FALSE);
    create_primary_output(netlist, models, "out:zero_a", zero_a_net);
    AtomNetId zero_b_net = create_constant_generator(netlist, models, "zero_b", vtr::LogicValue::FALSE);
    create_primary_output(netlist, models, "out:zero_b", zero_b_net);

    // Give zero_b an alias, similar to what absorbing a buffer LUT does (which
    // adds both the kept and absorbed net names as aliases).
    netlist.add_net_alias("zero_b", "zero_b");
    netlist.add_net_alias("zero_b", "zero_b_buf");

    // Create the constant-one generators. vcc has the highest fanout, so it
    // should be the generator which is kept.
    AtomNetId vcc_net = create_constant_generator(netlist, models, "vcc", vtr::LogicValue::TRUE);
    create_primary_output(netlist, models, "out:vcc_0", vcc_net);
    create_primary_output(netlist, models, "out:vcc_1", vcc_net);
    AtomNetId one_a_net = create_constant_generator(netlist, models, "one_a", vtr::LogicValue::TRUE);
    create_primary_output(netlist, models, "out:one_a", one_a_net);

    size_t num_removed = merge_constant_generators(netlist, /*verbosity=*/0);

    SECTION("Test that the redundant constant generators are removed") {
        REQUIRE(num_removed == 3);
        REQUIRE(netlist.find_block("gnd"));
        REQUIRE(netlist.find_block("vcc"));
        REQUIRE(!netlist.find_block("zero_a"));
        REQUIRE(!netlist.find_block("zero_b"));
        REQUIRE(!netlist.find_block("one_a"));
    }

    SECTION("Test that the redundant constant nets are merged into the kept nets") {
        REQUIRE(netlist.find_net("gnd") == gnd_net);
        REQUIRE(netlist.find_net("vcc") == vcc_net);
        REQUIRE(!netlist.find_net("zero_a"));
        REQUIRE(!netlist.find_net("zero_b"));
        REQUIRE(!netlist.find_net("one_a"));

        REQUIRE(netlist.net_sinks(gnd_net).size() == 4);
        REQUIRE(netlist.net_sinks(vcc_net).size() == 3);

        // The primary outputs should keep their names and be driven by the
        // kept nets.
        AtomBlockId zero_a_po = netlist.find_block("out:zero_a");
        REQUIRE(netlist.pin_net(*netlist.block_input_pins(zero_a_po).begin()) == gnd_net);
        AtomBlockId zero_b_po = netlist.find_block("out:zero_b");
        REQUIRE(netlist.pin_net(*netlist.block_input_pins(zero_b_po).begin()) == gnd_net);
        AtomBlockId one_a_po = netlist.find_block("out:one_a");
        REQUIRE(netlist.pin_net(*netlist.block_input_pins(one_a_po).begin()) == vcc_net);
    }

    SECTION("Test that the net aliases are set correctly") {
        // The kept nets must keep their own names as aliases, and gain the
        // names (and aliases) of the nets merged into them.
        REQUIRE(netlist.net_aliases("gnd") == std::unordered_set<std::string>{"gnd", "zero_a", "zero_b", "zero_b_buf"});
        REQUIRE(netlist.net_aliases("vcc") == std::unordered_set<std::string>{"vcc", "one_a"});
    }

    SECTION("Test that the netlist is valid after compression") {
        netlist.remove_and_compress();
        REQUIRE(netlist.verify());
        REQUIRE(netlist.net_aliases("gnd") == std::unordered_set<std::string>{"gnd", "zero_a", "zero_b", "zero_b_buf"});
        REQUIRE(netlist.net_aliases("vcc") == std::unordered_set<std::string>{"vcc", "one_a"});
    }
}

TEST_CASE("test_merge_constant_generators_single_generator", "[vpr_atom_netlist_utils]") {
    LogicalModels models;
    AtomNetlist netlist("test_netlist");

    // A single generator of each value should not be modified.
    AtomNetId gnd_net = create_constant_generator(netlist, models, "gnd", vtr::LogicValue::FALSE);
    create_primary_output(netlist, models, "out:gnd", gnd_net);
    AtomNetId vcc_net = create_constant_generator(netlist, models, "vcc", vtr::LogicValue::TRUE);
    create_primary_output(netlist, models, "out:vcc", vcc_net);

    REQUIRE(merge_constant_generators(netlist, /*verbosity=*/0) == 0);
    REQUIRE(netlist.find_net("gnd") == gnd_net);
    REQUIRE(netlist.find_net("vcc") == vcc_net);
    REQUIRE(netlist.net_aliases("gnd") == std::unordered_set<std::string>{"gnd"});
    REQUIRE(netlist.net_aliases("vcc") == std::unordered_set<std::string>{"vcc"});
}

} // namespace
