#include "arch_rule_gen.h"

#include "kernel/yosys.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <sstream>
#include <utility>

// concrete rule generators + the template substitution engine behind
// vtr_arch_rules. the emitters are template-backed: template files live in
// the arch family's templates dir (k6: frankenstein/yosys/k6/templates) and
// carry two token flavours:
//   @NAME@    scalar arch/policy values (widths, costs, arch name)
//   @@NAME@@  computed snippets (data-dependent code: N modes -> N arms)
// snippet text is built here with the same logic the old string-stream
// emitters used; everything else is verbatim template text, so k6 output
// stays byte-identical to the committed statics (header comment aside).
USING_YOSYS_NAMESPACE

namespace wildebeestVtr {

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

std::string loadTemplate(const ArchRulePolicy &policy,
                         const std::string &fileName) {
  if (policy.tplDir.empty())
    log_cmd_error("vtr_arch_rules: -tpldir <dir> is required (template %s)\n",
                  fileName.c_str());
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
// arch geometry helpers (same semantics as the legacy emitters)
// ---------------------------------------------------------------------------

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
// with the narrowest mode's addr bits. the '?' is column-aligned to the
// widest data value (matches the committed k6 map).
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

// a stub section ends with exactly one blank line so sections concatenate.
std::string finishStub(std::string stub) {
  if (stub.empty() || stub.back() != '\n')
    stub += '\n';
  return stub + '\n';
}

// ---------------------------------------------------------------------------
// BramRuleGen (memory kind): bram_memory_map.txt + tech_bram.v
// ---------------------------------------------------------------------------

class BramRuleGen : public ArchRuleGen {
public:
  BramRuleGen() : ArchRuleGen("bram") {}

  void emit(const VtrArchInfo &info, const ArchRulePolicy &policy,
            const std::string &outDir) const override {
    std::vector<BramModeInfo> spModes = splitModes(info, true);
    std::vector<BramModeInfo> dpModes = splitModes(info, false);
    if (spModes.empty() || dpModes.empty())
      log_cmd_error("vtr_arch_rules: arch has no sp/dp bram modes\n");

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
  }
};

// ---------------------------------------------------------------------------
// AdderRuleGen (comb kind): lib stub only; add_sub_map.v stays static
// ---------------------------------------------------------------------------

class AdderRuleGen : public ArchRuleGen {
public:
  AdderRuleGen() : ArchRuleGen("adder") {}

  void emit(const VtrArchInfo &info, const ArchRulePolicy &,
            const std::string &) const override {
    if (!info.adder.present)
      log_warning("vtr_arch_rules: no adder model in arch; adder stub omitted\n");
  }

  std::string hardblockStub(const VtrArchInfo &info,
                            const ArchRulePolicy &policy) const override {
    if (!info.adder.present)
      return {};
    const std::map<std::string, std::string> scalars = {
        {"ADDER_WIDTH", std::to_string(std::max(1, info.adder.aWidth))},
    };
    return finishStub(substituteTemplate(
        loadTemplate(policy, "adder_stub.v.tmpl"), scalars, {}));
  }
};

// ---------------------------------------------------------------------------
// MultiplyRuleGen (comb kind): mult_map.v + multiply lib stub
// ---------------------------------------------------------------------------

// built-in always-fail map for archs without a multiply model (not part of
// the k6 byte-parity target, so it needs no template).
std::string alwaysFailMultMap(const std::string &archName) {
  std::ostringstream out;
  out << "// mult_map.v -- generated by vtr_arch_rules from " << archName << "\n";
  out << "// techmap: yosys $mul -> vtr multiply primitive\n";
  out << "//\n";
  out << "// multiply modes detected in arch: none (this map always fails; "
         "$mul stays soft)\n";
  out << "//\n";
  out << "// unsigned multiplication: zero-extend operands, multiply directly,\n";
  out << "// truncate output.  signed multiplication sign-extends both operands\n";
  out << "// one extra bit; the extended width must still fit one block.\n\n";
  out << "module \\$mul (A, B, Y);\n";
  out << "    parameter A_SIGNED = 0;\n";
  out << "    parameter B_SIGNED = 0;\n";
  out << "    parameter A_WIDTH  = 1;\n";
  out << "    parameter B_WIDTH  = 1;\n";
  out << "    parameter Y_WIDTH  = 1;\n\n";
  out << "    input  [A_WIDTH-1:0] A;\n";
  out << "    input  [B_WIDTH-1:0] B;\n";
  out << "    output [Y_WIDTH-1:0] Y;\n\n";
  out << "    // for signed operands we need one extra bit per operand for sign "
         "extension.\n";
  out << "    localparam AW_EXT = (A_SIGNED != 0) ? A_WIDTH + 1 : A_WIDTH;\n";
  out << "    localparam BW_EXT = (B_SIGNED != 0) ? B_WIDTH + 1 : B_WIDTH;\n\n";
  out << "    // arch has no multiply hardblock: always fail so $mul stays soft.\n";
  out << "    wire _TECHMAP_FAIL_ = 1'b1;\n\n";
  out << "    localparam AW = AW_EXT;\n";
  out << "    localparam BW = BW_EXT;\n";
  out << "    // choose the smallest multiply mode that fits both extended "
         "operands\n";
  out << "    localparam MULT_W = 1;\n\n";
  out << "    // sign-extend or zero-extend A to MULT_W bits\n";
  out << "    wire [AW-1:0] a_ext;\n";
  out << "    generate\n";
  out << "        if (A_SIGNED)\n";
  out << "            assign a_ext = {{(AW - A_WIDTH){A[A_WIDTH-1]}}, A};\n";
  out << "        else\n";
  out << "            assign a_ext = {{(AW - A_WIDTH){1'b0}}, A};\n";
  out << "    endgenerate\n";
  out << "    wire [MULT_W-1:0] aa = {{(MULT_W - AW){1'b0}}, a_ext};\n\n";
  out << "    // sign-extend or zero-extend B to MULT_W bits\n";
  out << "    wire [BW-1:0] b_ext;\n";
  out << "    generate\n";
  out << "        if (B_SIGNED)\n";
  out << "            assign b_ext = {{(BW - B_WIDTH){B[B_WIDTH-1]}}, B};\n";
  out << "        else\n";
  out << "            assign b_ext = {{(BW - B_WIDTH){1'b0}}, B};\n";
  out << "    endgenerate\n";
  out << "    wire [MULT_W-1:0] bb = {{(MULT_W - BW){1'b0}}, b_ext};\n\n";
  out << "    wire [2*MULT_W-1:0] fullOut;\n\n";
  out << "    multiply #(.WIDTH(MULT_W)) _TECHMAP_REPLACE_ (\n";
  out << "        .a   (aa),\n";
  out << "        .b   (bb),\n";
  out << "        .out (fullOut)\n";
  out << "    );\n\n";
  out << "    assign Y = fullOut[Y_WIDTH-1:0];\n\n";
  out << "endmodule\n";
  return out.str();
}

class MultiplyRuleGen : public ArchRuleGen {
public:
  MultiplyRuleGen() : ArchRuleGen("multiply") {}

  void emit(const VtrArchInfo &info, const ArchRulePolicy &policy,
            const std::string &outDir) const override {
    if (!info.multiply.present || info.multiplyModes.empty()) {
      log_warning("vtr_arch_rules: no multiply model in arch; emitting "
                  "always-fail mult_map.v\n");
      writeFile(outDir + "/mult_map.v", alwaysFailMultMap(policy.archName));
      return;
    }
    const int maxW = info.multiplyModes.back();
    const std::map<std::string, std::string> scalars = {
        {"ARCH_NAME", policy.archName},
        {"MULT_MAX_WIDTH", std::to_string(maxW)},
    };
    const std::map<std::string, std::string> snippets = {
        {"MULT_TERNARY", multTernary(info.multiplyModes)},
    };
    writeFile(outDir + "/mult_map.v",
              substituteTemplate(loadTemplate(policy, "mult_map.v.tmpl"),
                                 scalars, snippets));
  }

  std::string hardblockStub(const VtrArchInfo &info,
                            const ArchRulePolicy &policy) const override {
    if (!info.multiply.present || info.multiplyModes.empty())
      return {};
    const int maxW = info.multiplyModes.back();
    const std::map<std::string, std::string> scalars = {
        {"MULT_MAX_WIDTH", std::to_string(maxW)},
        {"MULT_PRODUCT_WIDTH", std::to_string(2 * maxW)},
    };
    return finishStub(substituteTemplate(
        loadTemplate(policy, "multiply_stub.v.tmpl"), scalars, {}));
  }
};

// ---------------------------------------------------------------------------
// ExoticCombRuleGen (comb kind): generic -exotic extension point
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
// HardblockLibGen: aggregates comb-kind stubs into vtr_hardblock_lib.v
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
makeHardblockLibGen(std::vector<const ArchRuleGen *> providers) {
  return std::unique_ptr<ArchRuleGen>(
      new HardblockLibGen(std::move(providers)));
}

} // namespace wildebeestVtr
