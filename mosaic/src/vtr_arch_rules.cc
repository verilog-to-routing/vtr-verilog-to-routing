#include "arch_rule_gen.h"
#include "vtr_arch_info.h"

#include "kernel/yosys.h"

#include <cstdlib>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// vtr_arch_rules: generate the arch-specific mapping rules for the
// mosaic flow at synthesis time, directly from the vtr arch xml.

// thin cli over the rule-generator registry in arch_rule_gen.{h,cc}: each
// enabled generator substitutes arch-derived values into template files
// (-tpldir) and writes rule files into -outdir:
//   bram_memory_map.txt   memory_libmap rules for the arch's bram modes
//   tech_bram.v           recursive bit-sliced bram techmap
//   vtr_hardblock_lib.v   -lib stubs sized from the arch hardblock models
//   mult_map.v            $mul -> multiply techmap with arch mode widths
//   mul2dsp_map.v         mul2dsp _dsp_block_ -> multiply with the same modes
//   add_sub_map.v         $add/$sub -> carry-chain adder techmap
//   <model>_map.v         one per -exotic comb block

// files are required because the yosys consumers (memory_libmap -lib,
// techmap -map, read_verilog -lib) only accept file paths; the one rule
// that could be inline (the abc -luts cost spec) is not emitted, it lives
// as the lutCost knob in synthesis.tcl.

// arch facts (mode addr/data widths, multiply modes, hardblock presence)
// come from the xml; flow policy (libmap costs) arrives as pass options
// from the synthesis.tcl knob block.
USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

std::string archNameFromPath(const std::string &xmlPath) {
  size_t slash = xmlPath.find_last_of("/\\");
  std::string base = slash == std::string::npos ? xmlPath : xmlPath.substr(slash + 1);
  const std::string suffix = ".xml";
  if (base.size() > suffix.size() &&
      base.compare(base.size() - suffix.size(), suffix.size(), suffix) == 0)
    base.erase(base.size() - suffix.size());
  return base;
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

struct VtrArchRulesPass : public Pass {
  VtrArchRulesPass()
      : Pass("vtr_arch_rules", "generate arch mapping rules from a vtr arch xml") {}

  void help() override {
    log("\n");
    log("    vtr_arch_rules -xml <arch.xml> [-outdir <dir>] [-tpldir <dir>]\n");
    log("                    [-sp-cost N] [-dp-cost N] [-blocks a,b,...]\n");
    log("                    [-hard-adder-threshold N]\n");
    log("                    [-stub-all-hardblocks]\n");
    log("                    [-exotic <model> -exotic-template <file>]...\n");
    log("\n");
    log("generate mosaic arch-specific mapping rules from a vtr architecture\n");
    log("xml. emits bram_memory_map.txt, tech_bram.v, vtr_hardblock_lib.v and\n");
    log("mult_map.v into <dir> (default: current directory).\n");
    log("\n");
    log("rule files are template-backed: -tpldir points at the template dir\n");
    log("(default: vtr_flow/misc/mosaic/template/templates). -blocks selects a\n");
    log("subset of bram,adder,multiply,hardblock-lib (default: all). -exotic\n");
    log("adds a generic comb-block generator for <model> using <file> as its\n");
    log("techmap template; it may be repeated and pairs by order.\n");
    log("\n");
    log("-stub-all-hardblocks emits generic blackbox stubs for every hardblock\n");
    log("model except multiply/adder/rams, and writes hardblock_keep_types.txt\n");
    log("so synthesis.tcl can setattr keep on rtl-instantiated exotic cells.\n");
    log("\n");
    log("mode geometry is arch-derived; the libmap costs are flow policy\n");
    log("(-sp-cost / -dp-cost, default 128), as is the hard adder width\n");
    log("threshold (-hard-adder-threshold, default 3).\n");
    log("\n");
  }

  void execute(std::vector<std::string> args, RTLIL::Design *) override {
    std::string xmlPath;
    std::string outDir = ".";
    std::string tplDir;
    std::string blocksArg;
    int spCost = 128;
    int dpCost = 128;
    int hardAdderThreshold = 3;
    bool stubAllHardblocks = false;
    std::vector<std::string> exoticModels;
    std::vector<std::string> exoticTemplates;

    size_t argidx;
    for (argidx = 1; argidx < args.size(); argidx++) {
      if (args[argidx] == "-xml" && argidx + 1 < args.size()) {
        xmlPath = args[++argidx];
        continue;
      }
      if (args[argidx] == "-outdir" && argidx + 1 < args.size()) {
        outDir = args[++argidx];
        continue;
      }
      if (args[argidx] == "-tpldir" && argidx + 1 < args.size()) {
        tplDir = args[++argidx];
        continue;
      }
      if (args[argidx] == "-sp-cost" && argidx + 1 < args.size()) {
        spCost = std::atoi(args[++argidx].c_str());
        continue;
      }
      if (args[argidx] == "-dp-cost" && argidx + 1 < args.size()) {
        dpCost = std::atoi(args[++argidx].c_str());
        continue;
      }
      if (args[argidx] == "-hard-adder-threshold" && argidx + 1 < args.size()) {
        hardAdderThreshold = std::atoi(args[++argidx].c_str());
        continue;
      }
      if (args[argidx] == "-blocks" && argidx + 1 < args.size()) {
        blocksArg = args[++argidx];
        continue;
      }
      if (args[argidx] == "-stub-all-hardblocks") {
        stubAllHardblocks = true;
        continue;
      }
      if (args[argidx] == "-exotic" && argidx + 1 < args.size()) {
        exoticModels.push_back(args[++argidx]);
        continue;
      }
      if (args[argidx] == "-exotic-template" && argidx + 1 < args.size()) {
        exoticTemplates.push_back(args[++argidx]);
        continue;
      }
      break;
    }
    if (xmlPath.empty())
      log_cmd_error("vtr_arch_rules: -xml <arch.xml> is required\n");
    if (exoticModels.size() != exoticTemplates.size())
      log_cmd_error("vtr_arch_rules: each -exotic needs a paired "
                    "-exotic-template (%zu vs %zu)\n",
                    exoticModels.size(), exoticTemplates.size());

    // enabled blocks: default all built-ins; exotics always run (they were
    // explicitly requested).
    const std::set<std::string> builtinBlocks = {"bram", "adder", "multiply",
                                                 "hardblock-lib"};
    std::set<std::string> enabled = builtinBlocks;
    if (!blocksArg.empty()) {
      enabled.clear();
      std::istringstream csv(blocksArg);
      std::string name;
      while (std::getline(csv, name, ',')) {
        if (!builtinBlocks.count(name))
          log_cmd_error("vtr_arch_rules: unknown -blocks entry '%s' (valid: "
                        "bram,adder,multiply,hardblock-lib)\n",
                        name.c_str());
        enabled.insert(name);
      }
    }

    mosaic::VtrArchInfo info;
    std::string error;
    if (!mosaic::readArchInfo(xmlPath, info, &error))
      log_cmd_error("vtr_arch_rules: %s\n", error.c_str());

    const std::string archName = archNameFromPath(xmlPath);
    log("vtr_arch_rules: %s: %zu bram modes, multiply modes [%s], adder %s, "
        "lut K=%d K-1=%d\n",
        archName.c_str(), info.bramModes.size(),
        info.multiplyModes.empty() ? "-"
                                   : joinInts(info.multiplyModes, ",").c_str(),
        info.adder.present ? "present" : "absent", info.lutK, info.lutK1);

    mosaic::ArchRulePolicy policy;
    policy.archName = archName;
    policy.tplDir = tplDir;
    policy.spCost = spCost;
    policy.dpCost = dpCost;
    policy.hardAdderThreshold = hardAdderThreshold;

    auto gens = mosaic::makeBuiltinRuleGens();
    for (size_t i = 0; i < exoticModels.size(); ++i) {
      mosaic::ExoticRequest request;
      request.modelName = exoticModels[i];
      request.templatePath = exoticTemplates[i];
      gens.push_back(mosaic::makeExoticRuleGen(request));
    }

    // emit enabled generators in registry order, collecting the comb-kind
    // ones as stub providers for the hardblock lib. when stubbing all
    // exotics, per-model -exotic gens still emit techmaps but their stubs
    // are skipped so the lib does not get duplicate modules.
    std::vector<const mosaic::ArchRuleGen *> stubProviders;
    for (const auto &gen : gens) {
      const bool isExotic = !builtinBlocks.count(gen->blockName());
      if (!isExotic && !enabled.count(gen->blockName()))
        continue;
      gen->emit(info, policy, outDir);
      if (gen->blockName() == "bram")
        continue;
      if (stubAllHardblocks && isExotic)
        continue;
      stubProviders.push_back(gen.get());
    }

    std::unique_ptr<mosaic::ArchRuleGen> stubAllGen;
    if (stubAllHardblocks) {
      stubAllGen = mosaic::makeStubAllExoticsGen();
      stubAllGen->emit(info, policy, outDir);
      stubProviders.push_back(stubAllGen.get());
      size_t exoticCount = 0;
      for (const auto &kv : info.hardblockModels) {
        if (kv.first != "multiply" && kv.first != "adder" &&
            kv.first != "single_port_ram" && kv.first != "dual_port_ram")
          exoticCount++;
      }
      log("vtr_arch_rules: stub-all-hardblocks enabled (%zu exotic models)\n",
          exoticCount);
    }

    if (enabled.count("hardblock-lib"))
      mosaic::makeHardblockLibGen(stubProviders)
          ->emit(info, policy, outDir);
  }
} VtrArchRulesPass;

PRIVATE_NAMESPACE_END
