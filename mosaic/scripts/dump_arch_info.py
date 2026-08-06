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


def isTruthy(value):
    return bool(value) and value not in ("0", "false")


def collectAll(node, tag, out):
    if node.tag == tag:
        out.append(node)
    for child in node:
        collectAll(child, tag, out)


def attrInt(node, key, fallback=0):
    value = node.get(key)
    if value is None:
        return fallback
    try:
        return int(value)
    except ValueError:
        return fallback


def directPins(node, tag):
    return [(pin.get("name", ""), attrInt(pin, "num_pins", 1)) for pin in node.findall(tag)]


def pinWidth(pins, name):
    for pinName, width in pins:
        if pinName == name:
            return width
    return 0


def scanModels(root, info):
    for modelsNode in root.findall("models"):
        for model in modelsNode.findall("model"):
            for inputPorts in model.findall("input_ports"):
                for port in inputPorts.findall("port"):
                    if isTruthy(port.get("is_clock", "")):
                        name = model.get("name", "")
                        if name and name not in info["clockedModels"]:
                            info["clockedModels"].append(name)


def parseOpmodeBlif(blif):
    prefix = ".subckt "
    if not blif.startswith(prefix):
        return None
    rest = blif[len(prefix) :]
    opmodeTag = ".opmode{"
    opPos = rest.find(opmodeTag)
    if opPos <= 0:
        return None
    qualStart = opPos + len(opmodeTag)
    qualEnd = rest.find("}", qualStart)
    if qualEnd < 0:
        return None
    modelName = rest[:opPos]
    modeQualifier = rest[qualStart:qualEnd]
    if not modelName or not modeQualifier:
        return None
    return modelName, modeQualifier


def fillPortWidths(pb):
    inputWidths = {}
    outputWidths = {}
    for pinName, width in directPins(pb, "input"):
        inputWidths[pinName] = width
    for pinName, width in directPins(pb, "clock"):
        inputWidths[pinName] = width
    for pinName, width in directPins(pb, "output"):
        outputWidths[pinName] = width
    return inputWidths, outputWidths


def scanGenericModes(pbTypes, info):
    for pb in pbTypes:
        parsed = parseOpmodeBlif(pb.get("blif_model", ""))
        if parsed is None:
            continue
        modelName, modeQualifier = parsed
        inputWidths, outputWidths = fillPortWidths(pb)
        info["hardblockModes"].setdefault(modelName, []).append(
            {
                "modelName": modelName,
                "modeQualifier": modeQualifier,
                "pbTypeName": pb.get("name", ""),
                "inputWidths": inputWidths,
                "outputWidths": outputWidths,
            }
        )


def mapPinWidth(pins, name):
    return pins.get(name, 0)


def classicBramFromGeneric(generic, isSp):
    mode = {
        "name": generic["pbTypeName"],
        "isSp": isSp,
        "addrBitsA": mapPinWidth(generic["inputWidths"], "addr" if isSp else "addr1"),
        "dataBitsA": mapPinWidth(generic["inputWidths"], "data" if isSp else "data1"),
        "addrBitsB": mapPinWidth(generic["inputWidths"], "addr2"),
        "dataBitsB": mapPinWidth(generic["inputWidths"], "data2"),
    }
    if mode["addrBitsB"] == 0:
        mode["addrBitsB"] = mode["addrBitsA"]
    if mode["dataBitsB"] == 0:
        mode["dataBitsB"] = mode["dataBitsA"]
    if mode["addrBitsA"] > 0 and mode["dataBitsA"] > 0:
        return mode
    return None


def scanBramModes(pbTypes, info):
    for pb in pbTypes:
        blif = pb.get("blif_model", "")
        isSp = blif == ".subckt single_port_ram"
        isDp = blif == ".subckt dual_port_ram"
        if not isSp and not isDp:
            continue
        pins = directPins(pb, "input")
        mode = {
            "name": pb.get("name", ""),
            "isSp": isSp,
            "addrBitsA": pinWidth(pins, "addr" if isSp else "addr1"),
            "dataBitsA": pinWidth(pins, "data" if isSp else "data1"),
            "addrBitsB": pinWidth(pins, "addr2"),
            "dataBitsB": pinWidth(pins, "data2"),
        }
        if mode["addrBitsB"] == 0:
            mode["addrBitsB"] = mode["addrBitsA"]
        if mode["dataBitsB"] == 0:
            mode["dataBitsB"] = mode["dataBitsA"]
        if mode["addrBitsA"] > 0 and mode["dataBitsA"] > 0:
            info["bramModes"].append(mode)

    for model, isSp in (("single_port_ram", True), ("dual_port_ram", False)):
        for generic in info["hardblockModes"].get(model, []):
            mode = classicBramFromGeneric(generic, isSp)
            if mode is not None:
                info["bramModes"].append(mode)


def lutSize(pb):
    for inputPin in pb.findall("input"):
        return attrInt(inputPin, "num_pins", 0)
    return 0


def scanLutCost(pbTypes, info):
    singleK = 0
    subK = 0
    for pb in pbTypes:
        modes = pb.findall("mode")
        if len(modes) < 2:
            continue
        for mode in modes:
            luts = []
            for child in mode.findall("pb_type"):
                if child.get("blif_model") == ".names":
                    luts.append((lutSize(child), attrInt(child, "num_pb", 1)))
                else:
                    for grand in child.findall("pb_type"):
                        if grand.get("blif_model") == ".names":
                            luts.append(
                                (
                                    lutSize(grand),
                                    attrInt(grand, "num_pb", 1)
                                    * attrInt(child, "num_pb", 1),
                                )
                            )
            if not luts:
                continue
            total = sum(count for _, count in luts)
            maxSize = max(size for size, _ in luts)
            if total == 1:
                singleK = max(singleK, maxSize)
            elif total >= 2:
                subK = max(subK, maxSize)
    info["lutK"] = singleK
    info["lutK1"] = subK


def scanHardblockModels(pbTypes, info):
    prefix = ".subckt "
    for pb in pbTypes:
        blif = pb.get("blif_model", "")
        if not blif.startswith(prefix):
            continue
        modelName = blif[len(prefix) :]
        geo = info["hardblockModels"].setdefault(
            modelName, {"inputWidths": {}, "outputWidths": {}, "modes": []}
        )
        inputs = directPins(pb, "input")
        for pinName, width in inputs:
            geo["inputWidths"][pinName] = max(geo["inputWidths"].get(pinName, 0), width)
        for pinName, width in directPins(pb, "clock"):
            geo["inputWidths"][pinName] = max(geo["inputWidths"].get(pinName, 0), width)
        for pinName, width in directPins(pb, "output"):
            geo["outputWidths"][pinName] = max(
                geo["outputWidths"].get(pinName, 0), width
            )
        aWidth = pinWidth(inputs, "a")
        if aWidth > 0 and aWidth not in geo["modes"]:
            geo["modes"].append(aWidth)
    for geo in info["hardblockModels"].values():
        geo["modes"].sort()


def deriveHardblockAliases(info):
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
        aWidth = add["inputWidths"].get("a", 0)
        if aWidth > 0:
            hasCin = "cin" in add["inputWidths"]
            hasCout = "cout" in add["outputWidths"]
            hasSumout = "sumout" in add["outputWidths"]
            info["adder"] = {
                "present": True,
                "aWidth": aWidth,
                "carryChain": hasCin and hasCout and hasSumout,
            }


# USE: parse an arch xml into the same summary fields VtrArchInfo exposes.
def readArchInfo(xmlPath):
    tree = ET.parse(str(xmlPath))
    root = tree.getroot()
    info = {
        "archPath": str(xmlPath),
        "archName": xmlPath.stem,
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
    pbTypes = []
    collectAll(root, "pb_type", pbTypes)
    scanModels(root, info)
    scanGenericModes(pbTypes, info)
    scanBramModes(pbTypes, info)
    scanLutCost(pbTypes, info)
    scanHardblockModels(pbTypes, info)
    deriveHardblockAliases(info)
    return info


def summaryText(info):
  maxRamAbits = 0
  for mode in info["bramModes"]:
      maxRamAbits = max(maxRamAbits, mode["addrBitsA"])
  multiplyPresent = info["multiply"]["present"] and bool(info["multiplyModes"])
  lines = [
      "archName: {}".format(info["archName"]),
      "vtrRamAbits: {}".format(maxRamAbits),
      "dspMaxWidth: {}".format(info["multiplyModes"][-1] if multiplyPresent else 0),
      "dspMinWidth: {}".format(info["multiplyModes"][0] if multiplyPresent else 0),
      "multiplyPresent: {}".format(1 if multiplyPresent else 0),
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


# USE: mirror vtr_arch_rules -list-modes for offline titan checks.
def listModesText(info):
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
    for modelName in sorted(info["hardblockModes"]):
        modes = info["hardblockModes"][modelName]
        lines.append("  model '{}' ({} bindings):".format(modelName, len(modes)))
        # summarize distinct opmode qualifiers and sample port widths
        byQual = {}
        for mode in modes:
            byQual.setdefault(mode["modeQualifier"], []).append(mode)
        for qual in sorted(byQual):
            sample = byQual[qual][0]
            inPorts = ",".join(
                "{}:{}".format(k, sample["inputWidths"][k])
                for k in sorted(sample["inputWidths"])
            )
            outPorts = ",".join(
                "{}:{}".format(k, sample["outputWidths"][k])
                for k in sorted(sample["outputWidths"])
            )
            lines.append(
                "    opmode{{{}}} count={} sample_pb={} in={{{}}} out={{{}}}".format(
                    qual,
                    len(byQual[qual]),
                    sample["pbTypeName"],
                    inPorts,
                    outPorts,
                )
            )
    return "\n".join(lines) + "\n"


def goldenPathFor(archPath):
    return GOLDEN_DIR / (Path(archPath).stem + ".summary.txt")


# USE: emit or compare a golden summary for one arch xml.
def dumpArch(archPath, update=False):
    archPath = Path(archPath)
    if not archPath.is_file():
        raise SystemExit("arch file not found: {}".format(archPath))
    info = readArchInfo(archPath)
    text = summaryText(info)
    goldenPath = goldenPathFor(archPath)
    if update:
        GOLDEN_DIR.mkdir(parents=True, exist_ok=True)
        goldenPath.write_text(text, encoding="utf-8")
        print("updated {}".format(goldenPath))
        return 0
    if goldenPath.is_file():
        expected = goldenPath.read_text(encoding="utf-8")
        if expected != text:
            print("mismatch for {}".format(archPath), file=sys.stderr)
            print("expected:\n{}".format(expected), file=sys.stderr)
            print("actual:\n{}".format(text), file=sys.stderr)
            return 1
        print("ok {}".format(archPath.stem))
        return 0
    print(text, end="")
    return 0


def main():
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
    archList = [Path(p) for p in args.arch_xml] if args.arch_xml else DEFAULT_ARCHES
    exitCode = 0
    for archPath in archList:
        if args.json or args.list_modes:
            info = readArchInfo(archPath)
            if args.json:
                print(json.dumps(info, indent=2, sort_keys=True))
            if args.list_modes:
                print(listModesText(info), end="")
            continue
        code = dumpArch(archPath, update=args.update)
        exitCode = max(exitCode, code)
    return exitCode


if __name__ == "__main__":
    sys.exit(main())
