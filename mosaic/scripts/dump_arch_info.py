#!/usr/bin/env python3
"""offline dump of mosaic VtrArchInfo fields for regression goldens.

this script mirrors mosaic/src/vtr_arch_info.cc with xml.etree so goldens can
be refreshed without a yosys build. use --update to rewrite the checked in files.

usage:
  python mosaic/scripts/dump_arch_info.py <arch.xml>
  python mosaic/scripts/dump_arch_info.py --update
  python mosaic/scripts/dump_arch_info.py --help
"""

from __future__ import print_function

import argparse
import json
import sys
from pathlib import Path
import xml.etree.ElementTree as ET

SCRIPT_DIR = Path(__file__).resolve().parent
MOSAIC_ROOT = SCRIPT_DIR.parent
REPO_ROOT = MOSAIC_ROOT.parent
GOLDEN_DIR = MOSAIC_ROOT / "tests" / "golden"
FIXTURE_DIR = MOSAIC_ROOT / "tests" / "fixtures"

DEFAULT_ARCHES = [
    REPO_ROOT / "vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml",
    REPO_ROOT
    / "vtr_flow/arch/COFFE_22nm/k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml",
    FIXTURE_DIR / "min_bram_adder_no_mult.xml",
    FIXTURE_DIR / "min_exotic_integer_mul.xml",
]


def is_truthy(value):
    """return whether value is truthy and not a false-like string."""
    return bool(value) and value not in ("0", "false")


def collect_all(node, tag, out):
    """recursively collect all descendant elements matching tag."""
    if node.tag == tag:
        out.append(node)
    for child in node:
        collect_all(child, tag, out)


def attr_int(node, key, fallback=0):
    """return an integer attribute from node, or fallback on missing/bad value."""
    value = node.get(key)
    if value is None:
        return fallback
    try:
        return int(value)
    except ValueError:
        return fallback


def direct_pins(node, tag):
    """return list of (name, num_pins) pairs for direct child pins."""
    return [(pin.get("name", ""), attr_int(pin, "num_pins", 1)) for pin in node.findall(tag)]


def pin_width(pins, name):
    """look up a pin width by name from a list of (name, width) pairs."""
    for pin_name, width in pins:
        if pin_name == name:
            return width
    return 0


def scan_models(root, info):
    """scan <models> for clocked model names."""
    for models_node in root.findall("models"):  # pylint: disable=too-many-nested-blocks
        for model in models_node.findall("model"):
            for input_ports in model.findall("input_ports"):
                for port in input_ports.findall("port"):
                    if is_truthy(port.get("is_clock", "")):
                        name = model.get("name", "")
                        if name and name not in info["clockedModels"]:
                            info["clockedModels"].append(name)


def parse_opmode_blif(blif):
    """parse a .subckt blif string with an opmode qualifier."""
    prefix = ".subckt "
    if not blif.startswith(prefix):
        return None
    rest = blif[len(prefix) :]
    opmode_tag = ".opmode{"
    op_pos = rest.find(opmode_tag)
    if op_pos <= 0:
        return None
    qual_start = op_pos + len(opmode_tag)
    qual_end = rest.find("}", qual_start)
    if qual_end < 0:
        return None
    model_name = rest[:op_pos]
    mode_qualifier = rest[qual_start:qual_end]
    if not model_name or not mode_qualifier:
        return None
    return model_name, mode_qualifier


def fill_port_widths(pb):
    """collect input and output port widths from a pb_type element."""
    input_widths = {}
    output_widths = {}
    for pin_name, width in direct_pins(pb, "input"):
        input_widths[pin_name] = width
    for pin_name, width in direct_pins(pb, "clock"):
        input_widths[pin_name] = width
    for pin_name, width in direct_pins(pb, "output"):
        output_widths[pin_name] = width
    return input_widths, output_widths


def scan_generic_modes(pb_types, info):
    """scan pb_types for opmode-qualified hardblock bindings."""
    for pb in pb_types:
        parsed = parse_opmode_blif(pb.get("blif_model", ""))
        if parsed is None:
            continue
        model_name, mode_qualifier = parsed
        input_widths, output_widths = fill_port_widths(pb)
        info["hardblockModes"].setdefault(model_name, []).append(
            {
                "modelName": model_name,
                "modeQualifier": mode_qualifier,
                "pbTypeName": pb.get("name", ""),
                "inputWidths": input_widths,
                "outputWidths": output_widths,
            }
        )


def map_pin_width(pins, name):
    """look up a pin width by name from a dict."""
    return pins.get(name, 0)


def classic_bram_from_generic(generic, is_sp):
    """build a classic BRAM mode dict from a generic hardblock binding."""
    mode = {
        "name": generic["pbTypeName"],
        "isSp": is_sp,
        "addrBitsA": map_pin_width(generic["inputWidths"], "addr" if is_sp else "addr1"),
        "dataBitsA": map_pin_width(generic["inputWidths"], "data" if is_sp else "data1"),
        "addrBitsB": map_pin_width(generic["inputWidths"], "addr2"),
        "dataBitsB": map_pin_width(generic["inputWidths"], "data2"),
    }
    if mode["addrBitsB"] == 0:
        mode["addrBitsB"] = mode["addrBitsA"]
    if mode["dataBitsB"] == 0:
        mode["dataBitsB"] = mode["dataBitsA"]
    if mode["addrBitsA"] > 0 and mode["dataBitsA"] > 0:
        return mode
    return None


def scan_bram_modes(pb_types, info):
    """scan pb_types for classic single/dual port RAM modes."""
    for pb in pb_types:
        blif = pb.get("blif_model", "")
        is_sp = blif == ".subckt single_port_ram"
        is_dp = blif == ".subckt dual_port_ram"
        if not is_sp and not is_dp:
            continue
        pins = direct_pins(pb, "input")
        mode = {
            "name": pb.get("name", ""),
            "isSp": is_sp,
            "addrBitsA": pin_width(pins, "addr" if is_sp else "addr1"),
            "dataBitsA": pin_width(pins, "data" if is_sp else "data1"),
            "addrBitsB": pin_width(pins, "addr2"),
            "dataBitsB": pin_width(pins, "data2"),
        }
        if mode["addrBitsB"] == 0:
            mode["addrBitsB"] = mode["addrBitsA"]
        if mode["dataBitsB"] == 0:
            mode["dataBitsB"] = mode["dataBitsA"]
        if mode["addrBitsA"] > 0 and mode["dataBitsA"] > 0:
            info["bramModes"].append(mode)

    for model, is_sp in (("single_port_ram", True), ("dual_port_ram", False)):
        for generic in info["hardblockModes"].get(model, []):
            mode = classic_bram_from_generic(generic, is_sp)
            if mode is not None:
                info["bramModes"].append(mode)


def lut_size(pb):
    """return the input width of a LUT pb_type."""
    for input_pin in pb.findall("input"):
        return attr_int(input_pin, "num_pins", 0)
    return 0


def scan_lut_cost(pb_types, info):
    """scan pb_types for LUT sizes in fracturable modes."""
    single_k = 0
    sub_k = 0
    for pb in pb_types:  # pylint: disable=too-many-nested-blocks
        modes = pb.findall("mode")
        if len(modes) < 2:
            continue
        for mode in modes:
            luts = []
            for child in mode.findall("pb_type"):
                if child.get("blif_model") == ".names":
                    luts.append((lut_size(child), attr_int(child, "num_pb", 1)))
                else:
                    for grand in child.findall("pb_type"):
                        if grand.get("blif_model") == ".names":
                            luts.append(
                                (
                                    lut_size(grand),
                                    attr_int(grand, "num_pb", 1)
                                    * attr_int(child, "num_pb", 1),
                                )
                            )
            if not luts:
                continue
            total = sum(count for _, count in luts)
            max_size = max(size for size, _ in luts)
            if total == 1:
                single_k = max(single_k, max_size)
            elif total >= 2:
                sub_k = max(sub_k, max_size)
    info["lutK"] = single_k
    info["lutK1"] = sub_k


def scan_hardblock_models(pb_types, info):
    """scan pb_types for hardblock model geometry (port widths and modes)."""
    prefix = ".subckt "
    for pb in pb_types:
        blif = pb.get("blif_model", "")
        if not blif.startswith(prefix):
            continue
        model_name = blif[len(prefix) :]
        geo = info["hardblockModels"].setdefault(
            model_name, {"inputWidths": {}, "outputWidths": {}, "modes": []}
        )
        inputs = direct_pins(pb, "input")
        for pin_name, width in inputs:
            geo["inputWidths"][pin_name] = max(geo["inputWidths"].get(pin_name, 0), width)
        for pin_name, width in direct_pins(pb, "clock"):
            geo["inputWidths"][pin_name] = max(geo["inputWidths"].get(pin_name, 0), width)
        for pin_name, width in direct_pins(pb, "output"):
            geo["outputWidths"][pin_name] = max(
                geo["outputWidths"].get(pin_name, 0), width
            )
        a_width = pin_width(inputs, "a")
        if a_width > 0 and a_width not in geo["modes"]:
            geo["modes"].append(a_width)
    for geo in info["hardblockModels"].values():
        geo["modes"].sort()


def derive_hardblock_aliases(info):
    """populate top-level multiply/adder fields from hardblock model data."""
    mult = info["hardblockModels"].get("multiply")
    if mult and mult["modes"]:
        info["multiply"] = {
            "present": True,
            "aWidth": mult["modes"][-1],
            "carryChain": False,
        }
        info["multiplyModes"] = list(mult["modes"])
    add = info["hardblockModels"].get("adder")
    if add:
        a_width = add["inputWidths"].get("a", 0)
        if a_width > 0:
            has_cin = "cin" in add["inputWidths"]
            has_cout = "cout" in add["outputWidths"]
            has_sumout = "sumout" in add["outputWidths"]
            info["adder"] = {
                "present": True,
                "aWidth": a_width,
                "carryChain": has_cin and has_cout and has_sumout,
            }


def read_arch_info(xml_path):
    """parse an arch XML into the same summary fields VtrArchInfo exposes."""
    tree = ET.parse(str(xml_path))
    root = tree.getroot()
    info = {
        "archPath": str(xml_path),
        "archName": xml_path.stem,
        "clockedModels": [],
        "bramModes": [],
        "hardblockModes": {},
        "lutK": 0,
        "lutK1": 0,
        "hardblockModels": {},
        "adder": {"present": False, "aWidth": 0, "carryChain": False},
        "multiply": {"present": False, "aWidth": 0, "carryChain": False},
        "multiplyModes": [],
    }
    pb_types = []
    collect_all(root, "pb_type", pb_types)
    scan_models(root, info)
    scan_generic_modes(pb_types, info)
    scan_bram_modes(pb_types, info)
    scan_lut_cost(pb_types, info)
    scan_hardblock_models(pb_types, info)
    derive_hardblock_aliases(info)
    return info


def summary_text(info):
    """format arch info as a multi-line summary string."""
    max_ram_abits = 0
    for mode in info["bramModes"]:
        max_ram_abits = max(max_ram_abits, mode["addrBitsA"])
    multiply_present = info["multiply"]["present"] and bool(info["multiplyModes"])
    lines = [
        "archName: {}".format(info["archName"]),
        "vtrRamAbits: {}".format(max_ram_abits),
        "dspMaxWidth: {}".format(info["multiplyModes"][-1] if multiply_present else 0),
        "dspMinWidth: {}".format(info["multiplyModes"][0] if multiply_present else 0),
        "multiplyPresent: {}".format(1 if multiply_present else 0),
        "adderPresent: {}".format(1 if info["adder"]["present"] else 0),
        "adderCarryChain: {}".format(1 if info["adder"].get("carryChain") else 0),
        "multiplyModes: {}".format(" ".join(str(m) for m in info["multiplyModes"])),
        "lutK: {}".format(info["lutK"]),
        "lutK1: {}".format(info["lutK1"]),
        "bramModeCount: {}".format(len(info["bramModes"])),
        "clockedModelCount: {}".format(len(info["clockedModels"])),
        "hardblockModelCount: {}".format(len(info["hardblockModels"])),
    ]
    return "\n".join(lines) + "\n"


def list_modes_text(info):
    """mirror vtr_arch_rules -list-modes for offline titan checks."""
    lines = ["classic bramModes ({})".format(len(info["bramModes"]))]
    for mode in info["bramModes"]:
        lines.append(
            "  {} {} addrA={} dataA={} addrB={} dataB={}".format(
                "sp" if mode["isSp"] else "dp",
                mode["name"],
                mode["addrBitsA"],
                mode["dataBitsA"],
                mode["addrBitsB"],
                mode["dataBitsB"],
            )
        )
    lines.append("hardblockModes ({} models)".format(len(info["hardblockModes"])))
    for model_name in sorted(info["hardblockModes"]):
        modes = info["hardblockModes"][model_name]
        lines.append("  model '{}' ({} bindings):".format(model_name, len(modes)))
        by_qual = {}
        for mode in modes:
            by_qual.setdefault(mode["modeQualifier"], []).append(mode)
        for qual in sorted(by_qual):
            sample = by_qual[qual][0]
            in_ports = ",".join(
                "{}:{}".format(k, sample["inputWidths"][k])
                for k in sorted(sample["inputWidths"])
            )
            out_ports = ",".join(
                "{}:{}".format(k, sample["outputWidths"][k])
                for k in sorted(sample["outputWidths"])
            )
            lines.append(
                "    opmode{{{}}} count={} sample_pb={} in={{{}}} out={{{}}}".format(
                    qual,
                    len(by_qual[qual]),
                    sample["pbTypeName"],
                    in_ports,
                    out_ports,
                )
            )
    return "\n".join(lines) + "\n"


def golden_path_for(arch_path):
    """return the golden summary file path for an architecture."""
    return GOLDEN_DIR / (Path(arch_path).stem + ".summary.txt")


def dump_arch(arch_path, update=False):
    """emit or compare a golden summary for one arch XML."""
    arch_path = Path(arch_path)
    if not arch_path.is_file():
        raise SystemExit("arch file not found: {}".format(arch_path))
    info = read_arch_info(arch_path)
    text = summary_text(info)
    golden_path = golden_path_for(arch_path)
    if update:
        GOLDEN_DIR.mkdir(parents=True, exist_ok=True)
        golden_path.write_text(text, encoding="utf-8")
        print("updated {}".format(golden_path))
        return 0
    if golden_path.is_file():
        expected = golden_path.read_text(encoding="utf-8")
        if expected != text:
            print("mismatch for {}".format(arch_path), file=sys.stderr)
            print("expected:\n{}".format(expected), file=sys.stderr)
            print("actual:\n{}".format(text), file=sys.stderr)
            return 1
        print("ok {}".format(arch_path.stem))
        return 0
    print(text, end="")
    return 0


def main():
    """entry point: parse args and dump arch summaries."""
    parser = argparse.ArgumentParser(
        description="dump mosaic VtrArchInfo summary for an arch xml"
    )
    parser.add_argument(
        "arch_xml",
        nargs="*",
        help="architecture xml path(s); default: k6, koios, and test fixture",
    )
    parser.add_argument(
        "--update",
        action="store_true",
        help="rewrite golden summaries under mosaic/tests/golden/",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="emit full parsed structure as json instead of summary text",
    )
    parser.add_argument(
        "--list-modes",
        action="store_true",
        help="print hardblockModes / bramModes (offline -list-modes)",
    )
    args = parser.parse_args()
    arch_list = [Path(p) for p in args.arch_xml] if args.arch_xml else DEFAULT_ARCHES
    exit_code = 0
    for arch_path in arch_list:
        if args.json or args.list_modes:
            info = read_arch_info(arch_path)
            if args.json:
                print(json.dumps(info, indent=2, sort_keys=True))
            if args.list_modes:
                print(list_modes_text(info), end="")
            continue
        code = dump_arch(arch_path, update=args.update)
        exit_code = max(exit_code, code)
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
