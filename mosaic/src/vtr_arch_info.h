#ifndef MOSAIC_VTR_ARCH_INFO_H
#define MOSAIC_VTR_ARCH_INFO_H

#include <map>
#include <string>
#include <vector>

// generalized reader for the slices of a vtr architecture xml that mosaic
// needs at synthesis time.

// the xml is parsed with pugixml from vtr's vendored copy
// (libs/EXTERNAL/libpugixml), so there is no libarchfpga/libvtrutil link and
// the plugin keeps building against yosys alone. it walks the pb_type
// hierarchy for bram modes, lut fracturability, and hardblock port widths,
// the data that must come from the live arch rather than hardcoded statics.
namespace mosaic {

// one concrete bram mode from a pb_type with blif_model
// ".subckt single_port_ram" / ".subckt dual_port_ram".
struct BramModeInfo {
  std::string name;
  int addrBitsA = 0;
  int dataBitsA = 0;
  bool isSp = false;
  int addrBitsB = 0; // dp only; falls back to addrBitsA when absent
  int dataBitsB = 0; // dp only; falls back to dataBitsA when absent
};

// generic geometry of one hardblock model, gathered from every pb_type that
// binds blif_model ".subckt <name>" (the <models> section carries no sizes).
// port widths are the max num_pins seen per port name across bindings;
// <clock> pins are folded into inputWidths. modes holds the sorted distinct
// widths of the 'a' input port (multiply-style mode geometry; empty for
// models without an 'a' input).
struct ModelGeometry {
  std::map<std::string, int> inputWidths;
  std::map<std::string, int> outputWidths;
  std::vector<int> modes;
};

// port width of a hardblock model, read from the pb_type that binds the
// blif model. adder in k6-style archs is a 1-bit carry element.
struct HardblockInfo {
  bool present = false;
  int aWidth = 0;
};

struct VtrArchInfo {
  // <models>: names of models with an is_clock input port
  std::vector<std::string> clockedModels;

  // pb_type bram modes
  std::vector<BramModeInfo> bramModes;

  // lut fracturability from multi-mode pb_types containing .names luts:
  // lutK = size of the single-lut mode, lutK1 = max sub-lut size in the
  // fractured mode (0 when not detected / not fracturable)
  int lutK = 0;
  int lutK1 = 0;

  // every pb_type blif_model ".subckt <name>" binding, keyed by model name
  // (includes the ram models and any exotic comb blocks the arch carries).
  std::map<std::string, ModelGeometry> hardblockModels;

  // adder / multiply convenience accessors, derived from hardblockModels
  // by readArchInfo. multiplyModes holds the per-mode operand widths
  // (e.g. {9, 18, 36}); multiply.aWidth mirrors the widest mode.
  HardblockInfo adder;
  HardblockInfo multiply;
  std::vector<int> multiplyModes;
};

// parse the arch xml at xmlPath into info. returns false and sets errorOut
// (when non-null) on read/parse failure; partial data may be present.
bool readArchInfo(const std::string &xmlPath, VtrArchInfo &info,
                  std::string *errorOut = nullptr);

} // namespace mosaic

#endif // MOSAIC_VTR_ARCH_INFO_H
