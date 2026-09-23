#!/usr/bin/env python3
"""convert a BLIF netlist to verilog via yosys for verilator elaboration."""

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


def resolve_yosys(yosys_path: Path | None = None) -> Path:
    """locate a yosys binary for BLIF to verilog conversion."""
    if yosys_path is not None and yosys_path.is_file():
        return yosys_path
    repo_root = Path(__file__).resolve().parents[2]
    candidates = [
        repo_root / "build" / "bin" / "yosys",
        repo_root / "yosys" / "yosys",
    ]
    for path in candidates:
        if path.is_file():
            return path
    which = subprocess.run(["which", "yosys"], capture_output=True, text=True, check=False)
    if which.returncode == 0 and which.stdout.strip():
        return Path(which.stdout.strip())
    raise FileNotFoundError("yosys not found (pass --yosys or build vtr)")


def strip_hardblock_module_defs(verilog_text: str) -> str:
    """remove empty/blackbox module bodies that conflict with sim_hardblocks.v."""
    hardblocks = {
        "adder", "multiply", "single_port_ram", "dual_port_ram",
        "mux", "fpga_interconnect", "dff", "dffl", "dffe", "latch",
    }
    pattern = re.compile(
        r"^\s*module\s+(\w+)\b.*?^\s*endmodule\s*",
        re.M | re.S,
    )

    def keep_or_drop(match: re.Match) -> str:
        name = match.group(1)
        if name in hardblocks:
            return f"// stripped blackbox module {name} (provided by sim_hardblocks.v)\n"
        return match.group(0)

    return pattern.sub(keep_or_drop, verilog_text)


def _split_port_connections(body: str) -> List[str]:
    """split '.port(net),' list on top-level commas."""
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


def _normalize_net_expr(net: str) -> str:
    """trim net expr but keep trailing space after escaped identifiers.

    verilator requires a space after \\escaped ids before ',' ')' or '}'.
    strip() removes that space and breaks compile.
    """
    net = net.strip()
    if net.startswith("\\") and not net.endswith(" "):
        net = net + " "
    return net


def pack_bit_blasted_hardblock_ports(verilog_text: str) -> str:  # pylint: disable=too-many-locals
    """rewrite .\\a[0](n0), .\\a[1](n1) into .a({n1, n0}) for sim_hardblocks vector ports.

    yosys write_verilog keeps BLIF-style bitblasted pin names after read_blif of
    blackbox .model definitions. verilator then fails with PINNOTFOUND against
    the vector ports in sim_hardblocks.v. packing here is the fix.
    instances with no bit-blasted pins are left unchanged so we do not strip the
    trailing spaces yosys already emitted on escaped net names.
    """
    bit_pin_re = re.compile(
        r"^\.\\(?P<port>[A-Za-z_]\w*)(?:\[(?P<br>\d+)\]|~(?P<tilde>\d+))\s*\((?P<net>.*)\)$"
    )
    tilde_pin_re = re.compile(
        r"^\.(?P<port>[A-Za-z_]\w*)~(?P<tilde>\d+)\s*\((?P<net>.*)\)$"
    )
    plain_pin_re = re.compile(r"^\.(?P<port>[A-Za-z_]\w*)\s*\((?P<net>.*)\)$")
    inst_pattern = re.compile(
        r"(?P<indent>^[ \t]*)(?P<cell>adder|multiply|single_port_ram|dual_port_ram)"
        r"(?P<gap>\s+)(?P<inst>\S+)\s*\((?P<body>.*?)\);",
        re.M | re.S,
    )

    def rewrite_instance(match: re.Match) -> str:  # pylint: disable=too-many-branches,too-many-locals
        cell = match.group("cell")
        vector_ports = set(HARDBLOCK_VECTOR_PORTS.get(cell, ()))
        conns = _split_port_connections(match.group("body"))

        blasted: Dict[str, Dict[int, str]] = {}
        plain: List[Tuple[str, str]] = []
        for conn in conns:
            conn = conn.strip().rstrip(",")
            m_bit = bit_pin_re.match(conn) or tilde_pin_re.match(conn)
            if m_bit and m_bit.group("port") in vector_ports:
                port = m_bit.group("port")
                idx = int(m_bit.group("br") or m_bit.group("tilde"))
                net = _normalize_net_expr(m_bit.group("net"))
                blasted.setdefault(port, {})[idx] = net
                continue
            m_plain = plain_pin_re.match(conn)
            if m_plain:
                plain.append(
                    (m_plain.group("port"), _normalize_net_expr(m_plain.group("net")))
                )
                continue
            plain.append((f"_raw_{len(plain)}", conn))

        if not blasted:
            return match.group(0)

        new_conns: List[str] = []
        for port, bits in blasted.items():
            max_idx = max(bits)
            ordered = [bits.get(i, "1'b0") for i in range(max_idx + 1)]
            concat = "{" + ", ".join(reversed(ordered)) + "}"
            new_conns.append(f".{port}({concat})")
        for port, net in plain:
            if port.startswith("_raw_"):
                new_conns.append(net)
            else:
                new_conns.append(f".{port}({net})")

        indent = match.group("indent")
        inner = ",\n".join(f"{indent}    {c}" for c in new_conns)
        return (
            f"{indent}{cell}{match.group('gap')}{match.group('inst')} (\n"
            f"{inner}\n{indent});"
        )

    return inst_pattern.sub(rewrite_instance, verilog_text)


def pack_bit_blasted_top_ports(verilog_text: str) -> str:  # pylint: disable=too-many-locals,too-many-branches
    """pack abc ~bit and yosys [bit] top ports into RTL-style vectors."""
    mod_match = re.search(
        r"(?P<head>^\s*module\s+(?P<name>\w+)\s*\()(?P<ports>.*?)(?P<tail>\)\s*;)",
        verilog_text,
        re.M | re.S,
    )
    if not mod_match:
        return verilog_text

    raw_ports = [p.strip() for p in mod_match.group("ports").split(",") if p.strip()]
    bit_port_re = re.compile(
        r"^\\?(?P<base>[A-Za-z_][A-Za-z0-9_]*)(?:\[(?P<br>\d+)\]|~(?P<tilde>\d+))\s*$"
    )

    vector_bits: Dict[str, List[int]] = {}
    for port in raw_ports:
        m = bit_port_re.match(port)
        if m:
            idx = int(m.group("br") or m.group("tilde"))
            vector_bits.setdefault(m.group("base"), []).append(idx)

    if not vector_bits:
        return verilog_text

    new_port_list: List[str] = []
    seen_vectors = set()
    for port in raw_ports:
        m = bit_port_re.match(port)
        if not m:
            if port.startswith("\\") and "[" not in port and "~" not in port:
                new_port_list.append(port.lstrip("\\").strip())
            else:
                new_port_list.append(port)
            continue
        base = m.group("base")
        if base not in seen_vectors:
            new_port_list.append(base)
            seen_vectors.add(base)

    new_header = (
        f"{mod_match.group('head')}{', '.join(new_port_list)}{mod_match.group('tail')}"
    )
    text = verilog_text[: mod_match.start()] + new_header + verilog_text[mod_match.end() :]

    dir_re = re.compile(
        r"^\s*(?P<dir>input|output|inout)(?:\s+wire|\s+reg)?\s+"
        r"\\?(?P<base>[A-Za-z_][A-Za-z0-9_]*)(?:\[(?P<br>\d+)\]|~(?P<tilde>\d+))\s*;\s*$",
        re.M,
    )
    dir_by_base: Dict[str, str] = {}
    for m in dir_re.finditer(text):
        base = m.group("base")
        dir_by_base.setdefault(base, m.group("dir"))

    text = dir_re.sub("", text)

    decl_lines = []
    for base, idxs in vector_bits.items():
        direction = dir_by_base.get(base, "input")
        msb = max(idxs)
        decl_lines.append(f"  {direction} [{msb}:0] {base};")
    if decl_lines:
        text = re.sub(
            r"(module\s+\w+\s*\([^;]*\)\s*;)",
            r"\1\n" + "\n".join(decl_lines),
            text,
            count=1,
            flags=re.S,
        )

    for base in vector_bits:
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


def declare_const_nets(verilog_text: str) -> str:
    """ensure unconn/gnd/vcc wires exist when nets reference them."""
    needs = []
    if re.search(r"\bunconn\b", verilog_text) and not re.search(
        r"^\s*wire\s+unconn\s*;", verilog_text, re.M
    ):
        needs.append("  wire unconn = 1'b0;")
    if re.search(r"\bgnd\b", verilog_text) and not re.search(
        r"^\s*wire\s+gnd\s*;", verilog_text, re.M
    ):
        needs.append("  wire gnd = 1'b0;")
    if re.search(r"\bvcc\b", verilog_text) and not re.search(
        r"^\s*wire\s+vcc\s*;", verilog_text, re.M
    ):
        needs.append("  wire vcc = 1'b1;")
    if not needs:
        return verilog_text
    return re.sub(
        r"(module\s+\w+\b[^;]*;)",
        r"\1\n" + "\n".join(needs),
        verilog_text,
        count=1,
        flags=re.S,
    )


def blif_to_verilog(
    blif_path: Path,
    out_verilog: Path,
    *,
    top_name: str | None = None,
    yosys_path: Path | None = None,
    log_path: Path | None = None,
) -> None:
    """read_blif, optional hierarchy -top, then write_verilog -noattr."""
    yosys = resolve_yosys(yosys_path)
    out_verilog.parent.mkdir(parents=True, exist_ok=True)
    # -wideports packs top-level a[0],a[1] into buses when possible. hardblock
    # blackbox pins often still come out bit-blasted and need python packing.
    script_lines = [
        f"read_blif -wideports {blif_path.resolve().as_posix()}",
    ]
    if top_name:
        script_lines.append(f"hierarchy -top {top_name}")
    else:
        script_lines.append("hierarchy -auto-top")
    script_lines.append("flatten")
    script_lines.append(f"write_verilog -noattr {out_verilog.resolve().as_posix()}")
    cmd = [str(yosys), "-p", "; ".join(script_lines)]
    result = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if log_path is not None:
        log_path.parent.mkdir(parents=True, exist_ok=True)
        log_path.write_text(result.stdout + "\n" + result.stderr, encoding="utf-8")
    if result.returncode != 0 or not out_verilog.is_file():
        raise RuntimeError(
            f"yosys blif-->verilog failed for {blif_path} "
            f"(rc={result.returncode}); see {log_path}"
        )
    cleaned = strip_hardblock_module_defs(
        out_verilog.read_text(encoding="utf-8", errors="replace")
    )
    cleaned = pack_bit_blasted_hardblock_ports(cleaned)
    cleaned = pack_bit_blasted_top_ports(cleaned)
    cleaned = declare_const_nets(cleaned)
    out_verilog.write_text(cleaned, encoding="utf-8")


def main(argv=None) -> int:
    """CLI entry point for BLIF to verilog conversion."""
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
        blif_to_verilog(
            args.blif,
            args.output,
            top_name=args.top,
            yosys_path=args.yosys,
            log_path=args.log,
        )
    except (FileNotFoundError, RuntimeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
