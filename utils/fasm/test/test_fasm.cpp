#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_all.hpp"

#include "vpr_api.h"
#include "vtr_util.h"
#include "rr_metadata.h"
#include "fasm.h"
#include "fasm_utils.h"
#include "arch_util.h"
#include "rr_graph_writer.h"
#include "post_routing_pb_pin_fixup.h"
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
using Catch::Matchers::ContainsSubstring;

class RegexMatcher : public Catch::Matchers::MatcherBase<std::string> {

    std::string m_RegexStr;
    std::regex  m_Regex;

public:

    RegexMatcher (const std::string& a_Regex) : 
        m_RegexStr(a_Regex), m_Regex(a_Regex) {}

    bool match(const std::string& str) const override {
        std::smatch smatch;
        return std::regex_match(str, smatch, m_Regex);
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

/*
The following function returns a string describing a block pin given an IPIN/
OPIN rr node index. This is needed to correlate rr nodes with port names as
defined in the architecture and used for checking if genfasm correctlty honors
equivalent port pins rotation.

The output pin description format is:
"PIN_<xlow>_<ylow>_<sub_tile_type>_<sub_tile_port>_<port_pin_index>"

Pin decriptions returned by this functions are injected as FASM features to the
edges of a rr graph that are immediately connected with pins from "outside"
(not to from/to a SOURCE or SINK). Then, after genfasm is run they are identified,
and decoded to get all the pin information. This allows to get information
which block pins are used from the "FASM perspective".
*/
static std::string get_pin_feature (size_t inode) {
    auto& device_ctx = g_vpr_ctx.device();
    const auto& rr_graph = device_ctx.rr_graph;

    // Get tile physical tile and the pin number
    int ilow = rr_graph.node_xlow(RRNodeId(inode));
    int jlow = rr_graph.node_ylow(RRNodeId(inode));
    auto physical_tile = device_ctx.grid[ilow][jlow].type;
    int pin_num = rr_graph.node_pin_num(RRNodeId(inode));

    // Get the sub tile (type, not instance) and index of its pin that matches
    // the node index.
    const t_sub_tile* sub_tile_type = nullptr;
    int sub_tile_pin = -1;

    for (auto& sub_tile : physical_tile->sub_tiles) {
        auto max_inst_pins = sub_tile.num_phy_pins / sub_tile.capacity.total();
        for (int pin = 0; pin < sub_tile.num_phy_pins; pin++) {
            if (sub_tile.sub_tile_to_tile_pin_indices[pin] == pin_num) {
                sub_tile_type = &sub_tile;
                sub_tile_pin  = pin % max_inst_pins;
                break;
            }
        }

        if (sub_tile_type != nullptr) {
            break;
        }
    }

    REQUIRE(sub_tile_type != nullptr);
    REQUIRE(sub_tile_pin  != -1);

    // Find the sub tile port and pin index for the sub-tile type pin index.
    for (const auto& port : sub_tile_type->ports) {
        int pin_lo = port.absolute_first_pin_index;
        int pin_hi = pin_lo + port.num_pins;

        if (sub_tile_pin >= pin_lo && sub_tile_pin < pin_hi) {
            int port_pin = sub_tile_pin - pin_lo;
            return vtr::string_fmt("PIN_%d_%d_%s_%s_%d", ilow, jlow, sub_tile_type->name, port.name, port_pin);
        }
    }

    // Pin not found
    REQUIRE(false);
    return std::string();
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
        vpr_setup.RouterOpts.read_rr_edge_metadata = true;
        bool flow_succeeded = vpr_flow(vpr_setup, arch);
        REQUIRE(flow_succeeded == true);

        auto &device_ctx = g_vpr_ctx.mutable_device();
        const auto& rr_graph = device_ctx.rr_graph;
        for (const RRNodeId& inode : rr_graph.nodes()){
            for(t_edge_size iedge = 0; iedge < rr_graph.num_edges(inode); ++iedge) {
                auto sink_inode = size_t(rr_graph.edge_sink_node(inode, iedge));
                auto switch_id = rr_graph.edge_switch(inode, iedge);
                auto value = vtr::string_fmt("%d_%d_%zu",
                            (size_t)inode, sink_inode, switch_id);

                // Add additional features to edges that go to CLB.I[11:0] pins
                // to correlate them with features of CLB input mux later.
                auto sink_type = rr_graph.node_type(RRNodeId(sink_inode));
                if (sink_type == IPIN) {            
                    auto pin_feature = get_pin_feature(sink_inode);
                    value = value + "\n" + pin_feature;
                }

                vpr::add_rr_edge_metadata((size_t)inode, sink_inode, switch_id,
                        vtr::string_view("fasm_features"), vtr::string_view(value.data(), value.size()));
            }
        }

        write_rr_graph(kRrGraphFile);
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
    vpr_setup.RouterOpts.read_rr_edge_metadata = true;
    vpr_setup.AnalysisOpts.doAnalysis = STAGE_SKIP;

    bool flow_succeeded = vpr_flow(vpr_setup, arch);
    REQUIRE(flow_succeeded == true);

    /* Sync netlist to the actual routing (necessary if there are block
       ports with equivalent pins) */
    if (flow_succeeded) {
        sync_netlists_to_routing(g_vpr_ctx.device(),
                                 g_vpr_ctx.mutable_atom(),
                                 g_vpr_ctx.mutable_clustering(),
                                 g_vpr_ctx.placement(),
                                 g_vpr_ctx.routing(),
                                 vpr_setup.PackerOpts.pack_verbosity > 2);
    }

    std::stringstream fasm_string;
    fasm::FasmWriterVisitor visitor(&arch.strings, fasm_string);
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
            lut_def = "";
            continue;
        }
    }

    // Verify fasm
    fasm_string.clear();
    fasm_string.seekg(0);

    std::set<std::string> xbar_features;
    std::set<std::tuple<int, int, std::string, std::string, int>> routed_pins; // x, y, tile, port, pin
    std::set<std::tuple<int, int, short>> routing_edges;
    std::set<std::tuple<int, int>> occupied_locs;
    bool found_lut5 = false;
    bool found_lut6 = false;
    bool found_mux1 = false;
    bool found_mux2 = false;
    bool found_mux3 = false;
    bool found_mux4 = false;
    while(fasm_string) {
        // Should see something like:
        // CLB.FLE0_X1Y1.N2_LUT5
        // PLB.FLE1_X2Y1.LUT5_1.LUT[31:0]=32'b00000000000000010000000000000000
        // FLE0_X3Y1.OUT_MUX.LUT
        // SLICE.FLE1_X4Y1.DISABLE_FF
        // CLB.FLE0_X1Y3.LUT6[63:0]=64'b0000000000000000000000000000000100000000000000000000000000000000
        // CLB.FLE0_X1Y3.IN0_XBAR_I0
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
            if (std::regex_match(line, m, std::regex(".*FLE\\d?_X\\d+Y\\d+\\.IN0_XBAR_(I(\\d|1[01])|FLE[01]_OUT[01])$"))) {
                found_mux4 = true;
            }
        }

        // A feature representing block pin used by the router
        if(line.find("PIN_") != std::string::npos) {
            auto parts = vtr::split(line, "_");
            REQUIRE(parts.size() == 6);

            auto x = vtr::atoi(parts[1]);
            auto y = vtr::atoi(parts[2]);
            auto tile = parts[3];
            auto port = parts[4];
            auto pin = vtr::atoi(parts[5]);

            routed_pins.insert(std::make_tuple(x, y, tile, port, pin));
        }

        else if(line.find("FLE") != std::string::npos) {

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

            // Add to xbar features
            if (line.find("_XBAR_") != std::string::npos) {
                xbar_features.insert(line);
            }

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
                CHECK_THAT(line,  ContainsSubstring("_SING"));
            }
            else {
                CHECK_THAT(line, !ContainsSubstring("_SING"));
            }

            // Check that all tags were substituted
            CHECK_THAT(line, !ContainsSubstring("{") && !ContainsSubstring("}"));

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

            // Check "FLE" does not appear twice
            int FLE_occurrences = 0;
            pos = 0;
            while ((pos = line.find("FLE", pos)) != std::string::npos) {
                ++ FLE_occurrences;
                pos += 3;   // length of substring "FLE"
            }
            CHECK(FLE_occurrences <= 1);

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

    // Verify CLB crossbar mux features against routed pin features.
    for (const auto& xbar_feature : xbar_features) {

        // Decompose the xbar feature - extract only the necessary information
        // such as block location and pin index.
        std::smatch m;
        auto res = std::regex_match(xbar_feature, m, std::regex(
            ".*_X([0-9]+)Y([0-9]+)\\.IN([0-9])_XBAR_I([0-9]+)$"));
        REQUIRE(res == true);

        int x = vtr::atoi(m.str(1));
        int y = vtr::atoi(m.str(2));
        std::string mux = m.str(3);
        int pin = vtr::atoi(m.str(4));

        // Check if there is a corresponding routed pin feature
        auto pin_feature = std::make_tuple(x, y, std::string("clb"), std::string("I"), pin);
        size_t count = routed_pins.count(pin_feature);
        REQUIRE(count == 1);
    }

    // Verify that all LUTs defined in the BLIF file ended up in fasm
    CHECK(lut_defs.size() == 0);

    CHECK(found_lut5);
    CHECK(found_lut6);
    CHECK(found_mux1);
    CHECK(found_mux2);
    CHECK(found_mux3);
    CHECK(found_mux4);

    vpr_free_all(arch, vpr_setup);
}

} // namespace
