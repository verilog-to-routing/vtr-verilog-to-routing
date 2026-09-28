#include "arch_rule_gen/internal.h"

#include <memory>
#include <utility>
#include <vector>

namespace mosaic {

// USE: factory for built-in rule generators in emission order.
std::vector<std::unique_ptr<ArchRuleGen>> makeBuiltinRuleGens() {
    using arch_rule_detail::makeAdderRuleGen;
    using arch_rule_detail::makeBramRuleGen;
    using arch_rule_detail::makeMultiplyRuleGen;
    std::vector<std::unique_ptr<ArchRuleGen>> gens;
    gens.push_back(makeBramRuleGen());
    gens.push_back(makeAdderRuleGen());
    gens.push_back(makeMultiplyRuleGen());
    return gens;
}

// USE: factory for a generic -exotic combinational rule generator.
std::unique_ptr<ArchRuleGen> makeExoticRuleGen(const ExoticRequest &request) {
    return arch_rule_detail::makeExoticRuleGenImpl(request);
}

// USE: factory for a role-based exotic techmap generator.
std::unique_ptr<ArchRuleGen> makeRoleRuleGen(const ExoticRoleRequest &request, std::string *emittedMapPath) {
    return arch_rule_detail::makeRoleRuleGenImpl(request, emittedMapPath);
}

// USE: factory for stub-all-exotics generator.
std::unique_ptr<ArchRuleGen> makeStubAllExoticsGen() { return arch_rule_detail::makeStubAllExoticsGenImpl(); }

// USE: factory for hardblock-lib aggregator generator.
std::unique_ptr<ArchRuleGen> makeHardblockLibGen(std::vector<const ArchRuleGen *> providers) {
    return arch_rule_detail::makeHardblockLibGenImpl(std::move(providers));
}

} // namespace mosaic
