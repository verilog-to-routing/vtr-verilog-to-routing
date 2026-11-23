#include "arch_rule_gen/internal.h"

#include "kernel/yosys.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <utility>

USING_YOSYS_NAMESPACE

namespace mosaic {
namespace arch_rule_detail {

// memory-kind generator: bram_memory_map.txt, tech_bram.v, and ram whiteboxes.

// HELPER: drop bram modes shallower than minHardMemAbits from libmap emission.
std::vector<BramModeInfo> filterModesByMinAbits(std::vector<BramModeInfo> modes, int minHardMemAbits) {
    if (minHardMemAbits <= 0)
        return modes;
    std::vector<BramModeInfo> kept;
    for (const auto &mode : modes) {
        if (mode.addrBitsA >= minHardMemAbits)
            kept.push_back(mode);
    }
    return kept;
}

// HELPER: split scanned bram modes into single-port or dual-port subsets.
std::vector<BramModeInfo> splitModes(const VtrArchInfo &info, bool sp) {
    std::vector<BramModeInfo> modes;
    for (const auto &mode : info.bramModes)
        if (mode.isSp == sp)
            modes.push_back(mode);
    return modes;
}

// HELPER: sort bram modes by descending data width for addr chain generation.
std::vector<BramModeInfo> sortByDataDesc(std::vector<BramModeInfo> modes) {
    std::sort(modes.begin(), modes.end(), [](const auto &a, const auto &b) { return a.dataBitsA > b.dataBitsA; });
    return modes;
}

// HELPER: collect distinct data widths from bram modes in ascending order.
std::vector<int> widthsOf(std::vector<BramModeInfo> modes) {
    std::sort(modes.begin(), modes.end(), [](const auto &a, const auto &b) { return a.dataBitsA < b.dataBitsA; });
    std::vector<int> widths;
    for (const auto &m : modes)
        if (std::find(widths.begin(), widths.end(), m.dataBitsA) == widths.end())
            widths.push_back(m.dataBitsA);
    return widths;
}

// HELPER: max address width across a bram mode list.
int maxAbitsOf(const std::vector<BramModeInfo> &modes) {
    int maxA = 0;
    for (const auto &m : modes)
        maxA = std::max(maxA, m.addrBitsA);
    return maxA;
}

// HELPER: build ADDR_BITS_DERIVED chain text. first term is the overflow
// sentinel (-> 0, which _TECHMAP_FAIL_ rejects), then one term per mode pair,
// ending with the narrowest mode's addr bits.
std::string addrBitsChain(const char *widthParam, const std::vector<BramModeInfo> &modesDesc) {
    int pad = 1;
    for (const auto &m : modesDesc)
        pad = std::max(pad, (int)std::to_string(m.dataBitsA).size());
    auto padAfter = [&](int data) { return std::string(pad - (int)std::to_string(data).size() + 1, ' '); };
    std::ostringstream out;
    out << "        (" << widthParam << " > " << modesDesc[0].dataBitsA << ")" << padAfter(modesDesc[0].dataBitsA)
        << "? 0 :\n";
    for (size_t i = 0; i + 1 < modesDesc.size(); ++i) {
        const int data = modesDesc[i + 1].dataBitsA;
        out << "        (" << widthParam << " > " << data << ")" << padAfter(data) << "? " << modesDesc[i].addrBitsA;
        out << (i + 2 == modesDesc.size() ? " : " : " :\n");
    }
    out << modesDesc.back().addrBitsA << ";";
    return out.str();
}

// HELPER: max data width across all scanned bram modes for ram stub sizing.
int maxBramDataWidth(const VtrArchInfo &info) {
    int maxWidth = 1;
    for (const auto &mode : info.bramModes)
        maxWidth = std::max(maxWidth, std::max(mode.dataBitsA, mode.dataBitsB));
    return maxWidth;
}

// memory-kind generator: bram_memory_map.txt and tech_bram.v.
class BramRuleGen : public ArchRuleGen {
  public:
    BramRuleGen() : ArchRuleGen("bram") {}

    // USE: emit bram libmap, techmap, and ram whitebox files for classic sp/dp modes.
    void emit(const VtrArchInfo &info, const ArchRulePolicy &policy, const std::string &outDir) const override {
        std::vector<BramModeInfo> spModes = filterModesByMinAbits(splitModes(info, true), policy.minHardMemAbits);
        std::vector<BramModeInfo> dpModes = filterModesByMinAbits(splitModes(info, false), policy.minHardMemAbits);
        if (spModes.empty() || dpModes.empty()) {
            if (policy.softOnlyMemory) {
                log_warning("vtr_arch_rules: no classic bram modes; softOnlyMemory set so "
                            "memories will soft-map (no hard bram libmap)\n");
                writeFile(outDir + "/soft_only_memory.txt",
                          "# no classic single_port_ram or dual_port_ram modes were found\n");
                return;
            }
            std::ostringstream msg;
            msg << "vtr_arch_rules: arch is missing required bram modes";
            if (spModes.empty())
                msg << " (no single_port_ram modes for model '" << policy.classic.singlePortRam << "'";
            if (spModes.empty() && policy.minHardMemAbits > 0)
                msg << " at abits>=" << policy.minHardMemAbits;
            if (spModes.empty())
                msg << ")";
            if (dpModes.empty())
                msg << " (no dual_port_ram modes for model '" << policy.classic.dualPortRam << "'";
            if (dpModes.empty() && policy.minHardMemAbits > 0)
                msg << " at abits>=" << policy.minHardMemAbits;
            if (dpModes.empty())
                msg << ")";
            msg << ". if the arch uses different ram model names, pass "
                   "-alias single_port_ram=<model> and/or -alias dual_port_ram=<model> "
                   "from arch_config.tcl; set softOnlyMemory 1 for soft-only "
                   "memories (e.g. titan); otherwise check the arch xml pb_types.\n";
            log_cmd_error("%s", msg.str().c_str());
        }
        // clear any leftover soft-only marker from a prior run on the classic path.
        std::remove((outDir + "/soft_only_memory.txt").c_str());
        if (policy.minHardMemAbits > 0) {
            log("vtr_arch_rules: minHardMemAbits=%d kept %zu sp / %zu dp bram modes\n", policy.minHardMemAbits,
                spModes.size(), dpModes.size());
        }

        int maxAbits = 0;
        for (const auto &m : info.bramModes)
            maxAbits = std::max(maxAbits, m.addrBitsA);

        const std::vector<BramModeInfo> spDesc = sortByDataDesc(spModes);
        const std::vector<BramModeInfo> dpDesc = sortByDataDesc(dpModes);

        const std::map<std::string, std::string> scalars = {
            {"ARCH_NAME", policy.archName},
            {"MAX_ABITS", std::to_string(maxAbits)},
            {"MAX_ABITS_M1", std::to_string(maxAbits - 1)},
            {"SP_MAX_ABITS", std::to_string(maxAbitsOf(spModes))},
            {"DP_MAX_ABITS", std::to_string(maxAbitsOf(dpModes))},
            {"SP_WIDTHS", joinInts(widthsOf(spModes), " ")},
            {"DP_WIDTHS", joinInts(widthsOf(dpModes), " ")},
            {"SP_COST", std::to_string(policy.spCost)},
            {"DP_COST", std::to_string(policy.dpCost)},
            {"SP_MAX_WIDTH", std::to_string(spDesc.front().dataBitsA)},
            {"DP_MAX_WIDTH", std::to_string(dpDesc.front().dataBitsA)},
            {"SP_RAM_MODEL", policy.classic.singlePortRam},
            {"DP_RAM_MODEL", policy.classic.dualPortRam},
        };
        const std::map<std::string, std::string> bramSnippets = {};
        const std::map<std::string, std::string> techSnippets = {
            {"SP_ADDR_CHAIN", addrBitsChain("PORT_A_WIDTH", spDesc)},
            {"DP_ADDR_CHAIN", addrBitsChain("PORT_A_WIDTH", dpDesc)},
        };

        writeFile(outDir + "/bram_memory_map.txt",
                  substituteTemplate(loadTemplate(policy, "bram_memory_map.txt.tmpl"), scalars, bramSnippets));
        writeFile(outDir + "/tech_bram.v",
                  substituteTemplate(loadTemplate(policy, "tech_bram.v.tmpl"), scalars, techSnippets));
        writeFile(outDir + "/vtr_ram_whitebox.v",
                  substituteTemplate(loadTemplate(policy, "vtr_ram_whitebox.v.tmpl"), scalars, {}));
        writeFile(outDir + "/vtr_ram_bit_lib.v",
                  substituteTemplate(loadTemplate(policy, "vtr_ram_bit_lib.v.tmpl"), scalars, {}));
    }
};

std::unique_ptr<ArchRuleGen> makeBramRuleGen() { return std::unique_ptr<ArchRuleGen>(new BramRuleGen()); }

} // namespace arch_rule_detail
} // namespace mosaic
