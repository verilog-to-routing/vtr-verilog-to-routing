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

// template load, write, and @token@ / @@snippet@@ substitution helpers.

// HELPER: replace every occurrence of from with to in text.
std::string replaceAll(std::string text, const std::string &from, const std::string &to) {
    size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::string::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
    return text;
}

// HELPER: read a template or output file as raw text, failing the pass on error.
std::string readTextFile(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open())
        log_cmd_error("vtr_arch_rules: cannot read template %s\n", path.c_str());
    std::ostringstream buf;
    buf << in.rdbuf();
    return buf.str();
}

// HELPER: test whether a template path exists without reading it.
bool templateFileExists(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    return in.is_open();
}

// HELPER: resolve a template file from overlayTplDir first, then tplDir.
std::string loadTemplate(const ArchRulePolicy &policy, const std::string &fileName) {
    if (policy.tplDir.empty())
        log_cmd_error("vtr_arch_rules: -tpldir <dir> is required (template %s)\n", fileName.c_str());
    // per-arch overlay wins for individual template files so rules/ can merge
    // with tplDir instead of replacing the whole template set.
    if (!policy.overlayTplDir.empty()) {
        const std::string overlayPath = policy.overlayTplDir + "/" + fileName;
        if (templateFileExists(overlayPath))
            return readTextFile(overlayPath);
    }
    return readTextFile(policy.tplDir + "/" + fileName);
}

// HELPER: write generated rule text to disk, failing the pass on error.
void writeFile(const std::string &path, const std::string &content) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open())
        log_cmd_error("vtr_arch_rules: cannot write %s\n", path.c_str());
    out << content;
    if (!out.good())
        log_cmd_error("vtr_arch_rules: failed writing %s\n", path.c_str());
}

// HELPER: substitute @@snippet@@ tokens first, then @scalar@ tokens. snippet
// values inherit the template newline convention before scalar substitution.
std::string substituteTemplate(std::string text, const std::map<std::string, std::string> &scalars,
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

    // leftover @NAME@ means the emitter did not supply a template scalar token.
    const std::regex leftoverToken(R"(@[A-Za-z0-9_]+@)");
    std::smatch match;
    if (std::regex_search(text, match, leftoverToken))
        log_cmd_error("vtr_arch_rules: unsubstituted template token '%s' "
                      "(rebuild mosaic / check rule emitter scalars)\n",
                      match.str(0).c_str());
    return text;
}

// HELPER: join integers with a separator for template scalars.
std::string joinInts(const std::vector<int> &vals, const char *sep) {
    std::ostringstream out;
    for (size_t i = 0; i < vals.size(); ++i) {
        if (i)
            out << sep;
        out << vals[i];
    }
    return out.str();
}

// HELPER: ensure a stub section ends with exactly one blank line for concatenation.
std::string finishStub(std::string stub) {
    if (stub.empty() || stub.back() != '\n')
        stub += '\n';
    return stub + '\n';
}

} // namespace arch_rule_detail
} // namespace mosaic
