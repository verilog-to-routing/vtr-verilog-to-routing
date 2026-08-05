#ifndef MOSAIC_VTR_ARCH_INFO_H
#define MOSAIC_VTR_ARCH_INFO_H

#include <map>
#include <string>
#include <vector>

// reader for the slices of a vtr architecture xml that mosaic needs at
// synthesis time.
//
// contract (what readArchInfo guarantees):
//   - bramModes: classic single_port_ram / dual_port_ram geometry when the arch
//     declares those model names (exact ".subckt <name>" or
//     ".subckt <name>.opmode{...}"). may be empty for titan-style arches;
//     callers that emit classic bram maps should fail if either sp/dp is
//     missing after parsing.
//   - hardblockModes: every pb_type with blif_model
//     ".subckt <model>.opmode{...}" is grouped by base model name with one
//     entry per binding (mode qualifier + port widths).
//   - multiply / adder: optional. when absent, multiplyModes is empty and
//     multiply.present / adder.present are false.
//   - hardblockModels: every pb_type with blif_model ".subckt <name>" is
//     recorded with max port widths and sorted multiply-style modes from the
//     'a' input when present (full blif name, including opmode suffixes).
//   - clockedModels: model names that declare an is_clock input port.
//   - lutK / lutK1: fracturable lut geometry when detected (0 otherwise).
//
// the xml is parsed with pugixml from vtr's vendored copy
// (libs/EXTERNAL/libpugixml), so there is no libarchfpga/libvtrutil link and
// the plugin keeps building against yosys alone.
//
// synth-facts checklist (expand scanner + goldens when claiming support):
//   [x] bram sp/dp modes (addr/data widths; aliased model names)
//   [x] opmode-qualified hardblockModes (titan / stratix style)
//   [x] multiply modes from 'a' port; adder presence + carryChain
//   [x] hardblockModels port widths + clock pins folded into inputs
//   [x] lutK / lutK1 fracturable lut geometry
//   [x] clockedModels from <models> is_clock
//   [ ] timing / delay annotations (not needed for techmap legality)
//   [ ] non-'a' mode axes / asymmetric mult ports beyond max widths
//   [ ] memory depth-splitting metadata beyond scanned modes
//   [ ] full mode hierarchy parity with libarchfpga (optional later)
namespace mosaic {

// classic vtr hardblock model names; may be overridden via -alias on
// vtr_arch_rules or alias* knobs in arch_config.tcl.
struct ClassicModelNames {
  std::string multiply = "multiply";
  std::string adder = "adder";
  std::string singlePortRam = "single_port_ram";
  std::string dualPortRam = "dual_port_ram";
};

// one concrete bram mode from a pb_type with blif_model
// ".subckt single_port_ram" / ".subckt dual_port_ram" (or those names under
// an .opmode{...} qualifier).
struct BramModeInfo {
  std::string name;
  int addrBitsA = 0;
  int dataBitsA = 0;
  bool isSp = false;
  int addrBitsB = 0; // dp only; falls back to addrBitsA when absent
  int dataBitsB = 0; // dp only; falls back to dataBitsA when absent
};

// one pb_type binding for a hardblock whose blif_model carries an opmode
// qualifier, e.g. ".subckt stratixiv_ram_block.opmode{single_port}....".
// port widths come from direct <input>/<output>/<clock> children (clocks are
// folded into inputWidths). classic arches without opmode leave
// hardblockModes empty; titan-style arches populate it heavily.
struct GenericHardblockMode {
  std::string modelName;     ///< base model before .opmode
  std::string modeQualifier; ///< contents of opmode{...}
  std::string pbTypeName;    ///< pb_type name attribute
  std::map<std::string, int> inputWidths;  ///< inputs + clocks
  std::map<std::string, int> outputWidths;
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

// hardblock model presence and geometry derived from hardblockModels.
// adder.carryChain is true when cin/cout/sumout ports are all present
// (classic vtr carry-chain element).
struct HardblockInfo {
  bool present = false;
  int aWidth = 0;
  bool carryChain = false;
};

struct VtrArchInfo {
  // <models>: names of models with an is_clock input port
  std::vector<std::string> clockedModels;

  // pb_type bram modes (classic model names only; may be empty for titan)
  std::vector<BramModeInfo> bramModes;

  // opmode-qualified hardblock bindings, keyed by base model name
  // (e.g. "stratixiv_ram_block" -> [{single_port, ...}, {dual_port, ...}]).
  std::map<std::string, std::vector<GenericHardblockMode>> hardblockModes;

  // lut fracturability from multi-mode pb_types containing .names luts:
  // lutK = size of the single-lut mode, lutK1 = max sub-lut size in the
  // fractured mode (0 when not detected / not fracturable)
  int lutK = 0;
  int lutK1 = 0;

  // every pb_type blif_model ".subckt <name>" binding, keyed by model name
  // (includes the ram models and any exotic combinational blocks the arch carries).
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
                  const ClassicModelNames &classic = ClassicModelNames(),
                  std::string *errorOut = nullptr);

} // namespace mosaic

#endif // MOSAIC_VTR_ARCH_INFO_H
