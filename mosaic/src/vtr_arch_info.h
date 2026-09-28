#ifndef MOSAIC_VTR_ARCH_INFO_H
#define MOSAIC_VTR_ARCH_INFO_H

#include <map>
#include <string>
#include <vector>

// this file defines the arch facts mosaic reads from a VTR architecture xml at
// synthesis time. readArchInfo fills a VtrArchInfo so rule generation can size
// maps and stubs without linking libarchfpga.

// classic rams land in bramModes when the arch names single_port_ram or
// dual_port_ram (plain ".subckt <name>" or with ".opmode{...}"). titan style
// arches often leave bramModes empty, and callers that emit classic bram maps
// must treat a missing sp or dp mode as an error after the parse.

// every ".subckt <model>" binding goes into hardblockModels with max port widths,
// and multiply style mode widths come from the a input when that port exists.
// multiply and adder are optional convenience views of that map, so when either
// model is absent its present flag is false and multiplyModes stays empty.

// hardblockModes holds opmode qualified bindings such as
// ".subckt stratixiv_ram_block.opmode{single_port}", grouped by base model name.
// clockedModels names models with an is_clock input, and lutK / lutK1 capture
// fracturable lut geometry when the xml exposes it (otherwise both stay 0).

// the xml is parsed with pugixml from VTR's vendored copy under
// libs/EXTERNAL/libpugixml so the plugin builds against Yosys alone.
namespace mosaic {

// classic VTR hardblock model names. may be overridden via -alias on
// vtr_arch_rules or alias* knobs in arch_config.tcl.
struct ClassicModelNames {
    std::string multiply = "multiply";
    std::string adder = "adder";
    std::string singlePortRam = "single_port_ram";
    std::string dualPortRam = "dual_port_ram";
};

// one concrete bram mode from a pb_type with blif_model
// ".subckt single_port_ram" or ".subckt dual_port_ram", or those names under
// an .opmode{...} qualifier.
struct BramModeInfo {
    std::string name;
    int addrBitsA = 0;
    int dataBitsA = 0;
    bool isSp = false;
    // dp only. falls back to addrBitsA when absent.
    int addrBitsB = 0;
    // dp only. falls back to dataBitsA when absent.
    int dataBitsB = 0;
};

// one pb_type binding for a hardblock whose blif_model carries an opmode
// qualifier, e.g. ".subckt stratixiv_ram_block.opmode{single_port}....".
// port widths come from direct <input>, <output>, and <clock> children. clocks
// are folded into inputWidths. classic arches without opmode leave
// hardblockModes empty. titan-style arches populate it heavily.
struct GenericHardblockMode {
    // base model before .opmode
    std::string modelName;
    // contents of opmode{...}
    std::string modeQualifier;
    // pb_type name attribute
    std::string pbTypeName;
    // inputs plus clocks
    std::map<std::string, int> inputWidths;
    std::map<std::string, int> outputWidths;
};

// generic geometry of one hardblock model, gathered from every pb_type that
// binds blif_model ".subckt <name>". the <models> section carries no sizes.
// port widths are the max num_pins seen per port name across bindings. <clock>
// pins are folded into inputWidths. modes holds the sorted distinct widths of
// the a input port (multiply-style mode geometry, empty for models without an
// a input).
struct ModelGeometry {
    std::map<std::string, int> inputWidths;
    std::map<std::string, int> outputWidths;
    std::vector<int> modes;
};

// hardblock model presence and geometry derived from hardblockModels.
// adder.carryChain is true when cin, cout, and sumout ports are all present
// (classic VTR carry-chain element).
struct HardblockInfo {
    bool present = false;
    int aWidth = 0;
    bool carryChain = false;
};

struct VtrArchInfo {
    // names of models with an is_clock input port from <models>
    std::vector<std::string> clockedModels;

    // pb_type bram modes for classic model names only. may be empty for titan.
    std::vector<BramModeInfo> bramModes;

    // opmode-qualified hardblock bindings keyed by base model name
    // (e.g. stratixiv_ram_block maps to single_port and dual_port bindings).
    std::map<std::string, std::vector<GenericHardblockMode>> hardblockModes;

    // lut fracturability from multi-mode pb_types containing .names luts.
    // lutK is the size of the single-lut mode. lutK1 is the max sub-lut size in
    // the fractured mode (0 when not detected or not fracturable).
    int lutK = 0;
    int lutK1 = 0;

    // every pb_type blif_model ".subckt <name>" binding keyed by model name.
    // includes ram models and any exotic combinational blocks the arch carries.
    std::map<std::string, ModelGeometry> hardblockModels;

    // adder and multiply convenience accessors derived from hardblockModels by
    // readArchInfo. multiplyModes holds per-mode operand widths (e.g. 9, 18, 36).
    // multiply.aWidth mirrors the widest mode.
    HardblockInfo adder;
    HardblockInfo multiply;
    std::vector<int> multiplyModes;
};

// USE: parse the arch xml at xmlPath into info. returns false and sets
// errorOut when non-null on read or parse failure. partial data may be present.
bool readArchInfo(const std::string &xmlPath, VtrArchInfo &info, const ClassicModelNames &classic = ClassicModelNames(),
                  std::string *errorOut = nullptr);

} // namespace mosaic

#endif // MOSAIC_VTR_ARCH_INFO_H
