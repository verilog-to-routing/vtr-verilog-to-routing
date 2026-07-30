#include "vtr_arch_info.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <deque>
#include <fstream>
#include <sstream>

namespace wildebeestVtr {

namespace {

// ---------------------------------------------------------------------------
// minimal xml dom: elements + attributes only (no text, namespaces, or dtd).
// vtr arch xml uses none of the skipped features, so this is enough and keeps
// the plugin free of any xml dependency.
// ---------------------------------------------------------------------------

struct XmlNode {
  std::string name;
  std::vector<std::pair<std::string, std::string>> attrs;
  std::vector<XmlNode *> children;
  bool selfClosing_ = false;

  const std::string *attr(const std::string &key) const {
    for (const auto &kv : attrs)
      if (kv.first == key)
        return &kv.second;
    return nullptr;
  }

  std::vector<const XmlNode *> find(const std::string &childName) const {
    std::vector<const XmlNode *> out;
    for (const XmlNode *c : children)
      if (c->name == childName)
        out.push_back(c);
    return out;
  }
};

std::string decodeEntities(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '&') {
      size_t semi = s.find(';', i);
      if (semi != std::string::npos && semi - i <= 6) {
        std::string ent = s.substr(i + 1, semi - i - 1);
        if (ent == "amp") { out += '&'; i = semi; continue; }
        if (ent == "lt") { out += '<'; i = semi; continue; }
        if (ent == "gt") { out += '>'; i = semi; continue; }
        if (ent == "quot") { out += '"'; i = semi; continue; }
        if (ent == "apos") { out += '\''; i = semi; continue; }
      }
    }
    out += s[i];
  }
  return out;
}

class XmlParser {
public:
  XmlParser(const std::string &text) : text_(text) {}

  // parse the whole document; returns nullptr on malformed input.
  XmlNode *parse() {
    std::vector<XmlNode *> stack;
    XmlNode *root = nullptr;
    while (skipToTag()) {
      if (match("</")) {
        // closing tag: pop (validate name loosely)
        size_t end = text_.find('>', pos_);
        if (end == std::string::npos)
          return nullptr;
        if (!stack.empty())
          stack.pop_back();
        pos_ = end + 1;
        continue;
      }
      XmlNode *node = parseOpenTag();
      if (!node)
        return nullptr;
      if (!stack.empty())
        stack.back()->children.push_back(node);
      else if (!root)
        root = node;
      if (!node->selfClosing_)
        stack.push_back(node);
    }
    return root;
  }

private:
  const std::string &text_;
  size_t pos_ = 0;
  std::deque<XmlNode> pool_;

  bool match(const char *pat) const {
    return text_.compare(pos_, std::strlen(pat), pat) == 0;
  }

  // advance to the next '<', skipping prologs, comments, doctypes, cdata.
  bool skipToTag() {
    while (true) {
      size_t lt = text_.find('<', pos_);
      if (lt == std::string::npos)
        return false;
      pos_ = lt;
      if (match("<!--")) {
        size_t end = text_.find("-->", pos_ + 4);
        if (end == std::string::npos)
          return false;
        pos_ = end + 3;
        continue;
      }
      if (match("<?")) {
        size_t end = text_.find("?>", pos_ + 2);
        if (end == std::string::npos)
          return false;
        pos_ = end + 2;
        continue;
      }
      if (match("<![CDATA[")) {
        size_t end = text_.find("]]>", pos_ + 9);
        if (end == std::string::npos)
          return false;
        pos_ = end + 3;
        continue;
      }
      if (match("<!")) {
        size_t end = text_.find('>', pos_ + 2);
        if (end == std::string::npos)
          return false;
        pos_ = end + 1;
        continue;
      }
      return true;
    }
  }

  XmlNode *parseOpenTag() {
    ++pos_; // past '<'
    pool_.emplace_back();
    XmlNode *node = &pool_.back();
    // element name
    while (pos_ < text_.size() && !isspace((unsigned char)text_[pos_]) &&
           text_[pos_] != '>' && text_[pos_] != '/') {
      node->name += text_[pos_++];
    }
    if (node->name.empty())
      return nullptr;
    // attributes
    while (pos_ < text_.size()) {
      while (pos_ < text_.size() && isspace((unsigned char)text_[pos_]))
        ++pos_;
      if (pos_ >= text_.size())
        return nullptr;
      if (text_[pos_] == '>') {
        ++pos_;
        return node;
      }
      if (match("/>")) {
        pos_ += 2;
        node->selfClosing_ = true;
        return node;
      }
      std::string key;
      while (pos_ < text_.size() &&
             (isalnum((unsigned char)text_[pos_]) || text_[pos_] == '_' ||
              text_[pos_] == '-' || text_[pos_] == '.' || text_[pos_] == ':'))
        key += text_[pos_++];
      while (pos_ < text_.size() && isspace((unsigned char)text_[pos_]))
        ++pos_;
      if (pos_ >= text_.size() || text_[pos_] != '=')
        return nullptr;
      ++pos_;
      while (pos_ < text_.size() && isspace((unsigned char)text_[pos_]))
        ++pos_;
      if (pos_ >= text_.size() ||
          (text_[pos_] != '"' && text_[pos_] != '\''))
        return nullptr;
      char quote = text_[pos_++];
      size_t end = text_.find(quote, pos_);
      if (end == std::string::npos)
        return nullptr;
      node->attrs.emplace_back(key, decodeEntities(text_.substr(pos_, end - pos_)));
      pos_ = end + 1;
    }
    return nullptr;
  }
};

// ---------------------------------------------------------------------------
// extraction helpers (mirror the legacy offline generator semantics)
// ---------------------------------------------------------------------------

void collectAll(const XmlNode *node, const std::string &name,
                std::vector<const XmlNode *> &out) {
  if (node->name == name)
    out.push_back(node);
  for (const XmlNode *c : node->children)
    collectAll(c, name, out);
}

int attrInt(const XmlNode *node, const std::string &key, int fallback = 0) {
  const std::string *v = node->attr(key);
  if (!v)
    return fallback;
  try {
    return std::stoi(*v);
  } catch (...) {
    return fallback;
  }
}

std::string attrStr(const XmlNode *node, const std::string &key) {
  const std::string *v = node->attr(key);
  return v ? *v : std::string();
}

bool isTruthy(const std::string &v) {
  return !v.empty() && v != "0" && v != "false";
}

// map of direct <input>/<output> children by name -> num_pins
std::vector<std::pair<std::string, int>> directPins(const XmlNode *node,
                                                    const char *tag) {
  std::vector<std::pair<std::string, int>> out;
  for (const XmlNode *pin : node->find(tag))
    out.emplace_back(attrStr(pin, "name"), attrInt(pin, "num_pins", 1));
  return out;
}

int pinWidth(const std::vector<std::pair<std::string, int>> &pins,
             const std::string &name) {
  for (const auto &kv : pins)
    if (kv.first == name)
      return kv.second;
  return 0;
}

void scanModels(const XmlNode *root, VtrArchInfo &info) {
  for (const XmlNode *modelsNode : root->find("models")) {
    for (const XmlNode *model : modelsNode->find("model")) {
      for (const XmlNode *inputPorts : model->find("input_ports")) {
        for (const XmlNode *port : inputPorts->find("port")) {
          const std::string *isClock = port->attr("is_clock");
          if (isClock && isTruthy(*isClock)) {
            std::string name = attrStr(model, "name");
            if (std::find(info.clockedModels.begin(),
                          info.clockedModels.end(),
                          name) == info.clockedModels.end())
              info.clockedModels.push_back(name);
          }
        }
      }
    }
  }
}

void scanBramModes(const std::vector<const XmlNode *> &pbTypes,
                   VtrArchInfo &info) {
  for (const XmlNode *pb : pbTypes) {
    const std::string blif = attrStr(pb, "blif_model");
    bool isSp = blif == ".subckt single_port_ram";
    bool isDp = blif == ".subckt dual_port_ram";
    if (!isSp && !isDp)
      continue;
    auto pins = directPins(pb, "input");
    BramModeInfo mode;
    mode.name = attrStr(pb, "name");
    mode.isSp = isSp;
    mode.addrBitsA = pinWidth(pins, isSp ? "addr" : "addr1");
    mode.dataBitsA = pinWidth(pins, isSp ? "data" : "data1");
    mode.addrBitsB = pinWidth(pins, "addr2");
    mode.dataBitsB = pinWidth(pins, "data2");
    if (mode.addrBitsB == 0)
      mode.addrBitsB = mode.addrBitsA;
    if (mode.dataBitsB == 0)
      mode.dataBitsB = mode.dataBitsA;
    if (mode.addrBitsA > 0 && mode.dataBitsA > 0)
      info.bramModes.push_back(mode);
  }
}

// size of a .names lut pb_type = num_pins of its first direct <input>
int lutSize(const XmlNode *pb) {
  for (const XmlNode *in : pb->find("input"))
    return attrInt(in, "num_pins", 0);
  return 0;
}

void scanLutCost(const std::vector<const XmlNode *> &pbTypes,
                 VtrArchInfo &info) {
  int singleK = 0;
  int subK = 0;
  for (const XmlNode *pb : pbTypes) {
    auto modes = pb->find("mode");
    if (modes.size() < 2)
      continue;
    for (const XmlNode *mode : modes) {
      // .names luts among direct pb_type children, or one level deeper
      std::vector<std::pair<int, int>> luts; // (size, num_pb)
      for (const XmlNode *child : mode->find("pb_type")) {
        if (attrStr(child, "blif_model") == ".names")
          luts.emplace_back(lutSize(child), attrInt(child, "num_pb", 1));
        else
          for (const XmlNode *grand : child->find("pb_type"))
            if (attrStr(grand, "blif_model") == ".names")
              luts.emplace_back(lutSize(grand),
                                attrInt(grand, "num_pb", 1) *
                                    attrInt(child, "num_pb", 1));
      }
      if (luts.empty())
        continue;
      int total = 0;
      int maxSize = 0;
      for (const auto &l : luts) {
        total += l.second;
        maxSize = std::max(maxSize, l.first);
      }
      if (total == 1)
        singleK = std::max(singleK, maxSize);
      else if (total >= 2)
        subK = std::max(subK, maxSize);
    }
  }
  info.lutK = singleK;
  info.lutK1 = subK;
}

// generic hardblock scan: every pb_type with a blif_model ".subckt <name>"
// binding contributes its direct port widths to hardblockModels[<name>].
// this replaces the old adder/multiply-only scan; those two are derived
// from the generic map below.
void scanHardblockModels(const std::vector<const XmlNode *> &pbTypes,
                         VtrArchInfo &info) {
  const std::string prefix = ".subckt ";
  for (const XmlNode *pb : pbTypes) {
    const std::string blif = attrStr(pb, "blif_model");
    if (blif.compare(0, prefix.size(), prefix) != 0)
      continue;
    ModelGeometry &geo = info.hardblockModels[blif.substr(prefix.size())];
    auto inputs = directPins(pb, "input");
    for (const auto &kv : inputs) {
      int &w = geo.inputWidths[kv.first];
      w = std::max(w, kv.second);
    }
    for (const auto &kv : directPins(pb, "output")) {
      int &w = geo.outputWidths[kv.first];
      w = std::max(w, kv.second);
    }
    int a = pinWidth(inputs, "a");
    if (a > 0 && std::find(geo.modes.begin(), geo.modes.end(), a) ==
                     geo.modes.end())
      geo.modes.push_back(a);
  }
  for (auto &kv : info.hardblockModels)
    std::sort(kv.second.modes.begin(), kv.second.modes.end());
}

// fill the legacy adder/multiply accessors from the generic map so existing
// consumers (vtr_arch_clocks wrapper, rule generators) keep working.
void deriveHardblockAliases(VtrArchInfo &info) {
  auto mult = info.hardblockModels.find("multiply");
  if (mult != info.hardblockModels.end() && !mult->second.modes.empty()) {
    info.multiply.present = true;
    info.multiply.aWidth = mult->second.modes.back();
    info.multiplyModes = mult->second.modes;
  }
  auto add = info.hardblockModels.find("adder");
  if (add != info.hardblockModels.end()) {
    auto a = add->second.inputWidths.find("a");
    if (a != add->second.inputWidths.end() && a->second > 0) {
      info.adder.present = true;
      info.adder.aWidth = a->second;
    }
  }
}

} // namespace

bool readArchInfo(const std::string &xmlPath, VtrArchInfo &info,
                  std::string *errorOut) {
  std::ifstream in(xmlPath, std::ios::binary);
  if (!in.is_open()) {
    if (errorOut)
      *errorOut = "cannot open " + xmlPath;
    return false;
  }
  std::ostringstream buf;
  buf << in.rdbuf();
  const std::string text = buf.str();

  XmlParser parser(text);
  XmlNode *root = parser.parse();
  if (!root) {
    if (errorOut)
      *errorOut = "malformed xml in " + xmlPath;
    return false;
  }

  scanModels(root, info);

  std::vector<const XmlNode *> pbTypes;
  collectAll(root, "pb_type", pbTypes);
  scanBramModes(pbTypes, info);
  scanLutCost(pbTypes, info);
  scanHardblockModels(pbTypes, info);
  deriveHardblockAliases(info);
  return true;
}

} // namespace wildebeestVtr
