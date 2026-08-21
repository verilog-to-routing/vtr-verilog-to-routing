#ifndef MOSAIC_ARCH_RULE_GEN_INTERNAL_H
#define MOSAIC_ARCH_RULE_GEN_INTERNAL_H

// shared helpers for the arch_rule_gen/*.cc translation units.
// not part of the public mosaic API; include only from this directory.

#include "arch_rule_gen.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mosaic {
namespace arch_rule_detail {

// template io and substitution
std::string replaceAll(std::string text, const std::string &from, const std::string &to);
std::string readTextFile(const std::string &path);
bool templateFileExists(const std::string &path);
std::string loadTemplate(const ArchRulePolicy &policy, const std::string &fileName);
void writeFile(const std::string &path, const std::string &content);
std::string substituteTemplate(std::string text, const std::map<std::string, std::string> &scalars,
                               const std::map<std::string, std::string> &snippets);
std::string joinInts(const std::vector<int> &vals, const char *sep);
std::string finishStub(std::string stub);

// bram helpers
std::vector<BramModeInfo> filterModesByMinAbits(std::vector<BramModeInfo> modes, int minHardMemAbits);
std::vector<BramModeInfo> splitModes(const VtrArchInfo &info, bool sp);
std::vector<BramModeInfo> sortByDataDesc(std::vector<BramModeInfo> modes);
std::vector<int> widthsOf(std::vector<BramModeInfo> modes);
int maxAbitsOf(const std::vector<BramModeInfo> &modes);
std::string addrBitsChain(const char *widthParam, const std::vector<BramModeInfo> &modesDesc);
int maxBramDataWidth(const VtrArchInfo &info);

// multiply helpers
std::string multTernary(const std::vector<int> &modes);
std::string multTernarySingle(const char *widthExpr, const std::vector<int> &modes);
bool isBuiltinHardblock(const std::string &name, const ClassicModelNames &classic);
bool hasExoticHardblocks(const VtrArchInfo &info, const ClassicModelNames &classic);
void warnExoticOnlyMultiply(const VtrArchInfo &info, const ClassicModelNames &classic);

// exotic helpers
std::string tokenName(const std::string &port);
std::string genericStub(const std::string &model, const ModelGeometry &geo);
std::string identityTechmapModule(const std::string &model, const ModelGeometry &geo);
bool hasAbOutPorts(const ModelGeometry &geo);
std::vector<int> exoticMulModes(const ModelGeometry &geo);
std::string alwaysFailExoticMap(const std::string &archName, const std::string &model);

// per-file factories used by registry.cc
std::unique_ptr<ArchRuleGen> makeBramRuleGen();
std::unique_ptr<ArchRuleGen> makeAdderRuleGen();
std::unique_ptr<ArchRuleGen> makeMultiplyRuleGen();
std::unique_ptr<ArchRuleGen> makeExoticRuleGenImpl(const ExoticRequest &request);
std::unique_ptr<ArchRuleGen> makeRoleRuleGenImpl(const ExoticRoleRequest &request, std::string *emittedMapPath);
std::unique_ptr<ArchRuleGen> makeStubAllExoticsGenImpl();
std::unique_ptr<ArchRuleGen> makeHardblockLibGenImpl(std::vector<const ArchRuleGen *> providers);

} // namespace arch_rule_detail
} // namespace mosaic

#endif // MOSAIC_ARCH_RULE_GEN_INTERNAL_H
