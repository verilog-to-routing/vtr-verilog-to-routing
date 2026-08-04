#!/usr/bin/env python3
"""parse top-module ports from rtl verilog or blif for the random-check tb."""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional


@dataclass(frozen=True)
class PortDecl:
    name: str
    direction: str  # input | output | inout
    width: int = 1  # bit width; 1 for scalar


_CLOCK_NAMES = frozenset({
    "clk", "clock", "i_clk", "clk_a", "clk_b", "clk0", "clk1",
    "tm3_clk_v0", "clock_a", "clock_b",
})
_RESET_NAMES = frozenset({
    "rst", "reset", "reset_n", "rst_n", "i_reset", "areset", "nreset",
})


def isClockPort(name: str) -> bool:
    lower = name.lower()
    if lower in _CLOCK_NAMES:
        return True
    return lower.endswith("_clk") or lower.startswith("clk_")


def isResetPort(name: str) -> bool:
    lower = name.lower()
    if lower in _RESET_NAMES:
        return True
    return "reset" in lower or lower.endswith("_rst") or lower.startswith("rst_")


def _parseWidth(tokenBlock: str) -> int:
    m = re.search(r"\[\s*(\d+)\s*:\s*(\d+)\s*\]", tokenBlock)
    if not m:
        return 1
    msb, lsb = int(m.group(1)), int(m.group(2))
    return abs(msb - lsb) + 1


def _stripVerilogComments(text: str) -> str:
    """remove // and /* */ comments without treating //*** headers as /*."""
    # line comments first, vtr headers like //***** contain the substring /*
    text = re.sub(r"//.*?$", "", text, flags=re.M)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return text


def parseVerilogTopPorts(verilogPath: Path, topName: Optional[str] = None) -> List[PortDecl]:
    """parse ansi or non-ansi ports from the first / named module."""
    text = _stripVerilogComments(
        verilogPath.read_text(encoding="utf-8", errors="replace")
    )

    if topName:
        modRe = re.compile(
            rf"module\s+{re.escape(topName)}\s*(?:#\s*\([^;]*?\))?\s*\((.*?)\)\s*;",
            re.S,
        )
    else:
        modRe = re.compile(
            r"module\s+(\w+)\s*(?:#\s*\([^;]*?\))?\s*\((.*?)\)\s*;",
            re.S,
        )
    match = modRe.search(text)
    if not match:
        raise ValueError(f"module not found in {verilogPath}" + (f" (top={topName})" if topName else ""))

    if topName:
        portBlock = match.group(1)
        moduleBodyStart = match.end()
        resolvedTop = topName
    else:
        resolvedTop = match.group(1)
        portBlock = match.group(2)
        moduleBodyStart = match.end()

    # find endmodule for body decls (non-ansi)
    endMatch = re.search(r"\bendmodule\b", text[moduleBodyStart:])
    body = text[moduleBodyStart: moduleBodyStart + endMatch.start()] if endMatch else text[moduleBodyStart:]

    ports: List[PortDecl] = []
    # ansi-style: input [3:0] a, output wire b
    ansiItems = [p.strip() for p in portBlock.split(",") if p.strip()]
    looksAnsi = any(re.match(r"^(input|output|inout)\b", item, re.I) for item in ansiItems)

    if looksAnsi:
        currentDir = "input"
        currentWidth = 1
        for item in ansiItems:
            dirMatch = re.match(
                r"^(input|output|inout)\b(?:\s+(?:wire|reg|logic))?\s*(?:(\[[^\]]+\])\s*)?(\w+)\s*$",
                item,
                re.I,
            )
            if dirMatch:
                currentDir = dirMatch.group(1).lower()
                currentWidth = _parseWidth(dirMatch.group(2) or "")
                ports.append(PortDecl(dirMatch.group(3), currentDir, currentWidth))
                continue
            # continuation: just a name, maybe with width
            nameMatch = re.match(r"(?:(\[[^\]]+\])\s*)?(\w+)\s*$", item)
            if nameMatch:
                w = _parseWidth(nameMatch.group(1) or "") if nameMatch.group(1) else currentWidth
                ports.append(PortDecl(nameMatch.group(2), currentDir, w))
        return ports

    # non-ansi: port list is names only; directions in body
    names = []
    for item in ansiItems:
        # drop attributes / escapes
        item = re.sub(r"/\*.*?\*/", "", item)
        tok = item.strip().lstrip("\\")
        if tok:
            names.append(tok.split()[-1])

    dirMap: dict[str, tuple[str, int]] = {}
    for m in re.finditer(
        r"\b(input|output|inout)\b(?:\s+(?:wire|reg|logic))?\s*(\[[^\]]+\])?\s*([^;]+);",
        body,
        re.I,
    ):
        direction = m.group(1).lower()
        width = _parseWidth(m.group(2) or "")
        for name in re.findall(r"\b([A-Za-z_]\w*)\b", m.group(3)):
            dirMap[name] = (direction, width)

    for name in names:
        if name in dirMap:
            direction, width = dirMap[name]
            ports.append(PortDecl(name, direction, width))
        else:
            # default input if undeclared (rare)
            ports.append(PortDecl(name, "input", 1))
    if not ports:
        raise ValueError(f"no ports parsed from {verilogPath} module {resolvedTop}")
    return ports


def parseBlifTopPorts(blifPath: Path) -> tuple[str, List[PortDecl]]:
    """return (modelName, ports) from the first non-blackbox .model."""
    text = blifPath.read_text(encoding="utf-8", errors="replace")
    # join continuations
    lines: list[str] = []
    buf = ""
    for raw in text.splitlines():
        if raw.rstrip().endswith("\\"):
            buf += raw.rstrip()[:-1] + " "
        else:
            buf += raw
            lines.append(buf)
            buf = ""
    if buf:
        lines.append(buf)

    modelName = ""
    inputs: list[str] = []
    outputs: list[str] = []
    inModel = False
    isBlackbox = False
    for line in lines:
        s = line.strip()
        if s.startswith(".model "):
            if inModel and not isBlackbox and (inputs or outputs):
                break
            modelName = s.split(None, 1)[1].strip()
            inputs, outputs = [], []
            inModel = True
            isBlackbox = False
            continue
        if not inModel:
            continue
        if s.startswith(".blackbox"):
            isBlackbox = True
        elif s.startswith(".inputs"):
            inputs.extend(s.split()[1:])
        elif s.startswith(".outputs"):
            outputs.extend(s.split()[1:])
        elif s.startswith(".end"):
            if not isBlackbox and (inputs or outputs):
                break
            inModel = False

    if not modelName:
        raise ValueError(f"no .model found in {blifPath}")

    # blif bus bits like a[0] --> collapse to bus a with width
    def collapse(names: list[str], direction: str) -> List[PortDecl]:
        buses: dict[str, list[int]] = {}
        scalars: list[str] = []
        bitRe = re.compile(r"^(.+)\[(\d+)\]$")
        for n in names:
            m = bitRe.match(n)
            if m:
                buses.setdefault(m.group(1), []).append(int(m.group(2)))
            else:
                scalars.append(n)
        ports: List[PortDecl] = []
        for base, bits in buses.items():
            ports.append(PortDecl(base, direction, max(bits) - min(bits) + 1))
        for n in scalars:
            ports.append(PortDecl(n, direction, 1))
        return ports

    # prefer keeping bit-blasted names as individual 1-bit ports, matches
    # yosys write_verilog from blif which usually keeps a[0] style as separate
    # nets OR as buses. for tb driving we use the rtl port list as source of
    # truth; blif parse is only a fallback.
    ports = [PortDecl(n, "input", 1) for n in inputs] + [
        PortDecl(n, "output", 1) for n in outputs
    ]
    return modelName, ports


def findTopModuleName(
    verilogPath: Path,
    preferredName: Optional[str] = None,
    preferredPorts: Optional[List[str]] = None,
) -> str:
    """pick the design top module from a (possibly multi-module) rtl file.

    preference order:
      1. preferredName if that module exists (synth top / known name)
      2. module matching the file stem
      3. module whose port names best overlap preferredPorts
      4. last module in the file
    """
    text = _stripVerilogComments(
        verilogPath.read_text(encoding="utf-8", errors="replace")
    )
    names = re.findall(r"^\s*module\s+(\w+)\b", text, flags=re.M)
    if not names:
        raise ValueError(f"no module in {verilogPath}")
    if preferredName and preferredName in names:
        return preferredName
    stem = verilogPath.stem
    if stem in names:
        return stem

    if preferredPorts:
        want = set(preferredPorts)
        bestName = None
        bestScore = -1
        for name in names:
            try:
                ports = parseVerilogTopPorts(verilogPath, topName=name)
            except ValueError:
                continue
            score = len(want.intersection({p.name for p in ports}))
            if score > bestScore:
                bestScore = score
                bestName = name
        if bestName is not None and bestScore > 0:
            return bestName

    return names[-1]
