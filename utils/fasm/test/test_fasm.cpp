#include "catch.hpp"

#include "vpr_api.h"
#include "vtr_util.h"
#include "rr_metadata.h"
#include "fasm.h"
#include "fasm_utils.h"
#include "arch_util.h"
#include "rr_graph_writer.h"
#include <sstream>
#include <fstream>
#include <regex>
#include <cmath>
#include <algorithm>

static constexpr const char kArchFile[] = "test_fasm_arch.xml";
static constexpr const char kRrGraphFile[] = "test_fasm_rrgraph.xml";


namespace {

// ============================================================================

using Catch::Matchers::Equals;
using Catch::Matchers::StartsWith;
using Catch::Matchers::Contains;

class RegexMatcher : public Catch::MatcherBase<std::string> {

    std::string m_RegexStr;
    std::regex  m_Regex;

public:

    RegexMatcher (const std::string& a_Regex) : 
        m_RegexStr(a_Regex), m_Regex(a_Regex) {}

    bool match(const std::string& str) const override {
        std::smatch match;
        return std::regex_match(str, match, m_Regex);
    }

    virtual std::string describe() const override {
        return "does not match the regex '" + m_RegexStr + "'";
    }
};

inline RegexMatcher MatchesRegex (const std::string& a_Regex) {
    return RegexMatcher(a_Regex);
}

// ============================================================================


TEST_CASE("find_tags", "[fasm]") {
    const std::string feature ("{prefix}CLB_{loc}_{site}");

    auto tags = fasm::find_tags_in_feature (feature);

    REQUIRE (tags.size() == 3);
    REQUIRE (std::find(tags.begin(), tags.end(), "prefix") != tags.end());
    REQUIRE (std::find(tags.begin(), tags.end(), "loc")    != tags.end());
    REQUIRE (std::find(tags.begin(), tags.end(), "site")   != tags.end());
}

TEST_CASE("substitute_tags_correct", "[fasm]") {
    const std::string feature ("{prefix}CLB_{loc}_{site}");
    const std::map<const std::string, std::string> tags = {
        {"prefix", "L"},
        {"loc", "X7Y8"},
        {"site", "SLICE"}
    };

    auto result = fasm::substitute_tags(feature, tags);

    REQUIRE(result.compare("LCLB_X7Y8_SLICE") == 0);
}

TEST_CASE("substitute_tags_undef", "[fasm]") {
    const std::string feature ("{prefix}CLB_{loc}_{site}");
    const std::map<const std::string, std::string> tags = {
        {"loc", "X7Y8"},
        {"site", "SLICE"}
    };

    REQUIRE_THROWS(fasm::substitute_tags(feature, tags));
}

// ============================================================================

/*
VPR may rearrange LUT inputs and modify its content accordingly. This function
compares two LUT contents through applying every possible bit permutation. If
one matches then those two LUT contents are considered to define the same LUT
*/
bool match_lut_init(const std::string& a_BlifInit, const std::string& a_FasmInit) {

    // Strings differ in length, no match possible
    if (a_BlifInit.length() != a_FasmInit.length()) {
        return false;
    }

    // Skip "nn'b" at the beginning
    const std::string blifInit = a_BlifInit.substr(4);
    const std::string fasmInit = a_FasmInit.substr(4);

    // LUT bits
    size_t N = (size_t)log2f(blifInit.length());

    // Decode indices of individual ones.
    std::vector<int> blifOnes;
    std::vector<int> fasmOnes;
    for (size_t i=0; i<blifInit.length(); ++i) {
        size_t bit = blifInit.length() - 1 - i;
        if (blifInit[bit] == '1') {
            blifOnes.push_back(i);
        }
        if (fasmInit[bit] == '1') {
            fasmOnes.push_back(i);
        }
    }

    // Must have the same number of ones. If not the no match possible.
    if (blifOnes.size() != fasmOnes.size()) {
        return false;
    }

    // Initialize bit indices for permutations
    std::vector<int> indices(N);
    for (size_t i=0; i<N; ++i) {
        indices[i] = i;
    }

    // Check every possible permutation of LUT address bits
    do {

        // Compare ones positions
        bool match = true;
        for (size_t i=0; i<blifOnes.size(); ++i) {

            // Remap position according to current index permutation
            size_t fasmOne = 0;
            for (size_t k=0; k<indices.size(); ++k) {
                if (blifOnes[i] & (1<<k)) {
                    fasmOne |= (1<<indices[k]);
                }
            }

            // If doesn't match then no need to check further.
            if (std::find(fasmOnes.begin(), fasmOnes.end(), fasmOne) == fasmOnes.end())
            {
                match = false;
                break;
            }
        }

        // Got a match
        if (match) {
            return true;
        }

    } while (std::next_permutation(indices.begin(), indices.end()));

    return false;
}

TEST_CASE("match_lut_init", "[fasm]") {
    CHECK(match_lut_init("16'b0000000011111111", "16'b0000111100001111"));
}

TEST_CASE("fasm_integration_test", "[fasm]") {
    {
        t_vpr_setup vpr_setup;
        t_arch arch;
        t_options options;
        const char *argv[] = {
            "test_vpr",
            kArchFile,
            "wire.eblif",
            "--route_chan_width",
            "100",
        };
        vpr_init(sizeof(argv)/sizeof(argv[0]), argv,
                &options, &vpr_setup, &arch);
        bool flow_succeeded = vpr_flow(vpr_setup, arch);
        REQUIRE(flow_succeeded == true);

        auto &device_ctx = g_vpr_ctx.mutable_device();
        for(size_t inode = 0; inode < device_ctx.rr_nodes.size(); ++inode) {
            for(t_edge_size iedge = 0; iedge < device_ctx.rr_nodes[inode].num_edges(); ++iedge) {
                auto sink_inode = device_ctx.rr_nodes[inode].edge_sink_node(iedge);
                auto switch_id = device_ctx.rr_nodes[inode].edge_switch(iedge);
                vpr::add_rr_edge_metadata(inode, sink_inode, switch_id,
                        "fasm_features", vtr::string_fmt("%d_%d_%zu",
                            inode, sink_inode, switch_id));
            }
        }

        write_rr_graph(kRrGraphFile, vpr_setup.Segments);
        vpr_free_all(arch, vpr_setup);
    }

    t_vpr_setup vpr_setup;
    t_arch arch;
    t_options options;
    const char *argv[] = {
        "test_vpr",
        kArchFile,
        "wire.eblif",
        "--route_chan_width",
        "100",
        "--read_rr_graph",
        kRrGraphFile,
    };

    vpr_init(sizeof(argv)/sizeof(argv[0]), argv,
              &options, &vpr_setup, &arch);

    vpr_setup.PackerOpts.doPacking    = STAGE_LOAD;
    vpr_setup.PlacerOpts.doPlacement  = STAGE_LOAD;
    vpr_setup.RouterOpts.doRouting    = STAGE_LOAD;
    vpr_setup.AnalysisOpts.doAnalysis = STAGE_SKIP;

    bool flow_succeeded = vpr_flow(vpr_setup, arch);
    REQUIRE(flow_succeeded == true);

    std::stringstream fasm_string;
    fasm::FasmWriterVisitor visitor(fasm_string);
    NetlistWalker nl_walker(visitor);
    nl_walker.walk();

    // Write fasm to file
    fasm_string.seekg(0);
    std::ofstream fasm_file("output.fasm", std::ios_base::out);
    while(fasm_string) {
        std::string line;
        std::getline(fasm_string, line);
        fasm_file << line << std::endl;
    }

    // Load LUTs from the BLIF (comments)
    std::vector<std::string> lut_defs;
    std::string lut_def;
    std::ifstream blif_file("wire.eblif", std::ios_base::in);
    while (blif_file) {
        std::string line;
        std::getline(blif_file, line);

        if (!line.length()) {
            continue;
        }

        if (line.find("#") != std::string::npos) {
            auto init_pos = line.find("#");
            lut_def = line.substr(init_pos+2);
            continue;
        }

        if (line.find(".names") != std::string::npos) {
            REQUIRE(lut_def.length() > 0);
            lut_defs.push_back(lut_def);
            //fprintf(stderr, "'%s'\n", lut_def.c_str());
            lut_def = "";
            continue;
        }
    }

    // Verify fasm
    fasm_string.clear();
    fasm_string.seekg(0);

    std::set<std::tuple<int, int, short>> routing_edges;
    std::set<std::tuple<int, int>> occupied_locs;
    bool found_lut5 = false;
    bool found_lut6 = false;
    bool found_mux1 = false;
    bool found_mux2 = false;
    bool found_mux3 = false;
    while(fasm_string) {
        // Should see something like:
        // CLB.FLE0_X1Y1.N2_LUT5
        // PLB.FLE1_X2Y1.LUT5_1.LUT[31:0]=32'b00000000000000010000000000000000
        // FLE0_X3Y1.OUT_MUX.LUT
        // SLICE.FLE1_X4Y1.DISABLE_FF
        // CLB.FLE0_X1Y3.LUT6[63:0]=64'b0000000000000000000000000000000100000000000000000000000000000000
        // 3634_3690_0
        std::string line;
        std::getline(fasm_string, line);

        if(line == "") {
            continue;
        }

        // Look for muxes
        {
            std::smatch m;
            if (std::regex_match(line, m, std::regex(".*FLE\\d?_X\\d+Y\\d+.OUT_MUX.LUT$"))) {
                found_mux1 = true;
            }
            if (std::regex_match(line, m, std::regex(".*FLE\\d?_X\\d+Y\\d+.DISABLE_FF$"))) {
                found_mux2 = true;
            }
            if (std::regex_match(line, m, std::regex(".*FLE\\d?_X\\d+Y\\d+.IN\\d{1}$"))) {
                found_mux3 = true;
            }
        }

        if(line.find("FLE") != std::string::npos) {

            // Check correlation with top-level prefixes with X coordinates
            // as defined in the architecture
            if(line.find("_X1") != std::string::npos) {
                CHECK_THAT(line, StartsWith("CLB"));
            }
            if(line.find("_X2") != std::string::npos) {
                CHECK_THAT(line, StartsWith("PLB"));
            }
            if(line.find("_X3") != std::string::npos) {
                // X3 has no top-level prefix, so the intermediate one is the
                // first
                CHECK_THAT(line, StartsWith("FLE"));
            }
            if(line.find("_X4") != std::string::npos) {
                CHECK_THAT(line, StartsWith("SLICE"));
            }

            // Check presence of LOC prefix substitutions
            CHECK_THAT(line, MatchesRegex(".*X\\d+Y\\d+.*"));

            // Extract loc from tag
            std::smatch locMatch;
            REQUIRE(std::regex_match(line, locMatch, std::regex(".*X(\\d+)Y(\\d+).*")));
            REQUIRE(locMatch.size() == 3);
            int loc_x = vtr::atoi(locMatch[1].str());
            int loc_y = vtr::atoi(locMatch[2].str());
            occupied_locs.insert(std::make_tuple(loc_x, loc_y));

            // Check presence of ALUT_{LR} with substituted L/R
            if(line.find("ALUT") != std::string::npos) {
                CHECK_THAT(line, MatchesRegex(".*ALUT_[LR]{1}.*"));
            }

            // Check correct substitution of "" and "_SING"
            if (loc_y == 1) {
                CHECK_THAT(line,  Contains("_SING"));
            }
            else {
                CHECK_THAT(line, !Contains("_SING"));
            }

            // Check that all tags were substituted
            CHECK_THAT(line, !Contains("{") && !Contains("}"));

            // Check LUT
            auto pos = line.find("LUT[");
            if(pos != std::string::npos) {
                auto str = line.substr(pos);
                std::regex  regex("LUT\\[31:0\\]=(32'b[01]{32})$");
                std::smatch match;

                if (std::regex_match(str, match, regex)) {
                    bool found = false;
                    for (auto itr=lut_defs.begin(); itr!=lut_defs.end(); itr++) {
                        if (match_lut_init(*itr, match[1].str())) {
                            found = true;
                            lut_defs.erase(itr);
                            break;
                        }
                    }

                    if (!found) {
                        FAIL_CHECK("LUT definition '" << match[1] << "' not found in the BLIF file!");
                    } else {
                        found_lut5 = true;
                    }
                }
                else {
                    FAIL_CHECK("LUT definition '" << line << "' is malformed!");
                }
            }

            // Check LUT6
            pos = line.find("LUT6[");
            if(pos != std::string::npos) {
                auto str = line.substr(pos);
                std::regex  regex("LUT6\\[63:0\\]=(64'b[01]{64})$");
                std::smatch match;

                if (std::regex_match(str, match, regex)) {
                    bool found = false;
                    for (auto itr=lut_defs.begin(); itr!=lut_defs.end(); itr++) {
                        if (match_lut_init(*itr, match[1].str())) {
                            found = true;
                            lut_defs.erase(itr);
                            break;
                        }
                    }

                    if (!found) {
                        FAIL_CHECK("LUT definition '" << match[1] << "' not found in the BLIF file!");
                    } else {
                        found_lut6 = true;
                    }
                }
                else {
                    FAIL_CHECK("LUT definition '" << line << "' is malformed!");
                }
            }

        } else {
            auto parts = vtr::split(line, "_");
            REQUIRE(parts.size() == 3);
            auto src_inode = vtr::atoi(parts[0]);
            auto sink_inode = vtr::atoi(parts[1]);
            auto switch_id = vtr::atoi(parts[2]);

            auto ret = routing_edges.insert(std::make_tuple(src_inode, sink_inode, switch_id));
            CHECK(ret.second == true);
        }
    }

    // Verify occupied grid LOCs
    const auto & place_ctx = g_vpr_ctx.placement();
    for (const auto& loc: place_ctx.block_locs) {

        // Do not consider "IOB" tiles. They do not have fasm features
        // defined in the arch.
        if (loc.loc.x < 1 || loc.loc.x > 4)
            continue;
        if (loc.loc.y < 1 || loc.loc.y > 4)
            continue;

        auto iter = occupied_locs.find(std::make_tuple(loc.loc.x, loc.loc.y));
        if (iter == occupied_locs.end()) {
            FAIL_CHECK("X" << loc.loc.x << "Y" << loc.loc.y << " not emitted!");
        }
    }

    // Verify routes
    const auto & route_ctx = g_vpr_ctx.routing();
    for(const auto &trace : route_ctx.trace) {
        const t_trace *head = trace.head;
        while(head != nullptr) {
            const t_trace *next = head->next;

            if(next != nullptr && head->iswitch != OPEN) {
                const auto next_inode = next->index;
                auto iter = routing_edges.find(std::make_tuple(head->index, next_inode, head->iswitch));
                if (iter == routing_edges.end()) {
                    FAIL_CHECK("source: " << head->index << " sink: " << next_inode << " switch:" << head->iswitch);
                }
            }

            head = next;
        }
    }

    // Verify that all LUTs defined in the BLIF file ended up in fasm
    CHECK(lut_defs.size() == 0);

    CHECK(found_lut5);
    CHECK(found_lut6);
    CHECK(found_mux1);
    CHECK(found_mux2);
    CHECK(found_mux3);

    vpr_free_all(arch, vpr_setup);
}

} // namespace
