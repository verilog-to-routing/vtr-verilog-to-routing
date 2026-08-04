#ifndef MOSAIC_ARCH_RULE_GEN_H
#define MOSAIC_ARCH_RULE_GEN_H

#include "vtr_arch_info.h"

#include <memory>
#include <string>
#include <vector>

// registry-based rule generators behind the vtr_arch_rules pass.
//
// each ArchRuleGen turns one slice of the scanned arch (vtr_arch_info) plus
// the flow policy knobs into rule files under -outdir, substituting
// arch-derived values into template files from -tpldir. comb-kind
// generators additionally contribute a stub section that the hardblock-lib
// generator aggregates into the single vtr_hardblock_lib.v.
//
// block kinds:
//   memory  (bram):        emits libmap rules + techmap, no lib stub
//   comb    (adder/multiply/exotic): techmap and/or lib stub section
//   carry-chain blocks would subclass ArchRuleGen the same way; only the
//   stub/techmap text differs.
namespace mosaic {

// flow policy that is not arch-derivable; arrives as pass options.
struct ArchRulePolicy {
  std::string archName;
  std::string tplDir;
  int spCost = 128;
  int dpCost = 128;
  // $add/$sub at or below this width stay soft so abc can optimize
  // across them (hard adders are black boxes).
  int hardAdderThreshold = 3;
};

// one -exotic <model> -exotic-template <file> request.
struct ExoticRequest {
  std::string modelName;
  std::string templatePath;
};

class ArchRuleGen {
public:
  explicit ArchRuleGen(std::string blockName) : blockName_(blockName) {}
  virtual ~ArchRuleGen() = default;

  const std::string &blockName() const { return blockName_; }

  // emit this generator's rule files into outDir.
  virtual void emit(const VtrArchInfo &info, const ArchRulePolicy &policy,
                    const std::string &outDir) const = 0;

  // comb-kind generators contribute a vtr_hardblock_lib.v stub section
  // (text ends with one blank line). default: empty (memory-kind blocks
  // and models absent from the arch contribute nothing).
  virtual std::string hardblockStub(const VtrArchInfo &info,
                                    const ArchRulePolicy &policy) const {
    (void)info;
    (void)policy;
    return {};
  }

private:
  std::string blockName_;
};

// built-in generators in emission/stub order: bram, adder, multiply.
std::vector<std::unique_ptr<ArchRuleGen>> makeBuiltinRuleGens();

// generic comb-block extension point (-exotic); block name is the model
// name. when the model is absent from the arch the generator emits an
// always-_TECHMAP_FAIL_ map and contributes no stub.
std::unique_ptr<ArchRuleGen> makeExoticRuleGen(const ExoticRequest &request);

// stub-only generator for every hardblock model that is not a builtin
// (multiply/adder/rams). emit() writes hardblock_keep_types.txt; the
// hardblock stub section is generic blackboxes from scanned port geometry.
// block name: "stub-all-exotics".
std::unique_ptr<ArchRuleGen> makeStubAllExoticsGen();

// aggregates the stub sections of providers (in the given order) into
// vtr_hardblock_lib.v. block name: "hardblock-lib".
std::unique_ptr<ArchRuleGen>
makeHardblockLibGen(std::vector<const ArchRuleGen *> providers);

// emit arch_facts.tcl: arch-derived scalars consumed by synthesis.tcl
// (not flow policy knobs from arch_config.tcl).
void emitArchFacts(const VtrArchInfo &info, const ArchRulePolicy &policy,
                   const std::string &outDir);

} // namespace mosaic

#endif // MOSAIC_ARCH_RULE_GEN_H
