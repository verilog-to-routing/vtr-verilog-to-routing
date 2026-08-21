#!/usr/bin/env python3
# convert a blif netlist to verilog via yosys for verilator elaboration.

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Tuple


# hardblock ports that are vectors in sim_hardblocks.v. scalar ports stay as-is
HARDBLOCK_VECTOR_PORTS: Dict[str, Tuple[str, ...]] = {
    "adder": ("a", "b", "sumout"),
    "multiply": ("a", "b", "out"),
    "single_port_ram": ("addr", "data", "out"),
    "dual_port_ram": ("addr1", "addr2", "data1", "data2", "out1", "out2"),
}


# USE: locate a yosys binary for blif to verilog conversion.
def resolveYosys(yosysPath: Path | None = None) -> Path:
    if yosysPath is not None and yosysPath.is_file():
        return yosysPath
    repoRoot = Path(__file__).resolve().parents[2]
    candidates = [
        repoRoot / "build" / "bin" / "yosys",
        repoRoot / "yosys" / "yosys",
    ]
    for path in candidates:
        if path.is_file():
            return path
    which = subprocess.run(["which", "yosys"], capture_output=True, text=True, check=False)
    if which.returncode == 0 and which.stdout.strip():
        return Path(which.stdout.strip())
    raise FileNotFoundError("yosys not found (pass --yosys or build vtr)")


# USE: remove empty/blackbox module bodies that conflict with sim_hardblocks.v.
def stripHardblockModuleDefs(verilogText: str) -> str:
    hardblocks = {
        "adder", "multiply", "single_port_ram", "dual_port_ram",
        "mux", "fpga_interconnect", "dff", "dffl", "dffe", "latch",
    }
    pattern = re.compile(
        r"^\s*module\s+(\w+)\b.*?^\s*endmodule\s*",
        re.M | re.S,
    )

    def keepOrDrop(match: re.Match) -> str:
        name = match.group(1)
        if name in hardblocks:
            return f"// stripped blackbox module {name} (provided by sim_hardblocks.v)\n"
        return match.group(0)

    return pattern.sub(keepOrDrop, verilogText)


# HELPER: split '.port(net),' list on top-level commas.
def _splitPortConnections(body: str) -> List[str]:
    parts: List[str] = []
    depth = 0
    start = 0
    for i, ch in enumerate(body):
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        elif ch == "," and depth == 0:
            part = body[start:i].strip()
            if part:
                parts.append(part)
            start = i + 1
    tail = body[start:].strip()
    if tail:
        parts.append(tail)
    return parts


# HELPER: trim net expr but keep trailing space after escaped identifiers.
# verilator requires a space after \\escaped ids before ',' ')' or '}'.
# strip() removes that space and breaks compile.
def _normalizeNetExpr(net: str) -> str:
    net = net.strip()
    if net.startswith("\\") and not net.endswith(" "):
        net = net + " "
    return net


# USE: rewrite .\\a[0](n0), .\\a[1](n1) into .a({n1, n0}) for sim_hardblocks vector ports.
# yosys write_verilog keeps blif-style bitblasted pin names after read_blif of
# blackbox .model definitions. verilator then fails with PINNOTFOUND against
# the vector ports in sim_hardblocks.v. packing here is the fix.
# instances with no bit-blasted pins are left unchanged so we do not strip the
# trailing spaces yosys already emitted on escaped net names.
def packBitBlastedHardblockPorts(verilogText: str) -> str:
    # yosys emits .\a[0] (net) as '.' plus escaped-id '\a[0] '
    # abc/parmys blif uses a~0 instead of a[0]
    bitPinRe = re.compile(
        r"^\.\\(?P<port>[A-Za-z_]\w*)(?:\[(?P<br>\d+)\]|~(?P<tilde>\d+))\s*\((?P<net>.*)\)$"
    )
    tildePinRe = re.compile(
        r"^\.(?P<port>[A-Za-z_]\w*)~(?P<tilde>\d+)\s*\((?P<net>.*)\)$"
    )
    plainPinRe = re.compile(r"^\.(?P<port>[A-Za-z_]\w*)\s*\((?P<net>.*)\)$")
    instPattern = re.compile(
        r"(?P<indent>^[ \t]*)(?P<cell>adder|multiply|single_port_ram|dual_port_ram)"
        r"(?P<gap>\s+)(?P<inst>\S+)\s*\((?P<body>.*?)\);",
        re.M | re.S,
    )

    def rewriteInstance(match: re.Match) -> str:
        cell = match.group("cell")
        vectorPorts = set(HARDBLOCK_VECTOR_PORTS.get(cell, ()))
        conns = _splitPortConnections(match.group("body"))

        blasted: Dict[str, Dict[int, str]] = {}
        plain: List[Tuple[str, str]] = []
        for conn in conns:
            conn = conn.strip().rstrip(",")
            mBit = bitPinRe.match(conn) or tildePinRe.match(conn)
            if mBit and mBit.group("port") in vectorPorts:
                port = mBit.group("port")
                idx = int(mBit.group("br") or mBit.group("tilde"))
                net = _normalizeNetExpr(mBit.group("net"))
                blasted.setdefault(port, {})[idx] = net
                continue
            mPlain = plainPinRe.match(conn)
            if mPlain:
                plain.append(
                    (mPlain.group("port"), _normalizeNetExpr(mPlain.group("net")))
                )
                continue
            plain.append((f"_raw_{len(plain)}", conn))

        # leave non-blasted instances alone to preserve yosys spacing / formatting
        if not blasted:
            return match.group(0)

        newConns: List[str] = []
        for port, bits in blasted.items():
            maxIdx = max(bits)
            ordered = [bits.get(i, "1'b0") for i in range(maxIdx + 1)]
            concat = "{" + ", ".join(reversed(ordered)) + "}"
            newConns.append(f".{port}({concat})")
        for port, net in plain:
            if port.startswith("_raw_"):
                newConns.append(net)
            else:
                newConns.append(f".{port}({net})")

        indent = match.group("indent")
        inner = ",\n".join(f"{indent}    {c}" for c in newConns)
        return (
            f"{indent}{cell}{match.group('gap')}{match.group('inst')} (\n"
            f"{inner}\n{indent});"
        )

    return instPattern.sub(rewriteInstance, verilogText)


# pack abc ~bit and yosys [bit] top ports into rtl-style vectors
def packBitBlastedTopPorts(verilogText: str) -> str:
    modMatch = re.search(
        r"(?P<head>^\s*module\s+(?P<name>\w+)\s*\()(?P<ports>.*?)(?P<tail>\)\s*;)",
        verilogText,
        re.M | re.S,
    )
    if not modMatch:
        return verilogText

    rawPorts = [p.strip() for p in modMatch.group("ports").split(",") if p.strip()]
    bitPortRe = re.compile(
        r"^\\?(?P<base>[A-Za-z_][A-Za-z0-9_]*)(?:\[(?P<br>\d+)\]|~(?P<tilde>\d+))\s*$"
    )

    vectorBits: Dict[str, List[int]] = {}
    for port in rawPorts:
        m = bitPortRe.match(port)
        if m:
            idx = int(m.group("br") or m.group("tilde"))
            vectorBits.setdefault(m.group("base"), []).append(idx)

    if not vectorBits:
        return verilogText

    newPortList: List[str] = []
    seenVectors = set()
    for port in rawPorts:
        m = bitPortRe.match(port)
        if not m:
            if port.startswith("\\") and "[" not in port and "~" not in port:
                newPortList.append(port.lstrip("\\").strip())
            else:
                newPortList.append(port)
            continue
        base = m.group("base")
        if base not in seenVectors:
            newPortList.append(base)
            seenVectors.add(base)

    newHeader = (
        f"{modMatch.group('head')}{', '.join(newPortList)}{modMatch.group('tail')}"
    )
    text = verilogText[: modMatch.start()] + newHeader + verilogText[modMatch.end() :]

    dirRe = re.compile(
        r"^\s*(?P<dir>input|output|inout)(?:\s+wire|\s+reg)?\s+"
        r"\\?(?P<base>[A-Za-z_][A-Za-z0-9_]*)(?:\[(?P<br>\d+)\]|~(?P<tilde>\d+))\s*;\s*$",
        re.M,
    )
    dirByBase: Dict[str, str] = {}
    for m in dirRe.finditer(text):
        base = m.group("base")
        dirByBase.setdefault(base, m.group("dir"))

    text = dirRe.sub("", text)

    declLines = []
    for base, idxs in vectorBits.items():
        direction = dirByBase.get(base, "input")
        msb = max(idxs)
        declLines.append(f"  {direction} [{msb}:0] {base};")
    if declLines:
        text = re.sub(
            r"(module\s+\w+\s*\([^;]*\)\s*;)",
            r"\1\n" + "\n".join(declLines),
            text,
            count=1,
            flags=re.S,
        )

    for base in vectorBits:
        text = re.sub(
            rf"\\{re.escape(base)}\[(\d+)\](\s?)",
            rf"{base}[\1]\2",
            text,
        )
        text = re.sub(
            rf"\\{re.escape(base)}~(\d+)(\s?)",
            rf"{base}[\1]\2",
            text,
        )
        text = re.sub(
            rf"(?<![A-Za-z0-9_]){re.escape(base)}~(\d+)",
            rf"{base}[\1]",
            text,
        )
    return text


# USE: ensure unconn/gnd/vcc wires exist when nets reference them.
def declareConstNets(verilogText: str) -> str:
    needs = []
    if re.search(r"\bunconn\b", verilogText) and not re.search(
        r"^\s*wire\s+unconn\s*;", verilogText, re.M
    ):
        needs.append("  wire unconn = 1'b0;")
    if re.search(r"\bgnd\b", verilogText) and not re.search(
        r"^\s*wire\s+gnd\s*;", verilogText, re.M
    ):
        needs.append("  wire gnd = 1'b0;")
    if re.search(r"\bvcc\b", verilogText) and not re.search(
        r"^\s*wire\s+vcc\s*;", verilogText, re.M
    ):
        needs.append("  wire vcc = 1'b1;")
    if not needs:
        return verilogText
    return re.sub(
        r"(module\s+\w+\b[^;]*;)",
        r"\1\n" + "\n".join(needs),
        verilogText,
        count=1,
        flags=re.S,
    )


# USE: read_blif, optional hierarchy -top, then write_verilog -noattr.
def blifToVerilog(
    blifPath: Path,
    outVerilog: Path,
    *,
    topName: str | None = None,
    yosysPath: Path | None = None,
    logPath: Path | None = None,
) -> None:
    yosys = resolveYosys(yosysPath)
    outVerilog.parent.mkdir(parents=True, exist_ok=True)
    # -wideports packs top-level a[0],a[1] into buses when possible. hardblock
    # blackbox pins often still come out bit-blasted and need python packing.
    scriptLines = [
        f"read_blif -wideports {blifPath.resolve().as_posix()}",
    ]
    if topName:
        scriptLines.append(f"hierarchy -top {topName}")
    else:
        scriptLines.append("hierarchy -auto-top")
    # flatten keeps a single module for the tb to instantiate under a rename
    scriptLines.append("flatten")
    scriptLines.append(f"write_verilog -noattr {outVerilog.resolve().as_posix()}")
    cmd = [str(yosys), "-p", "; ".join(scriptLines)]
    result = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if logPath is not None:
        logPath.parent.mkdir(parents=True, exist_ok=True)
        logPath.write_text(result.stdout + "\n" + result.stderr, encoding="utf-8")
    if result.returncode != 0 or not outVerilog.is_file():
        raise RuntimeError(
            f"yosys blif-->verilog failed for {blifPath} "
            f"(rc={result.returncode}); see {logPath}"
        )
    cleaned = stripHardblockModuleDefs(
        outVerilog.read_text(encoding="utf-8", errors="replace")
    )
    cleaned = packBitBlastedHardblockPorts(cleaned)
    cleaned = packBitBlastedTopPorts(cleaned)
    cleaned = declareConstNets(cleaned)
    outVerilog.write_text(cleaned, encoding="utf-8")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        description="convert a blif netlist to verilog via yosys"
    )
    parser.add_argument("blif", type=Path)
    parser.add_argument("-o", "--output", type=Path, required=True)
    parser.add_argument("--top", default=None, help="top module name (default: auto-top)")
    parser.add_argument("--yosys", type=Path, default=None)
    parser.add_argument("--log", type=Path, default=None)
    args = parser.parse_args(argv)
    if not args.blif.is_file():
        print(f"error: missing blif {args.blif}", file=sys.stderr)
        return 1
    try:
        blifToVerilog(
            args.blif,
            args.output,
            topName=args.top,
            yosysPath=args.yosys,
            logPath=args.log,
        )
    except (FileNotFoundError, RuntimeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
