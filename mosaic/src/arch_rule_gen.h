#ifndef MOSAIC_ARCH_RULE_GEN_H
#define MOSAIC_ARCH_RULE_GEN_H

#include "vtr_arch_info.h"

#include <memory>
#include <string>
#include <vector>

// public API for the registry-based rule generators behind vtr_arch_rules.
// implementations live under mosaic/src/arch_rule_gen/ (one translation unit
// per generator family) so this header stays the stable include for the pass.
//
// each ArchRuleGen turns one slice of the scanned arch (VtrArchInfo) plus flow
// policy knobs into rule files under -outdir, substituting arch-derived values
// into template files from -tpldir, with optional -overlay-tpldir checked first
// per file. combinational-kind generators also contribute a stub section that
// the hardblock-lib generator aggregates into the single vtr_hardblock_lib.v.
//
// memory (bram) emits libmap rules and techmap with no lib stub, while
// combinational (adder, multiply, exotic) emits techmap and/or a lib stub
// section. carry-chain blocks would subclass ArchRuleGen the same way with only
// the stub and techmap text differing.
namespace mosaic {

// flow policy that is not arch-derivable; arrives as pass options.
struct ArchRulePolicy {
    std::string archName;
    std::string tplDir;
    // optional per-arch overlay checked before tplDir for each template file.
    std::string overlayTplDir;
    int spCost = 128;
    int dpCost = 128;
    // $add/$sub at or below this width stay soft so ABC can optimize across them
    // because hard adders are black boxes.
    int hardAdderThreshold = 3;
    // $mul stays soft when both operand widths are at or below this threshold.
    // 0 disables this limit.
    int minHardMulWidth = 0;
    // drop bram modes shallower than this address width from libmap. 0 keeps all.
    int minHardMemAbits = 0;
    // when set, missing classic sp/dp ram modes soft-map memories instead of
    // erroring, which titan-style arches without classic ram models need.
    bool softOnlyMemory = false;
    ClassicModelNames classic;
};

// one -exotic <model> -exotic-template <file> request.
struct ExoticRequest {
    std::string modelName;
    std::string templatePath;
};

// one -exotic-role <model> <role> request using the stock template under
// roles/<role>_map.v.tmpl, resolved via overlay then tplDir.
struct ExoticRoleRequest {
    std::string modelName;
    std::string roleName;
};

class ArchRuleGen {
  public:
    explicit ArchRuleGen(std::string blockName) : blockName_(blockName) {}
    virtual ~ArchRuleGen() = default;

    const std::string &blockName() const { return blockName_; }

    // USE: emit this generator's rule files into outDir.
    virtual void emit(const VtrArchInfo &info, const ArchRulePolicy &policy, const std::string &outDir) const = 0;

    // USE: return a vtr_hardblock_lib.v stub section for combinational-kind
    // generators. text ends with one blank line. memory-kind blocks and models
    // absent from the arch return empty.
    virtual std::string hardblockStub(const VtrArchInfo &info, const ArchRulePolicy &policy) const {
        (void)info;
        (void)policy;
        return {};
    }

  private:
    std::string blockName_;
};

// USE: built-in generators in emission and stub order (bram, adder, multiply).
std::vector<std::unique_ptr<ArchRuleGen>> makeBuiltinRuleGens();

// USE: generic combinational-block extension point (-exotic). block name is the
// model name. when the model is absent from the arch the generator emits an
// always-_TECHMAP_FAIL_ map and contributes no stub.
std::unique_ptr<ArchRuleGen> makeExoticRuleGen(const ExoticRequest &request);

// USE: role-based exotic techmap (-exotic-role) using roles/<role>_map.v.tmpl
// via loadTemplate (overlayTplDir then tplDir). sets *emittedMapPath to the
// written map path, or empty when integer_mul is skipped because classic
// multiply is present.
std::unique_ptr<ArchRuleGen> makeRoleRuleGen(const ExoticRoleRequest &request, std::string *emittedMapPath);

// USE: stub-only generator for every hardblock model that is not a builtin
// (multiply, adder, rams). emit() writes hardblock_keep_types.txt and the
// hardblock stub section is generic blackboxes from scanned port geometry.
// block name is stub-all-exotics.
std::unique_ptr<ArchRuleGen> makeStubAllExoticsGen();

// USE: aggregate stub sections from providers in the given order into
// vtr_hardblock_lib.v. block name is hardblock-lib.
std::unique_ptr<ArchRuleGen> makeHardblockLibGen(std::vector<const ArchRuleGen *> providers);

// USE: emit arch_facts.tcl with arch-derived scalars for synthesis.tcl, not
// flow policy knobs from arch_config.tcl.
void emitArchFacts(const VtrArchInfo &info, const ArchRulePolicy &policy, const std::string &outDir);

} // namespace mosaic

#endif // MOSAIC_ARCH_RULE_GEN_H
