#include "arch_rule_gen.h"

#include "kernel/yosys.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <utility>

// rule generators and template substitution for vtr_arch_rules.
// templates live under vtr_flow/misc/mosaic/template/rules/ with:
//   @NAME@    scalar arch/policy values
//   @@NAME@@  computed snippets (e.g. one arm per bram/multiply mode)
USING_YOSYS_NAMESPACE

namespace mosaic {

namespace {

// ---------------------------------------------------------------------------
// file + substitution helpers
// ---------------------------------------------------------------------------

std::string replaceAll(std::string text, const std::string &from,
                       const std::string &to) {
  size_t pos = 0;
  while ((pos = text.find(from, pos)) != std::string::npos) {
    text.replace(pos, from.size(), to);
    pos += to.size();
  }
  return text;
}

std::string readTextFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in.is_open())
    log_cmd_error("vtr_arch_rules: cannot read template %s\n", path.c_str());
  std::ostringstream buf;
  buf << in.rdbuf();
  return buf.str();
}

bool templateFileExists(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  return in.is_open();
}

std::string loadTemplate(const ArchRulePolicy &policy,
                         const std::string &fileName) {
  if (policy.tplDir.empty())
    log_cmd_error("vtr_arch_rules: -tpldir <dir> is required (template %s)\n",
                  fileName.c_str());
  // per-arch overlay wins for individual files; missing names fall through
  // to the shared tplDir so rules/ is a merge, not a full replacement.
  if (!policy.overlayTplDir.empty()) {
    const std::string overlayPath = policy.overlayTplDir + "/" + fileName;
    if (templateFileExists(overlayPath))
      return readTextFile(overlayPath);
  }
  return readTextFile(policy.tplDir + "/" + fileName);
}

void writeFile(const std::string &path, const std::string &content) {
  std::ofstream out(path, std::ios::binary);
  if (!out.is_open())
    log_cmd_error("vtr_arch_rules: cannot write %s\n", path.c_str());
  out << content;
  if (!out.good())
    log_cmd_error("vtr_arch_rules: failed writing %s\n", path.c_str());
}

// substitute @@snippet@@ tokens first (their values get the template's
// newline convention), then @scalar@ tokens.
std::string
substituteTemplate(std::string text,
                   const std::map<std::string, std::string> &scalars,
                   const std::map<std::string, std::string> &snippets) {
  const bool crlf = text.find("\r\n") != std::string::npos;
  for (const auto &kv : snippets) {
    std::string value = kv.second;
    if (crlf)
      value = replaceAll(value, "\n", "\r\n");
    text = replaceAll(text, "@@" + kv.first + "@@", value);
  }
  for (const auto &kv : scalars)
    text = replaceAll(text, "@" + kv.first + "@", kv.second);

  // leftover @NAME@ means a template token was not provided by the emitter.
  const std::regex leftoverToken(R"(@[A-Za-z0-9_]+@)");
  std::smatch match;
  if (std::regex_search(text, match, leftoverToken))
    log_cmd_error("vtr_arch_rules: unsubstituted template token '%s' "
                  "(rebuild mosaic / check rule emitter scalars)\n",
                  match.str(0).c_str());
  return text;
}

std::string joinInts(const std::vector<int> &vals, const char *sep) {
  std::ostringstream out;
  for (size_t i = 0; i < vals.size(); ++i) {
    if (i)
      out << sep;
    out << vals[i];
  }
  return out.str();
}

// ---------------------------------------------------------------------------
// arch geometry helpers
// ---------------------------------------------------------------------------

std::vector<BramModeInfo>
filterModesByMinAbits(std::vector<BramModeInfo> modes, int minHardMemAbits) {
  if (minHardMemAbits <= 0)
    return modes;
  std::vector<BramModeInfo> kept;
  for (const auto &mode : modes) {
    if (mode.addrBitsA >= minHardMemAbits)
      kept.push_back(mode);
  }
  return kept;
}

std::vector<BramModeInfo> splitModes(const VtrArchInfo &info, bool sp) {
  std::vector<BramModeInfo> modes;
  for (const auto &mode : info.bramModes)
    if (mode.isSp == sp)
      modes.push_back(mode);
  return modes;
}

std::vector<BramModeInfo> sortByDataDesc(std::vector<BramModeInfo> modes) {
  std::sort(modes.begin(), modes.end(), [](const auto &a, const auto &b) {
    return a.dataBitsA > b.dataBitsA;
  });
  return modes;
}

std::vector<int> widthsOf(std::vector<BramModeInfo> modes) {
  std::sort(modes.begin(), modes.end(), [](const auto &a, const auto &b) {
    return a.dataBitsA < b.dataBitsA;
  });
  std::vector<int> widths;
  for (const auto &m : modes)
    if (std::find(widths.begin(), widths.end(), m.dataBitsA) == widths.end())
      widths.push_back(m.dataBitsA);
  return widths;
}

int maxAbitsOf(const std::vector<BramModeInfo> &modes) {
  int maxA = 0;
  for (const auto &m : modes)
    maxA = std::max(maxA, m.addrBitsA);
  return maxA;
}

// chain text for ADDR_BITS_DERIVED: first term is the overflow sentinel
// (-> 0, which _TECHMAP_FAIL_ rejects), then one term per mode pair, ending
// with the narrowest mode's addr bits.
std::string addrBitsChain(const char *widthParam,
                          const std::vector<BramModeInfo> &modesDesc) {
  int pad = 1;
  for (const auto &m : modesDesc)
    pad = std::max(pad, (int)std::to_string(m.dataBitsA).size());
  auto padAfter = [&](int data) {
    return std::string(pad - (int)std::to_string(data).size() + 1, ' ');
  };
  std::ostringstream out;
  out << "        (" << widthParam << " > " << modesDesc[0].dataBitsA << ")"
      << padAfter(modesDesc[0].dataBitsA) << "? 0 :\n";
  for (size_t i = 0; i + 1 < modesDesc.size(); ++i) {
    const int data = modesDesc[i + 1].dataBitsA;
    out << "        (" << widthParam << " > " << data << ")" << padAfter(data)
        << "? " << modesDesc[i].addrBitsA;
    out << (i + 2 == modesDesc.size() ? " : " : " :\n");
  }
  out << modesDesc.back().addrBitsA << ";";
  return out.str();
}

// MULT_W mode-selection ternary: smallest mode that fits both extended
// operands, widest mode as the fallthrough.
std::string multTernary(const std::vector<int> &modes) {
  std::ostringstream out;
  for (size_t i = 0; i + 1 < modes.size(); ++i)
    out << "(AW <= " << modes[i] << " && BW <= " << modes[i]
        << ") ? " << modes[i] << " : ";
  out << modes.back();
  return out.str();
}

// same ternary over a single combined width expression (mul2dsp's needW).
std::string multTernarySingle(const char *widthExpr,
                              const std::vector<int> &modes) {
  std::ostringstream out;
  for (size_t i = 0; i + 1 < modes.size(); ++i)
    out << "(" << widthExpr << " <= " << modes[i] << ") ? " << modes[i]
        << " : ";
  out << modes.back();
  return out.str();
}

// a stub section ends with exactly one blank line so sections concatenate.
std::string finishStub(std::string stub) {
  if (stub.empty() || stub.back() != '\n')
    stub += '\n';
  return stub + '\n';
}

// builtins already get stubs (or whiteboxes) from other generators.
bool isBuiltinHardblock(const std::string &name,
                        const ClassicModelNames &classic) {
  return name == classic.multiply || name == classic.adder ||
         name == classic.singlePortRam || name == classic.dualPortRam;
}

int maxBramDataWidth(const VtrArchInfo &info) {
  int maxWidth = 1;
  for (const auto &mode : info.bramModes)
    maxWidth = std::max(maxWidth, std::max(mode.dataBitsA, mode.dataBitsB));
  return maxWidth;
}

bool hasExoticHardblocks(const VtrArchInfo &info,
                         const ClassicModelNames &classic) {
  for (const auto &kv : info.hardblockModels)
    if (!isBuiltinHardblock(kv.first, classic))
      return true;
  return false;
}

void warnExoticOnlyMultiply(const VtrArchInfo &info,
                            const ClassicModelNames &classic) {
  if (info.multiply.present || !hasExoticHardblocks(info, classic))
    return;
  log_warning(
      "vtr_arch_rules: arch has exotic hardblock models but no classic "
      "'%s' model; inferred $mul will not map to exotics. use "
      "stubAllHardblocks / passthrough_exotics for rtl-instantiated "
      "exotic cells, exoticRoles / -exotic-role for role inference, or "
      "exoticTemplatePairs / -exotic for a per-model techmap.\n",
      classic.multiply.c_str());
}

// ---------------------------------------------------------------------------
// BramRuleGen (memory kind): bram_memory_map.txt + tech_bram.v
// ---------------------------------------------------------------------------

class BramRuleGen : public ArchRuleGen {
public:
  BramRuleGen() : ArchRuleGen("bram") {}

  void emit(const VtrArchInfo &info, const ArchRulePolicy &policy,
            const std::string &outDir) const override {
    std::vector<BramModeInfo> spModes =
        filterModesByMinAbits(splitModes(info, true), policy.minHardMemAbits);
    std::vector<BramModeInfo> dpModes =
        filterModesByMinAbits(splitModes(info, false), policy.minHardMemAbits);
    if (spModes.empty() || dpModes.empty()) {
      std::ostringstream msg;
      msg << "vtr_arch_rules: arch is missing required bram modes";
      if (spModes.empty())
        msg << " (no single_port_ram modes for model '"
            << policy.classic.singlePortRam << "'";
      if (spModes.empty() && policy.minHardMemAbits > 0)
        msg << " at abits>=" << policy.minHardMemAbits;
      if (spModes.empty())
        msg << ")";
      if (dpModes.empty())
        msg << " (no dual_port_ram modes for model '"
            << policy.classic.dualPortRam << "'";
      if (dpModes.empty() && policy.minHardMemAbits > 0)
        msg << " at abits>=" << policy.minHardMemAbits;
      if (dpModes.empty())
        msg << ")";
      msg << ". if the arch uses different ram model names, pass "
             "-alias single_port_ram=<model> and/or -alias dual_port_ram=<model> "
             "from arch_config.tcl; otherwise check the arch xml pb_types.\n";
      log_cmd_error("%s", msg.str().c_str());
    }
    if (policy.minHardMemAbits > 0) {
      log("vtr_arch_rules: minHardMemAbits=%d kept %zu sp / %zu dp bram modes\n",
          policy.minHardMemAbits, spModes.size(), dpModes.size());
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
              substituteTemplate(loadTemplate(policy, "bram_memory_map.txt.tmpl"),
                                 scalars, bramSnippets));
    writeFile(outDir + "/tech_bram.v",
              substituteTemplate(loadTemplate(policy, "tech_bram.v.tmpl"),
                                 scalars, techSnippets));
    writeFile(outDir + "/vtr_ram_whitebox.v",
              substituteTemplate(loadTemplate(policy, "vtr_ram_whitebox.v.tmpl"),
                                 scalars, {}));
    writeFile(outDir + "/vtr_ram_bit_lib.v",
              substituteTemplate(loadTemplate(policy, "vtr_ram_bit_lib.v.tmpl"),
                                 scalars, {}));
  }
};

// ---------------------------------------------------------------------------
// AdderRuleGen (combinational kind): add_sub_map.v + adder lib stub
// ---------------------------------------------------------------------------

// built-in always-fail map when hard $add/$sub binding is unavailable.
std::string alwaysFailAddSubMap(const std::string &archName,
                                const char *reasonLine) {
  std::ostringstream out;
  out << "// add_sub_map.v -- generated by vtr_arch_rules from " << archName
      << "\n";
  out << "// " << reasonLine << "\n";
  out << "// this map always fails so $add/$sub stay soft.\n";
  for (const char *op : {"$add", "$sub"}) {
    out << "module \\" << op << " (A, B, Y);\n";
    out << "    parameter A_SIGNED = 0;\n";
    out << "    parameter B_SIGNED = 0;\n";
    out << "    parameter A_WIDTH  = 1;\n";
    out << "    parameter B_WIDTH  = 1;\n";
    out << "    parameter Y_WIDTH  = 1;\n\n";
    out << "    input  [A_WIDTH-1:0] A;\n";
    out << "    input  [B_WIDTH-1:0] B;\n";
    out << "    output [Y_WIDTH-1:0] Y;\n\n";
    out << "    wire _TECHMAP_FAIL_ = 1'b1;\n";
    out << "endmodule\n";
  }
  return out.str();
}

class AdderRuleGen : public ArchRuleGen {
public:
  AdderRuleGen() : ArchRuleGen("adder") {}

  void emit(const VtrArchInfo &info, const ArchRulePolicy &policy,
            const std::string &outDir) const override {
    if (!info.adder.present) {
      log_warning("vtr_arch_rules: no adder model in arch; emitting "
                  "always-fail add_sub_map.v\n");
      writeFile(outDir + "/add_sub_map.v",
                alwaysFailAddSubMap(policy.archName,
                                    "arch has no adder model;"));
      return;
    }
    if (!info.adder.carryChain) {
      // non-carry / wide-adder pb_types are not first-class: soft-only, never
      // emit an illegal carry-chain map against a non-carry model.
      log_warning(
          "vtr_arch_rules: adder model '%s' is present but lacks carry-chain "
          "ports (cin/cout/sumout); soft-only $add/$sub (always-fail "
          "add_sub_map.v)\n",
          policy.classic.adder.c_str());
      writeFile(outDir + "/add_sub_map.v",
                alwaysFailAddSubMap(
                    policy.archName,
                    "adder model lacks carry-chain ports (cin/cout/sumout);"));
      return;
    }
    const std::map<std::string, std::string> scalars = {
        {"ARCH_NAME", policy.archName},
        {"HARD_ADDER_THRESHOLD", std::to_string(policy.hardAdderThreshold)},
        {"ADDER_MODEL", policy.classic.adder},
    };
    writeFile(outDir + "/add_sub_map.v",
              substituteTemplate(loadTemplate(policy, "add_sub_map.v.tmpl"),
                                 scalars, {}));
  }

  std::string hardblockStub(const VtrArchInfo &info,
                            const ArchRulePolicy &policy) const override {
    if (!info.adder.present)
      return {};
    const std::map<std::string, std::string> scalars = {
        {"ADDER_WIDTH", std::to_string(std::max(1, info.adder.aWidth))},
        {"ADDER_MODEL", policy.classic.adder},
    };
    return finishStub(substituteTemplate(
        loadTemplate(policy, "adder_stub.v.tmpl"), scalars, {}));
  }
};

// ---------------------------------------------------------------------------
// MultiplyRuleGen (combinational kind): mult_map.v + multiply lib stub
// ---------------------------------------------------------------------------

// built-in always-fail map for archs without a multiply model.
std::string alwaysFailMultMap(const std::string &archName) {
  std::ostringstream out;
  out << "// mult_map.v -- generated by vtr_arch_rules from " << archName << "\n";
  out << "// arch has no multiply model: this map always fails so $mul stays soft.\n";
  out << "module \\$mul (A, B, Y);\n";
  out << "    parameter A_SIGNED = 0;\n";
  out << "    parameter B_SIGNED = 0;\n";
  out << "    parameter A_WIDTH  = 1;\n";
  out << "    parameter B_WIDTH  = 1;\n";
  out << "    parameter Y_WIDTH  = 1;\n\n";
  out << "    input  [A_WIDTH-1:0] A;\n";
  out << "    input  [B_WIDTH-1:0] B;\n";
  out << "    output [Y_WIDTH-1:0] Y;\n\n";
  out << "    wire _TECHMAP_FAIL_ = 1'b1;\n";
  out << "endmodule\n";
  return out.str();
}

// built-in always-fail map for archs without a multiply model (dspMaxWidth
// should be 0 there so mul2dsp never builds a _dsp_block_).
std::string alwaysFailMul2dspMap(const std::string &archName) {
  std::ostringstream out;
  out << "// mul2dsp_map.v -- generated by vtr_arch_rules from " << archName
      << "\n";
  out << "// arch has no multiply model: this map always fails so\n";
  out << "// _dsp_block_ chunks never bind to a hardblock.\n";
  out << "module _dsp_block_ (A, B, Y);\n";
  out << "    parameter A_SIGNED = 0;\n";
  out << "    parameter B_SIGNED = 0;\n";
  out << "    parameter A_WIDTH  = 1;\n";
  out << "    parameter B_WIDTH  = 1;\n";
  out << "    parameter Y_WIDTH  = 1;\n\n";
  out << "    input  [A_WIDTH-1:0] A;\n";
  out << "    input  [B_WIDTH-1:0] B;\n";
  out << "    output [Y_WIDTH-1:0] Y;\n\n";
  out << "    wire _TECHMAP_FAIL_ = 1'b1;\n";
  out << "endmodule\n";
  return out.str();
}

class MultiplyRuleGen : public ArchRuleGen {
public:
  MultiplyRuleGen() : ArchRuleGen("multiply") {}

  void emit(const VtrArchInfo &info, const ArchRulePolicy &policy,
            const std::string &outDir) const override {
    if (!info.multiply.present || info.multiplyModes.empty()) {
      warnExoticOnlyMultiply(info, policy.classic);
      log_warning("vtr_arch_rules: no multiply model in arch; emitting "
                  "always-fail mult_map.v and mul2dsp_map.v\n");
      writeFile(outDir + "/mult_map.v", alwaysFailMultMap(policy.archName));
      writeFile(outDir + "/mul2dsp_map.v",
                alwaysFailMul2dspMap(policy.archName));
      return;
    }
    const int maxW = info.multiplyModes.back();
    const int minHardW = std::max(0, policy.minHardMulWidth);
    if (minHardW > 0) {
      log("vtr_arch_rules: minHardMulWidth=%d ($mul stays soft when both "
          "operand widths <= threshold)\n",
          minHardW);
    }
    const std::map<std::string, std::string> scalars = {
        {"ARCH_NAME", policy.archName},
        {"MULT_MAX_WIDTH", std::to_string(maxW)},
        {"MULT_MIN_WIDTH", std::to_string(minHardW)},
        {"MULT_MODEL", policy.classic.multiply},
    };
    const std::map<std::string, std::string> snippets = {
        {"MULT_TERNARY", multTernary(info.multiplyModes)},
    };
    writeFile(outDir + "/mult_map.v",
              substituteTemplate(loadTemplate(policy, "mult_map.v.tmpl"),
                                 scalars, snippets));
    writeFile(outDir + "/mul2dsp_map.v",
              substituteTemplate(loadTemplate(policy, "mul2dsp_map.v.tmpl"),
                                 scalars,
                                 {{"MULT_NEED_TERNARY",
                                   multTernarySingle("needW",
                                                     info.multiplyModes)}}));
  }

  std::string hardblockStub(const VtrArchInfo &info,
                            const ArchRulePolicy &policy) const override {
    if (!info.multiply.present || info.multiplyModes.empty())
      return {};
    const int maxW = info.multiplyModes.back();
    const std::map<std::string, std::string> scalars = {
        {"MULT_MAX_WIDTH", std::to_string(maxW)},
        {"MULT_PRODUCT_WIDTH", std::to_string(2 * maxW)},
        {"MULT_MODEL", policy.classic.multiply},
    };
    return finishStub(substituteTemplate(
        loadTemplate(policy, "multiply_stub.v.tmpl"), scalars, {}));
  }
};

// ---------------------------------------------------------------------------
// ExoticCombRuleGen (combinational kind): generic -exotic extension point
// ---------------------------------------------------------------------------

// token-safe port name: uppercase, non-alnum -> '_'
std::string tokenName(const std::string &port) {
  std::string out;
  for (char c : port)
    out += isalnum((unsigned char)c) ? (char)toupper((unsigned char)c) : '_';
  return out;
}

// generic blackbox stub built from the scanned port geometry (used when the
// exotic template carries no stub of its own). ports are emitted in sorted
// name order, inputs before outputs; 1-bit ports get no range.
std::string genericStub(const std::string &model, const ModelGeometry &geo) {
  std::ostringstream ports;
  bool first = true;
  auto port = [&](const char *dir, const std::string &name, int width) {
    if (!first)
      ports << ",\n";
    first = false;
    ports << "    " << dir << " ";
    if (width > 1)
      ports << "[" << (width - 1) << ":0] ";
    ports << name;
  };
  for (const auto &kv : geo.inputWidths)
    port("input", kv.first, kv.second);
  for (const auto &kv : geo.outputWidths)
    port("output", kv.first, kv.second);

  std::ostringstream out;
  out << "(* blackbox *)\n";
  out << "module " << model << " (\n";
  out << ports.str() << "\n";
  out << ");\n";
  out << "endmodule\n";
  return finishStub(out.str());
}

// identity techmap: yosys cell with the exotic model name maps to the
// same hardblock with 1:1 port connections.
std::string identityTechmapModule(const std::string &model,
                                  const ModelGeometry &geo) {
  std::ostringstream decl;
  std::ostringstream conn;
  bool firstDecl = true;
  bool firstConn = true;
  auto port = [&](const char *dir, const std::string &name, int width) {
    if (!firstDecl)
      decl << ",\n";
    firstDecl = false;
    decl << "    " << dir << " ";
    if (width > 1)
      decl << "[" << (width - 1) << ":0] ";
    decl << name;

    if (!firstConn)
      conn << ",\n";
    firstConn = false;
    conn << "        ." << name << "(" << name << ")";
  };
  for (const auto &kv : geo.inputWidths)
    port("input", kv.first, kv.second);
  for (const auto &kv : geo.outputWidths)
    port("output", kv.first, kv.second);

  std::ostringstream out;
  out << "// identity techmap for " << model << "\n";
  out << "module " << model << " (\n";
  out << decl.str() << "\n";
  out << ");\n\n";
  out << "    " << model << " _TECHMAP_REPLACE_ (\n";
  out << conn.str() << "\n";
  out << "    );\n";
  out << "endmodule\n\n";
  return out.str();
}

bool hasAbOutPorts(const ModelGeometry &geo) {
  return geo.inputWidths.count("a") && geo.inputWidths.count("b") &&
         geo.outputWidths.count("out");
}

std::vector<int> exoticMulModes(const ModelGeometry &geo) {
  if (!geo.modes.empty())
    return geo.modes;
  auto a = geo.inputWidths.find("a");
  if (a != geo.inputWidths.end() && a->second > 0)
    return {a->second};
  return {};
}

std::string alwaysFailExoticMap(const std::string &archName,
                                const std::string &model) {
  std::ostringstream out;
  out << "// " << model << "_map.v -- generated by vtr_arch_rules from "
      << archName << "\n";
  out << "// arch has no \"" << model << "\" hardblock model: this map always\n";
  out << "// fails so any matching cells stay soft.\n";
  out << "module \\" << model << " (A, B, Y);\n";
  out << "    parameter WIDTH = 1;\n";
  out << "    input  [WIDTH-1:0] A;\n";
  out << "    input  [WIDTH-1:0] B;\n";
  out << "    output [WIDTH-1:0] Y;\n";
  out << "    wire _TECHMAP_FAIL_ = 1'b1;\n";
  out << "endmodule\n";
  return out.str();
}

// builtins already get stubs (or whiteboxes) from other generators; skip
// them when stubbing every remaining hardblock model.
class StubAllExoticsRuleGen : public ArchRuleGen {
public:
  StubAllExoticsRuleGen() : ArchRuleGen("stub-all-exotics") {}

  void emit(const VtrArchInfo &info, const ArchRulePolicy &policy,
            const std::string &outDir) const override {
    std::ostringstream keep;
    std::ostringstream identityMaps;
    bool first = true;
    for (const auto &kv : info.hardblockModels) {
      if (isBuiltinHardblock(kv.first, policy.classic))
        continue;
      if (!first)
        keep << " ";
      first = false;
      keep << "t:" << kv.first;
      identityMaps << identityTechmapModule(kv.first, kv.second);
    }
    keep << "\n";
    writeFile(outDir + "/hardblock_keep_types.txt", keep.str());
    if (!identityMaps.str().empty()) {
      std::ostringstream header;
      header << "// exotic_identity_maps.v -- generated by vtr_arch_rules from "
             << policy.archName << "\n";
      header << "// identity passthrough for scanned exotic hardblocks\n\n";
      writeFile(outDir + "/exotic_identity_maps.v",
                header.str() + identityMaps.str());
    }
  }

  std::string hardblockStub(const VtrArchInfo &info,
                            const ArchRulePolicy &policy) const override {
    std::ostringstream stubs;
    for (const auto &kv : info.hardblockModels) {
      if (isBuiltinHardblock(kv.first, policy.classic))
        continue;
      stubs << genericStub(kv.first, kv.second);
    }
    return stubs.str();
  }
};

class ExoticCombRuleGen : public ArchRuleGen {
public:
  explicit ExoticCombRuleGen(ExoticRequest request)
      : ArchRuleGen(request.modelName), request_(std::move(request)) {}

  void emit(const VtrArchInfo &info, const ArchRulePolicy &policy,
            const std::string &outDir) const override {
    auto it = info.hardblockModels.find(request_.modelName);
    if (it == info.hardblockModels.end()) {
      log_warning("vtr_arch_rules: exotic model '%s' not in arch; emitting "
                  "always-fail map\n",
                  request_.modelName.c_str());
      writeFile(outDir + "/" + request_.modelName + "_map.v",
                alwaysFailExoticMap(policy.archName, request_.modelName));
      return;
    }
    const ModelGeometry &geo = it->second;

    std::string text = readTextFile(request_.templatePath);
    // optional first-line directive: "// output-name: <file>" (or "# ...")
    // picks the emitted file name; default is <model>_map.v.
    std::string outName = request_.modelName + "_map.v";
    const size_t eol = text.find('\n');
    const std::string firstLine =
        eol == std::string::npos ? text : text.substr(0, eol);
    const std::string directive = "output-name:";
    size_t dpos = firstLine.find(directive);
    if ((firstLine.rfind("//", 0) == 0 || firstLine.rfind("#", 0) == 0) &&
        dpos != std::string::npos) {
      outName = firstLine.substr(dpos + directive.size());
      while (!outName.empty() && isspace((unsigned char)outName.front()))
        outName.erase(outName.begin());
      while (!outName.empty() &&
             isspace((unsigned char)outName.back()))
        outName.pop_back();
      text.erase(0, eol == std::string::npos ? eol : eol + 1);
    }

    std::map<std::string, std::string> scalars = {
        {"ARCH_NAME", policy.archName},
        {"MODEL_NAME", request_.modelName},
        {"MAX_A_WIDTH",
         std::to_string(geo.modes.empty() ? 0 : geo.modes.back())},
        {"MODES", geo.modes.empty() ? "-" : joinInts(geo.modes, ",")},
    };
    for (const auto &kv : geo.inputWidths)
      scalars["PORT_IN_" + tokenName(kv.first)] = std::to_string(kv.second);
    for (const auto &kv : geo.outputWidths)
      scalars["PORT_OUT_" + tokenName(kv.first)] = std::to_string(kv.second);

    writeFile(outDir + "/" + outName, substituteTemplate(text, scalars, {}));
  }

  std::string hardblockStub(const VtrArchInfo &info,
                            const ArchRulePolicy &) const override {
    auto it = info.hardblockModels.find(request_.modelName);
    if (it == info.hardblockModels.end())
      return {};
    return genericStub(request_.modelName, it->second);
  }

private:
  ExoticRequest request_;
};

// ---------------------------------------------------------------------------
// RoleRuleGen (combinational kind): stock role templates under tpldir/roles/
// ---------------------------------------------------------------------------

class RoleRuleGen : public ArchRuleGen {
public:
  RoleRuleGen(ExoticRoleRequest request, std::string *emittedMapPath)
      : ArchRuleGen("role:" + request.modelName + ":" + request.roleName),
        request_(std::move(request)), emittedMapPath_(emittedMapPath) {}

  void emit(const VtrArchInfo &info, const ArchRulePolicy &policy,
            const std::string &outDir) const override {
    if (emittedMapPath_)
      emittedMapPath_->clear();

    if (request_.roleName == "integer_mul" && info.multiply.present &&
        !info.multiplyModes.empty()) {
      log("vtr_arch_rules: skipping integer_mul role for '%s' because classic "
          "multiply model '%s' is present\n",
          request_.modelName.c_str(), policy.classic.multiply.c_str());
      return;
    }

    auto it = info.hardblockModels.find(request_.modelName);
    if (it == info.hardblockModels.end()) {
      log_warning("vtr_arch_rules: exotic role model '%s' not in arch; "
                  "emitting always-fail map\n",
                  request_.modelName.c_str());
      const std::string outPath = outDir + "/" + request_.modelName + "_map.v";
      writeFile(outPath,
                alwaysFailExoticMap(policy.archName, request_.modelName));
      if (emittedMapPath_)
        *emittedMapPath_ = outPath;
      return;
    }
    const ModelGeometry &geo = it->second;

    if ((request_.roleName == "integer_mul" ||
         request_.roleName == "integer_mac") &&
        !hasAbOutPorts(geo)) {
      log_warning(
          "vtr_arch_rules: model '%s' lacks a/b/out ports required for role "
          "'%s'; emitting always-fail map\n",
          request_.modelName.c_str(), request_.roleName.c_str());
      const std::string outPath = outDir + "/" + request_.modelName + "_map.v";
      writeFile(outPath,
                alwaysFailExoticMap(policy.archName, request_.modelName));
      if (emittedMapPath_)
        *emittedMapPath_ = outPath;
      return;
    }

    if (policy.tplDir.empty())
      log_cmd_error("vtr_arch_rules: -tpldir <dir> is required for role '%s'\n",
                    request_.roleName.c_str());
    std::string text = loadTemplate(
        policy, "roles/" + request_.roleName + "_map.v.tmpl");

    const std::vector<int> mulModes = exoticMulModes(geo);
    const int maxW = mulModes.empty() ? 0 : mulModes.back();
    std::map<std::string, std::string> scalars = {
        {"ARCH_NAME", policy.archName},
        {"MODEL_NAME", request_.modelName},
        {"MAX_A_WIDTH", std::to_string(maxW)},
        {"MULT_MAX_WIDTH", std::to_string(maxW)},
        {"MULT_MIN_WIDTH", std::to_string(std::max(0, policy.minHardMulWidth))},
        {"MODES", mulModes.empty() ? "-" : joinInts(mulModes, ",")},
        {"HAS_ACC_PORT",
         geo.inputWidths.count("acc") ? "1" : "0"},
    };
    for (const auto &kv : geo.inputWidths)
      scalars["PORT_IN_" + tokenName(kv.first)] = std::to_string(kv.second);
    for (const auto &kv : geo.outputWidths)
      scalars["PORT_OUT_" + tokenName(kv.first)] = std::to_string(kv.second);

    std::map<std::string, std::string> snippets;
    if (!mulModes.empty()) {
      snippets["MULT_TERNARY"] = multTernary(mulModes);
      snippets["MULT_NEED_TERNARY"] = multTernarySingle("needW", mulModes);
    } else {
      snippets["MULT_TERNARY"] = "0";
      snippets["MULT_NEED_TERNARY"] = "0";
    }

    const std::string outPath = outDir + "/" + request_.modelName + "_map.v";
    writeFile(outPath, substituteTemplate(text, scalars, snippets));
    if (emittedMapPath_)
      *emittedMapPath_ = outPath;
  }

  std::string hardblockStub(const VtrArchInfo &info,
                            const ArchRulePolicy &) const override {
    auto it = info.hardblockModels.find(request_.modelName);
    if (it == info.hardblockModels.end())
      return {};
    return genericStub(request_.modelName, it->second);
  }

private:
  ExoticRoleRequest request_;
  std::string *emittedMapPath_;
};

// ---------------------------------------------------------------------------
// HardblockLibGen: aggregates combinational-kind stubs into vtr_hardblock_lib.v
// ---------------------------------------------------------------------------

class HardblockLibGen : public ArchRuleGen {
public:
  explicit HardblockLibGen(std::vector<const ArchRuleGen *> providers)
      : ArchRuleGen("hardblock-lib"), providers_(std::move(providers)) {}

  void emit(const VtrArchInfo &info, const ArchRulePolicy &policy,
            const std::string &outDir) const override {
    std::ostringstream stubs;
    for (const ArchRuleGen *gen : providers_)
      stubs << gen->hardblockStub(info, policy);

    const std::map<std::string, std::string> scalars = {
        {"ARCH_NAME", policy.archName},
        {"RAM_STUB_DATA_WIDTH", std::to_string(maxBramDataWidth(info))},
        {"SP_RAM_MODEL", policy.classic.singlePortRam},
        {"DP_RAM_MODEL", policy.classic.dualPortRam},
    };
    const std::map<std::string, std::string> snippets = {
        {"HARDBLOCK_STUBS", stubs.str()},
    };
    writeFile(outDir + "/vtr_hardblock_lib.v",
              substituteTemplate(loadTemplate(policy, "vtr_hardblock_lib.v.tmpl"),
                                 scalars, snippets));
  }

private:
  std::vector<const ArchRuleGen *> providers_;
};

} // namespace

void emitArchFacts(const VtrArchInfo &info, const ArchRulePolicy &policy,
                   const std::string &outDir) {
  int maxAbits = 0;
  for (const auto &m : info.bramModes)
    maxAbits = std::max(maxAbits, m.addrBitsA);

  const bool multiplyPresent =
      info.multiply.present && !info.multiplyModes.empty();
  const int dspMaxWidth =
      multiplyPresent ? info.multiplyModes.back() : 0;
  const int dspMinWidth =
      multiplyPresent ? info.multiplyModes.front() : 0;

  std::ostringstream modes;
  for (size_t i = 0; i < info.multiplyModes.size(); ++i) {
    if (i)
      modes << " ";
    modes << info.multiplyModes[i];
  }

  std::ostringstream facts;
  facts << "# generated by vtr_arch_rules\n";
  facts << "set archName \"" << policy.archName << "\"\n";
  facts << "set vtrRamAbits " << maxAbits << "\n";
  facts << "set dspMaxWidth " << dspMaxWidth << "\n";
  facts << "set dspMinWidth " << dspMinWidth << "\n";
  facts << "set multiplyPresent " << (multiplyPresent ? 1 : 0) << "\n";
  facts << "set adderPresent " << (info.adder.present ? 1 : 0) << "\n";
  facts << "set adderCarryChain " << (info.adder.carryChain ? 1 : 0) << "\n";
  facts << "set lutK " << info.lutK << "\n";
  facts << "set lutK1 " << info.lutK1 << "\n";
  facts << "set multiplyModes \"" << modes.str() << "\"\n";
  facts << "set spRamModel \"" << policy.classic.singlePortRam << "\"\n";
  facts << "set dpRamModel \"" << policy.classic.dualPortRam << "\"\n";
  facts << "set multiplyModel \"" << policy.classic.multiply << "\"\n";
  facts << "set adderModel \"" << policy.classic.adder << "\"\n";

  const std::string path = outDir + "/arch_facts.tcl";
  std::ofstream out(path, std::ios::binary);
  if (!out.is_open())
    log_cmd_error("vtr_arch_rules: cannot write %s\n", path.c_str());
  out << facts.str();
  if (!out.good())
    log_cmd_error("vtr_arch_rules: failed writing %s\n", path.c_str());
}

std::vector<std::unique_ptr<ArchRuleGen>> makeBuiltinRuleGens() {
  std::vector<std::unique_ptr<ArchRuleGen>> gens;
  gens.push_back(std::unique_ptr<ArchRuleGen>(new BramRuleGen()));
  gens.push_back(std::unique_ptr<ArchRuleGen>(new AdderRuleGen()));
  gens.push_back(std::unique_ptr<ArchRuleGen>(new MultiplyRuleGen()));
  return gens;
}

std::unique_ptr<ArchRuleGen> makeExoticRuleGen(const ExoticRequest &request) {
  return std::unique_ptr<ArchRuleGen>(new ExoticCombRuleGen(request));
}

std::unique_ptr<ArchRuleGen>
makeRoleRuleGen(const ExoticRoleRequest &request, std::string *emittedMapPath) {
  return std::unique_ptr<ArchRuleGen>(
      new RoleRuleGen(request, emittedMapPath));
}

std::unique_ptr<ArchRuleGen> makeStubAllExoticsGen() {
  return std::unique_ptr<ArchRuleGen>(new StubAllExoticsRuleGen());
}

std::unique_ptr<ArchRuleGen>
makeHardblockLibGen(std::vector<const ArchRuleGen *> providers) {
  return std::unique_ptr<ArchRuleGen>(
      new HardblockLibGen(std::move(providers)));
}

} // namespace mosaic
