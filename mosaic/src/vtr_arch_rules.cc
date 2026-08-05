#include "arch_rule_gen.h"
#include "vtr_arch_info.h"

#include "kernel/yosys.h"

#include <cstdlib>
#include <fstream>
#include <map>
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
//   <model>_map.v         one per -exotic combinational block

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

std::string joinPortWidths(const std::map<std::string, int> &widths) {
  std::ostringstream out;
  bool first = true;
  for (const auto &kv : widths) {
    if (!first)
      out << ",";
    first = false;
    out << kv.first << ":" << kv.second;
  }
  return out.str();
}

void listScannedModes(const mosaic::VtrArchInfo &info) {
  log("vtr_arch_rules: classic bramModes (%zu):\n", info.bramModes.size());
  for (const auto &m : info.bramModes) {
    log("  %s %s addrA=%d dataA=%d addrB=%d dataB=%d\n",
        m.isSp ? "sp" : "dp", m.name.c_str(), m.addrBitsA, m.dataBitsA,
        m.addrBitsB, m.dataBitsB);
  }
  log("vtr_arch_rules: hardblockModes (%zu models):\n",
      info.hardblockModes.size());
  for (const auto &kv : info.hardblockModes) {
    log("  model '%s' (%zu bindings):\n", kv.first.c_str(), kv.second.size());
    for (const auto &mode : kv.second) {
      log("    opmode{%s} pb=%s in={%s} out={%s}\n",
          mode.modeQualifier.c_str(), mode.pbTypeName.c_str(),
          joinPortWidths(mode.inputWidths).c_str(),
          joinPortWidths(mode.outputWidths).c_str());
    }
  }
}

void applyClassicAlias(mosaic::ClassicModelNames &classic,
                       const std::string &arg) {
  const size_t eq = arg.find('=');
  if (eq == std::string::npos)
    log_cmd_error("vtr_arch_rules: -alias requires <role>=<model>, got '%s'\n",
                  arg.c_str());
  const std::string role = arg.substr(0, eq);
  const std::string model = arg.substr(eq + 1);
  if (model.empty())
    log_cmd_error("vtr_arch_rules: -alias requires <role>=<model>, got '%s'\n",
                  arg.c_str());
  if (role == "multiply")
    classic.multiply = model;
  else if (role == "adder")
    classic.adder = model;
  else if (role == "single_port_ram")
    classic.singlePortRam = model;
  else if (role == "dual_port_ram")
    classic.dualPortRam = model;
  else
    log_cmd_error("vtr_arch_rules: unknown -alias role '%s' (valid: multiply, "
                  "adder, single_port_ram, dual_port_ram)\n",
                  role.c_str());
}

struct VtrArchRulesPass : public Pass {
  VtrArchRulesPass()
      : Pass("vtr_arch_rules", "generate arch mapping rules from a vtr arch xml") {}

  void help() override {
    log("\n");
    log("    vtr_arch_rules -xml <arch.xml> [-outdir <dir>] [-tpldir <dir>]\n");
    log("                    [-overlay-tpldir <dir>]\n");
    log("                    [-sp-cost N] [-dp-cost N] [-blocks a,b,...]\n");
    log("                    [-hard-adder-threshold N]\n");
    log("                    [-min-hard-mul N] [-min-hard-mem-abits N]\n");
    log("                    [-stub-all-hardblocks]\n");
    log("                    [-alias <role>=<model>]...\n");
    log("                    [-exotic <model> -exotic-template <file>]...\n");
    log("                    [-exotic-role <model> <role>]...\n");
    log("                    [-list-modes]\n");
    log("\n");
    log("generate mosaic arch-specific mapping rules from a vtr architecture\n");
    log("xml. emits bram_memory_map.txt, tech_bram.v, vtr_hardblock_lib.v and\n");
    log("mult_map.v into <dir> (default: current directory).\n");
    log("\n");
    log("rule files are template-backed: -tpldir points at the shared template\n");
    log("dir (default: vtr_flow/misc/mosaic/template/rules). optional\n");
    log("-overlay-tpldir is checked first per file so a per-arch rules/ dir\n");
    log("can override selected .tmpl files without replacing the whole set.\n");
    log("-blocks selects a subset of bram,adder,multiply,hardblock-lib\n");
    log("(default: all). -exotic adds a generic combinational-block generator\n");
    log("for <model> using <file> as its techmap template; it may be repeated\n");
    log("and pairs by order.\n");
    log("\n");
    log("-stub-all-hardblocks emits generic blackbox stubs for every hardblock\n");
    log("model except multiply/adder/rams, writes hardblock_keep_types.txt,\n");
    log("and emits exotic_identity_maps.v for identity passthrough.\n");
    log("\n");
    log("-alias maps classic roles (multiply, adder, single_port_ram,\n");
    log("dual_port_ram) to arch model names when they differ from the defaults.\n");
    log("\n");
    log("-exotic-role binds a stock role template from roles/<role>_map.v.tmpl\n");
    log("(overlay then tpldir) to an exotic model for inferred yosys ops.\n");
    log("integer_mul is skipped when classic multiply is present.\n");
    log("\n");
    log("-list-modes prints opmode-qualified hardblockModes and classic bramModes\n");
    log("then exits without emitting rule files (debug aid for titan arches).\n");
    log("\n");
    log("mode geometry comes from the arch xml. the following options are\n");
    log("flow policy, not arch facts: -sp-cost / -dp-cost (libmap costs),\n");
    log("-hard-adder-threshold, -min-hard-mul, and -min-hard-mem-abits.\n");
    log("\n");
  }

  void execute(std::vector<std::string> args, RTLIL::Design *) override {
    std::string xmlPath;
    std::string outDir = ".";
    std::string tplDir;
    std::string overlayTplDir;
    std::string blocksArg;
    int spCost = 128;
    int dpCost = 128;
    int hardAdderThreshold = 3;
    int minHardMulWidth = 0;
    int minHardMemAbits = 0;
    bool stubAllHardblocks = false;
    bool listModes = false;
    mosaic::ClassicModelNames classic;
    std::vector<std::string> exoticModels;
    std::vector<std::string> exoticTemplates;
    std::vector<std::string> exoticRoleModels;
    std::vector<std::string> exoticRoleNames;

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
      if (args[argidx] == "-overlay-tpldir" && argidx + 1 < args.size()) {
        overlayTplDir = args[++argidx];
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
      if (args[argidx] == "-min-hard-mul" && argidx + 1 < args.size()) {
        minHardMulWidth = std::atoi(args[++argidx].c_str());
        continue;
      }
      if (args[argidx] == "-min-hard-mem-abits" && argidx + 1 < args.size()) {
        minHardMemAbits = std::atoi(args[++argidx].c_str());
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
      if (args[argidx] == "-list-modes") {
        listModes = true;
        continue;
      }
      if (args[argidx] == "-alias" && argidx + 1 < args.size()) {
        applyClassicAlias(classic, args[++argidx]);
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
      if (args[argidx] == "-exotic-role" && argidx + 2 < args.size()) {
        exoticRoleModels.push_back(args[++argidx]);
        exoticRoleNames.push_back(args[++argidx]);
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
    if (exoticRoleModels.size() != exoticRoleNames.size())
      log_cmd_error("vtr_arch_rules: each -exotic-role needs <model> <role> "
                    "(%zu vs %zu)\n",
                    exoticRoleModels.size(), exoticRoleNames.size());

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
    if (!mosaic::readArchInfo(xmlPath, info, classic, &error))
      log_cmd_error("vtr_arch_rules: %s\n", error.c_str());

    if (listModes) {
      listScannedModes(info);
      return;
    }

    const std::string archName = archNameFromPath(xmlPath);
    const bool multiplyPresent =
        info.multiply.present && !info.multiplyModes.empty();
    int maxRamAbits = 0;
    for (const auto &m : info.bramModes)
      maxRamAbits = std::max(maxRamAbits, m.addrBitsA);
    size_t hardblockModeCount = 0;
    for (const auto &kv : info.hardblockModes)
      hardblockModeCount += kv.second.size();
    log("vtr_arch_rules: %s: %zu bram modes, %zu hardblockModes (%zu models), "
        "multiply modes [%s], adder %s, lut K=%d K-1=%d\n",
        archName.c_str(), info.bramModes.size(), hardblockModeCount,
        info.hardblockModes.size(),
        info.multiplyModes.empty() ? "-"
                                   : joinInts(info.multiplyModes, ",").c_str(),
        info.adder.present ? "present" : "absent", info.lutK, info.lutK1);
    log("vtr_arch_rules: arch facts: vtrRamAbits=%d dspMaxWidth=%d "
        "multiplyPresent=%d adderPresent=%d\n",
        maxRamAbits, multiplyPresent ? info.multiplyModes.back() : 0,
        multiplyPresent ? 1 : 0, info.adder.present ? 1 : 0);

    mosaic::ArchRulePolicy policy;
    policy.archName = archName;
    policy.tplDir = tplDir;
    policy.overlayTplDir = overlayTplDir;
    policy.spCost = spCost;
    policy.dpCost = dpCost;
    policy.hardAdderThreshold = hardAdderThreshold;
    policy.minHardMulWidth = minHardMulWidth;
    policy.minHardMemAbits = minHardMemAbits;
    policy.classic = classic;

    mosaic::emitArchFacts(info, policy, outDir);

    auto gens = mosaic::makeBuiltinRuleGens();
    for (size_t i = 0; i < exoticModels.size(); ++i) {
      mosaic::ExoticRequest request;
      request.modelName = exoticModels[i];
      request.templatePath = exoticTemplates[i];
      gens.push_back(mosaic::makeExoticRuleGen(request));
    }

    std::vector<std::string> roleMapEmittedPaths(exoticRoleModels.size());
    for (size_t i = 0; i < exoticRoleModels.size(); ++i) {
      mosaic::ExoticRoleRequest request;
      request.modelName = exoticRoleModels[i];
      request.roleName = exoticRoleNames[i];
      gens.push_back(
          mosaic::makeRoleRuleGen(request, &roleMapEmittedPaths[i]));
    }

    // emit enabled generators in registry order, collecting the combinational-kind
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

    {
      std::ostringstream roleList;
      bool firstRole = true;
      for (const std::string &path : roleMapEmittedPaths) {
        if (path.empty())
          continue;
        if (!firstRole)
          roleList << "\n";
        firstRole = false;
        roleList << path;
      }
      if (!firstRole) {
        std::ofstream out(outDir + "/role_map_files.txt", std::ios::binary);
        if (!out.is_open())
          log_cmd_error("vtr_arch_rules: cannot write %s/role_map_files.txt\n",
                        outDir.c_str());
        out << roleList.str();
      }
    }

    std::unique_ptr<mosaic::ArchRuleGen> stubAllGen;
    if (stubAllHardblocks) {
      stubAllGen = mosaic::makeStubAllExoticsGen();
      stubAllGen->emit(info, policy, outDir);
      stubProviders.push_back(stubAllGen.get());
      size_t exoticCount = 0;
      for (const auto &kv : info.hardblockModels) {
        if (kv.first == policy.classic.multiply ||
            kv.first == policy.classic.adder ||
            kv.first == policy.classic.singlePortRam ||
            kv.first == policy.classic.dualPortRam)
          continue;
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
