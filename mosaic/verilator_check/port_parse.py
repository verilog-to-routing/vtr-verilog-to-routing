#!/usr/bin/env python3
"""parse top-module ports from rtl verilog or blif for the random-check tb."""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional


@dataclass(frozen=True)
class PortDecl:
    """immutable record for a single port declaration (name, direction, width)."""

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


def is_clock_port(name: str) -> bool:
    """return true if name looks like a clock signal."""
    lower = name.lower()
    if lower in _CLOCK_NAMES:
        return True
    return lower.endswith("_clk") or lower.startswith("clk_")


def is_reset_port(name: str) -> bool:
    """return true if name looks like a reset signal."""
    lower = name.lower()
    if lower in _RESET_NAMES:
        return True
    return "reset" in lower or lower.endswith("_rst") or lower.startswith("rst_")


def _parse_width(token_block: str) -> int:
    """extract bit width from a verilog range token like [7:0]."""
    m = re.search(r"\[\s*(\d+)\s*:\s*(\d+)\s*\]", token_block)
    if not m:
        return 1
    msb, lsb = int(m.group(1)), int(m.group(2))
    return abs(msb - lsb) + 1


# pack abc/yosys bit-blasted names (a~3 or a[3]) into one bus port
def collapse_bit_blasted_ports(ports: List[PortDecl]) -> List[PortDecl]:
    """merge bit-blasted scalar ports (a~3, a[3]) into a single bus port."""
    bit_re = re.compile(r"^(.+)(?:~|\[)(\d+)\]?$")
    buses: dict[str, tuple[str, list[int]]] = {}
    packed: List[PortDecl] = []
    seen = set()
    for port in ports:
        m = bit_re.match(port.name)
        if not m:
            packed.append(port)
            continue
        base = m.group(1)
        idx = int(m.group(2))
        if base not in buses:
            buses[base] = (port.direction, [idx])
        else:
            buses[base][1].append(idx)
        if base in seen:
            continue
        seen.add(base)
        packed.append(port)
    out: List[PortDecl] = []
    for port in packed:
        m = bit_re.match(port.name)
        if not m:
            out.append(port)
            continue
        base = m.group(1)
        direction, idxs = buses[base]
        out.append(PortDecl(base, direction, max(idxs) - min(idxs) + 1))
    return out


# HELPER: remove // and /* */ comments without treating //*** headers as /*.
def _strip_verilog_comments(text: str) -> str:
    """remove line and block comments from verilog source text."""
    # line comments first. vtr headers like //***** contain the substring /*
    text = re.sub(r"//.*?$", "", text, flags=re.M)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return text


# USE: parse ansi or non-ansi ports from the first or named module.
def parse_verilog_top_ports(  # pylint: disable=too-many-locals,too-many-branches,too-many-statements
    verilog_path: Path,
    top_name: Optional[str] = None,
) -> List[PortDecl]:
    """parse port declarations from the top module in a verilog file."""
    text = _strip_verilog_comments(
        verilog_path.read_text(encoding="utf-8", errors="replace")
    )

    if top_name:
        mod_re = re.compile(
            rf"module\s+{re.escape(top_name)}"
            r"\s*(?:#\s*\([^;]*?\))?\s*\((.*?)\)\s*;",
            re.S,
        )
    else:
        mod_re = re.compile(
            r"module\s+(\w+)"
            r"\s*(?:#\s*\([^;]*?\))?\s*\((.*?)\)\s*;",
            re.S,
        )
    match = mod_re.search(text)
    if not match:
        msg = f"module not found in {verilog_path}"
        if top_name:
            msg += f" (top={top_name})"
        raise ValueError(msg)

    if top_name:
        port_block = match.group(1)
        module_body_start = match.end()
        resolved_top = top_name
    else:
        resolved_top = match.group(1)
        port_block = match.group(2)
        module_body_start = match.end()

    # find endmodule for body decls (non-ansi)
    end_match = re.search(r"\bendmodule\b", text[module_body_start:])
    body = (
        text[module_body_start: module_body_start + end_match.start()]
        if end_match
        else text[module_body_start:]
    )

    ports: List[PortDecl] = []
    # ansi-style. input [3:0] a, output wire b
    ansi_items = [
        p.strip() for p in port_block.split(",") if p.strip()
    ]
    looks_ansi = any(
        re.match(r"^(input|output|inout)\b", item, re.I)
        for item in ansi_items
    )

    if looks_ansi:
        current_dir = "input"
        current_width = 1
        for item in ansi_items:
            dir_match = re.match(
                r"^(input|output|inout)\b"
                r"(?:\s+(?:wire|reg|logic))?"
                r"\s*(?:(\[[^\]]+\])\s*)?(\w+)\s*$",
                item,
                re.I,
            )
            if dir_match:
                current_dir = dir_match.group(1).lower()
                current_width = _parse_width(
                    dir_match.group(2) or ""
                )
                ports.append(PortDecl(
                    dir_match.group(3), current_dir, current_width
                ))
                continue
            # continuation is just a name, maybe with width
            name_match = re.match(
                r"(?:(\[[^\]]+\])\s*)?(\w+)\s*$", item
            )
            if name_match:
                w = (
                    _parse_width(name_match.group(1) or "")
                    if name_match.group(1)
                    else current_width
                )
                ports.append(PortDecl(
                    name_match.group(2), current_dir, w
                ))
        return collapse_bit_blasted_ports(ports)

    # non-ansi. port list is names only. directions live in the body
    names = []
    for item in ansi_items:
        # drop attributes / escapes
        item = re.sub(r"/\*.*?\*/", "", item)
        tok = item.strip().lstrip("\\")
        if tok:
            names.append(tok.split()[-1])

    dir_map: dict[str, tuple[str, int]] = {}
    for m in re.finditer(
        r"\b(input|output|inout)\b"
        r"(?:\s+(?:wire|reg|logic))?"
        r"\s*(\[[^\]]+\])?\s*([^;]+);",
        body,
        re.I,
    ):
        direction = m.group(1).lower()
        width = _parse_width(m.group(2) or "")
        for name in re.findall(r"\b([A-Za-z_]\w*)\b", m.group(3)):
            dir_map[name] = (direction, width)

    for name in names:
        if name in dir_map:
            direction, width = dir_map[name]
            ports.append(PortDecl(name, direction, width))
        else:
            # default input if undeclared (rare)
            ports.append(PortDecl(name, "input", 1))
    if not ports:
        raise ValueError(
            f"no ports parsed from {verilog_path} module {resolved_top}"
        )
    return collapse_bit_blasted_ports(ports)


# USE: return (model_name, ports) from the first non-blackbox .model.
def parse_blif_top_ports(  # pylint: disable=too-many-locals,too-many-branches,too-many-statements
    blif_path: Path,
) -> tuple[str, List[PortDecl]]:
    """parse the first non-blackbox .model from a blif file."""
    text = blif_path.read_text(encoding="utf-8", errors="replace")
    # join continuations into logical lines
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

    model_name = ""
    inputs: list[str] = []
    outputs: list[str] = []
    in_model = False
    is_blackbox = False
    for line in lines:
        s = line.strip()
        if s.startswith(".model "):
            if in_model and not is_blackbox and (inputs or outputs):
                break
            model_name = s.split(None, 1)[1].strip()
            inputs, outputs = [], []
            in_model = True
            is_blackbox = False
            continue
        if not in_model:
            continue
        if s.startswith(".blackbox"):
            is_blackbox = True
        elif s.startswith(".inputs"):
            inputs.extend(s.split()[1:])
        elif s.startswith(".outputs"):
            outputs.extend(s.split()[1:])
        elif s.startswith(".end"):
            if not is_blackbox and (inputs or outputs):
                break
            in_model = False

    if not model_name:
        raise ValueError(f"no .model found in {blif_path}")

    # blif bus bits like a[0] can collapse to bus a with width
    def _collapse(names: list[str], direction: str) -> List[PortDecl]:
        """collapse bit-blasted blif names into bus ports."""
        buses: dict[str, list[int]] = {}
        scalars: list[str] = []
        bit_re = re.compile(r"^(.+)(?:\[(\d+)\]|~(\d+))$")
        for n in names:
            m = bit_re.match(n)
            if m:
                buses.setdefault(m.group(1), []).append(
                    int(m.group(2) or m.group(3))
                )
            else:
                scalars.append(n)
        ports: List[PortDecl] = []
        for base, bits in buses.items():
            ports.append(PortDecl(
                base, direction, max(bits) - min(bits) + 1
            ))
        for n in scalars:
            ports.append(PortDecl(n, direction, 1))
        return ports

    # prefer keeping bit-blasted names as individual 1-bit ports. that matches
    # yosys write_verilog from blif which usually keeps a[0] style as separate
    # nets or as buses. for tb driving we use the rtl port list as source of
    # truth. blif parse is only a fallback.
    ports = [PortDecl(n, "input", 1) for n in inputs] + [
        PortDecl(n, "output", 1) for n in outputs
    ]
    return model_name, collapse_bit_blasted_ports(ports)



# USE: pick the design top module from a (possibly multi-module) rtl file.
# preference order:
#   1. preferred_name if that module exists (synth top / known name)
#   2. module matching the file stem
#   3. module whose port names best overlap preferred_ports
#   4. last module in the file
def find_top_module_name(
    verilog_path: Path,
    preferred_name: Optional[str] = None,
    preferred_ports: Optional[List[str]] = None,
) -> str:
    """pick the design top module from a multi-module rtl file."""
    text = _strip_verilog_comments(
        verilog_path.read_text(encoding="utf-8", errors="replace")
    )
    names = re.findall(r"^\s*module\s+(\w+)\b", text, flags=re.M)
    if not names:
        raise ValueError(f"no module in {verilog_path}")
    if preferred_name and preferred_name in names:
        return preferred_name
    stem = verilog_path.stem
    if stem in names:
        return stem

    if preferred_ports:
        want = set(preferred_ports)
        best_name = None
        best_score = -1
        for name in names:
            try:
                ports = parse_verilog_top_ports(
                    verilog_path, top_name=name
                )
            except ValueError:
                continue
            score = len(want.intersection({p.name for p in ports}))
            if score > best_score:
                best_score = score
                best_name = name
        if best_name is not None and best_score > 0:
            return best_name

    return names[-1]
