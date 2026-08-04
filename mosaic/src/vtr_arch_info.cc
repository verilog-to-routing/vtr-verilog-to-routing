#include "vtr_arch_info.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "pugixml.hpp"

namespace mosaic {

namespace {

// ---------------------------------------------------------------------------
// extraction helpers
// ---------------------------------------------------------------------------

void collectAll(const pugi::xml_node &node, const char *name,
                std::vector<pugi::xml_node> &out) {
  if (std::strcmp(node.name(), name) == 0)
    out.push_back(node);
  for (const pugi::xml_node child : node.children())
    collectAll(child, name, out);
}

int attrInt(const pugi::xml_node &node, const char *key, int fallback = 0) {
  return node.attribute(key).as_int(fallback);
}

std::string attrStr(const pugi::xml_node &node, const char *key) {
  return node.attribute(key).value();
}

bool isTruthy(const std::string &v) {
  return !v.empty() && v != "0" && v != "false";
}

// map of direct <input>/<output> children by name -> num_pins
std::vector<std::pair<std::string, int>> directPins(const pugi::xml_node &node,
                                                    const char *tag) {
  std::vector<std::pair<std::string, int>> out;
  for (const pugi::xml_node pin : node.children(tag))
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

void scanModels(const pugi::xml_node &root, VtrArchInfo &info) {
  for (const pugi::xml_node modelsNode : root.children("models")) {
    for (const pugi::xml_node model : modelsNode.children("model")) {
      for (const pugi::xml_node inputPorts : model.children("input_ports")) {
        for (const pugi::xml_node port : inputPorts.children("port")) {
          const pugi::xml_attribute isClock = port.attribute("is_clock");
          if (!isClock.empty() && isTruthy(isClock.value())) {
            const std::string name = attrStr(model, "name");
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

void scanBramModes(const std::vector<pugi::xml_node> &pbTypes,
                   const ClassicModelNames &classic, VtrArchInfo &info) {
  for (const pugi::xml_node pb : pbTypes) {
    const std::string blif = attrStr(pb, "blif_model");
    const std::string spModel = ".subckt " + classic.singlePortRam;
    const std::string dpModel = ".subckt " + classic.dualPortRam;
    const bool isSp = blif == spModel;
    const bool isDp = blif == dpModel;
    if (!isSp && !isDp)
      continue;
    const auto pins = directPins(pb, "input");
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
int lutSize(const pugi::xml_node &pb) {
  for (const pugi::xml_node input : pb.children("input"))
    return attrInt(input, "num_pins", 0);
  return 0;
}

void scanLutCost(const std::vector<pugi::xml_node> &pbTypes,
                 VtrArchInfo &info) {
  int singleK = 0;
  int subK = 0;
  for (const pugi::xml_node pb : pbTypes) {
    std::vector<pugi::xml_node> modes;
    for (const pugi::xml_node mode : pb.children("mode"))
      modes.push_back(mode);
    if (modes.size() < 2)
      continue;
    for (const pugi::xml_node mode : modes) {
      // .names luts among direct pb_type children, or one level deeper
      std::vector<std::pair<int, int>> luts; // (size, num_pb)
      for (const pugi::xml_node child : mode.children("pb_type")) {
        if (attrStr(child, "blif_model") == ".names")
          luts.emplace_back(lutSize(child), attrInt(child, "num_pb", 1));
        else
          for (const pugi::xml_node grand : child.children("pb_type"))
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

// every pb_type with blif_model ".subckt <name>" contributes port widths to
// hardblockModels[<name>]. <clock> children are folded into inputWidths so
// stubs expose clk ports for rtl connections (e.g. mult_fp_clk_16).
void scanHardblockModels(const std::vector<pugi::xml_node> &pbTypes,
                         VtrArchInfo &info) {
  const std::string prefix = ".subckt ";
  for (const pugi::xml_node pb : pbTypes) {
    const std::string blif = attrStr(pb, "blif_model");
    if (blif.compare(0, prefix.size(), prefix) != 0)
      continue;
    ModelGeometry &geo = info.hardblockModels[blif.substr(prefix.size())];
    const auto inputs = directPins(pb, "input");
    for (const auto &kv : inputs) {
      int &w = geo.inputWidths[kv.first];
      w = std::max(w, kv.second);
    }
    for (const auto &kv : directPins(pb, "clock")) {
      int &w = geo.inputWidths[kv.first];
      w = std::max(w, kv.second);
    }
    for (const auto &kv : directPins(pb, "output")) {
      int &w = geo.outputWidths[kv.first];
      w = std::max(w, kv.second);
    }
    const int a = pinWidth(inputs, "a");
    if (a > 0 && std::find(geo.modes.begin(), geo.modes.end(), a) ==
                     geo.modes.end())
      geo.modes.push_back(a);
  }
  for (auto &kv : info.hardblockModels)
    std::sort(kv.second.modes.begin(), kv.second.modes.end());
}

// fill adder / multiply convenience fields from hardblockModels.
void deriveHardblockAliases(const ClassicModelNames &classic,
                            VtrArchInfo &info) {
  auto mult = info.hardblockModels.find(classic.multiply);
  if (mult != info.hardblockModels.end() && !mult->second.modes.empty()) {
    info.multiply.present = true;
    info.multiply.aWidth = mult->second.modes.back();
    info.multiplyModes = mult->second.modes;
  }
  auto add = info.hardblockModels.find(classic.adder);
  if (add != info.hardblockModels.end()) {
    auto a = add->second.inputWidths.find("a");
    if (a != add->second.inputWidths.end() && a->second > 0) {
      info.adder.present = true;
      info.adder.aWidth = a->second;
      const bool hasCin =
          add->second.inputWidths.find("cin") != add->second.inputWidths.end();
      const bool hasCout =
          add->second.outputWidths.find("cout") !=
          add->second.outputWidths.end();
      const bool hasSumout =
          add->second.outputWidths.find("sumout") !=
          add->second.outputWidths.end();
      info.adder.carryChain = hasCin && hasCout && hasSumout;
    }
  }
}

} // namespace

bool readArchInfo(const std::string &xmlPath, VtrArchInfo &info,
                  const ClassicModelNames &classic, std::string *errorOut) {
  pugi::xml_document doc;
  const pugi::xml_parse_result result = doc.load_file(xmlPath.c_str());
  if (!result) {
    if (errorOut)
      *errorOut = "cannot parse " + xmlPath + ": " + result.description();
    return false;
  }

  const pugi::xml_node root = doc.document_element();

  scanModels(root, info);

  std::vector<pugi::xml_node> pbTypes;
  collectAll(root, "pb_type", pbTypes);
  scanBramModes(pbTypes, classic, info);
  scanLutCost(pbTypes, info);
  scanHardblockModels(pbTypes, info);
  deriveHardblockAliases(classic, info);
  return true;
}

} // namespace mosaic
