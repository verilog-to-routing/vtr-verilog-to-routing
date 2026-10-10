/**
 * @file
 * @brief Unit tests for selecting which echo files are generated via the
 *        --echo_file command-line option.
 */

#include "catch2/catch_test_macros.hpp"

#include "echo_files.h"
#include "read_options.h"
#include "vpr_error.h"

#include <string>
#include <vector>

namespace {

/**
 * @brief Parse the given --echo_file values as VPR would from the command
 *        line, and set up the echo files from them.
 */
void init_echo_files_from_command_line(const std::vector<const char*>& echo_file_args) {
    std::vector<const char*> argv = {"test_vpr",
                                     "test_read_arch_metadata.xml",
                                     "wire.eblif",
                                     "--echo_file"};
    argv.insert(argv.end(), echo_file_args.begin(), echo_file_args.end());
    t_options options = read_options(argv.size(), argv.data());
    init_echo_files_from_options(options.echo_file.value());
}

TEST_CASE("echo_file_option", "[vpr]") {
    // The echo file state is global; make sure it is cleared from any
    // previous test before starting.
    free_echo_file_info();

    SECTION("Default is off") {
        std::vector<const char*> argv = {"test_vpr",
                                         "test_read_arch_metadata.xml",
                                         "wire.eblif"};
        t_options options = read_options(argv.size(), argv.data());
        init_echo_files_from_options(options.echo_file.value());

        CHECK_FALSE(getEchoEnabled());
        for (int i = 0; i < (int)E_ECHO_END_TOKEN; i++) {
            CHECK_FALSE(isEchoFileEnabled((e_echo_files)i));
        }
    }

    SECTION("Off disables all echo files") {
        init_echo_files_from_command_line({"off"});

        CHECK_FALSE(getEchoEnabled());
        for (int i = 0; i < (int)E_ECHO_END_TOKEN; i++) {
            CHECK_FALSE(isEchoFileEnabled((e_echo_files)i));
        }
    }

    SECTION("On enables all echo files") {
        init_echo_files_from_command_line({"on"});

        CHECK(getEchoEnabled());
        for (int i = 0; i < (int)E_ECHO_END_TOKEN; i++) {
            CHECK(isEchoFileEnabled((e_echo_files)i));
        }
    }

    SECTION("A list of echo files enables only those files") {
        init_echo_files_from_command_line({"clusters.echo",
                                           "timing_graph.analysis.echo"});

        CHECK(getEchoEnabled());
        for (int i = 0; i < (int)E_ECHO_END_TOKEN; i++) {
            e_echo_files echo_file = (e_echo_files)i;
            bool expected = (echo_file == E_ECHO_CLUSTERS || echo_file == E_ECHO_ANALYSIS_TIMING_GRAPH);
            CHECK(isEchoFileEnabled(echo_file) == expected);
        }
    }

    SECTION("A single echo file enables only that file") {
        init_echo_files_from_command_line({"clusters.echo"});

        CHECK(getEchoEnabled());
        for (int i = 0; i < (int)E_ECHO_END_TOKEN; i++) {
            e_echo_files echo_file = (e_echo_files)i;
            CHECK(isEchoFileEnabled(echo_file) == (echo_file == E_ECHO_CLUSTERS));
        }
    }

    SECTION("An unknown echo file name is an error") {
        REQUIRE_THROWS_AS(init_echo_files_from_command_line({"clusters.echo",
                                                             "not_a_real_file.echo"}),
                          VprError);
    }

    SECTION("On / off cannot be combined with echo file names") {
        REQUIRE_THROWS_AS(init_echo_files_from_command_line({"on", "clusters.echo"}),
                          VprError);
        free_echo_file_info();
        REQUIRE_THROWS_AS(init_echo_files_from_command_line({"clusters.echo", "off"}),
                          VprError);
    }

    free_echo_file_info();
}

} // namespace
