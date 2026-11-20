/**
 * @file
 * @brief Unit tests for giving the positional arguments (architecture and circuit) in a
 *        --options_file.
 *
 * The other features of --options_file are covered by the vtr_reg_strong/strong_options_file
 * regression test. Positional arguments can not be tested there, since run_vtr_flow always passes
 * them on the command-line.
 */

#include "catch2/catch_test_macros.hpp"

#include "read_options.h"

#include <string>
#include <vector>

TEST_CASE("options_file_positional_args", "[vpr]") {
    std::vector<const char*> argv = {"test_vpr",
                                     "--options_file",
                                     "test_read_options_file.yaml"};
    t_options options = read_options(argv.size(), argv.data());

    CHECK(options.ArchFile.value() == "test_read_arch_metadata.xml");
    CHECK(options.ArchFile.provenance() == argparse::Provenance::SPECIFIED);
    CHECK(options.CircuitFile.value() == "wire.eblif");
    CHECK(options.CircuitName.value() == "wire");

    // Options given in the same file are read as well.
    CHECK(options.seed.value() == 7);

    // --options_file is replaced by the file's arguments, with the positional arguments first.
    std::vector<std::string> expected_args = {"test_read_arch_metadata.xml", "wire.eblif", "--seed", "7"};
    CHECK(options.expanded_args == expected_args);
}
