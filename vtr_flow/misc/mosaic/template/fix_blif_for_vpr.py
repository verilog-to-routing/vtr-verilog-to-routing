#!/usr/bin/env python3
"""post-process mosaic blif so vpr and abc behave like vanilla parmys.

1. ram unused addr pads change from gnd to unconn because vpr treats a pin tied
   to const0 as connected, which forces every 1-bit ram slice to claim all addr
   pins and blocks wide-mode packing. unconn pins stay open so sibling slices
   share live addr nets. only ram addr, addr1, and addr2 pins are rewritten
   because touching .names inputs brings back the sparse-pin crash.

2. hierarchical net dots become ~ so naming matches the parmys style, which is
   cosmetic but keeps abc happier.

3. latch-Q and blackbox-input CI/CO uniquify (--vtr-abc) because a latch Q that
   also feeds a blackbox input or PO makes abc create a CI/CO pair sharing one
   name and Abc_NtkCheck then fails. rename the latch Q to <q>$lq and buffer
   <q>$lq to <q> so consumers keep the old name while the latch CI is unique.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

LATCH_SUBCKT_RE = re.compile(r"^\.subckt\s+latch_")
ADDR_GND_RE = re.compile(r"\b((?:addr|addr1|addr2)\[\d+\]=)gnd\b")
# hierarchical separator inside identifiers (word.word, not directive dots).
HIER_DOT_RE = re.compile(r"(?<=[\w\]])(\.)(?=[\w$])")
LATCH_Q_SUFFIX = "$lq"


def make_ram_subckt_re(sp_ram_model: str, dp_ram_model: str) -> re.Pattern[str]:
    """build a .subckt matcher for the (possibly aliased) ram model names."""
    names = sorted({sp_ram_model, dp_ram_model})
    escaped = "|".join(re.escape(name) for name in names)
    return re.compile(r"^\.subckt\s+(?:" + escaped + r")\b")


def join_continued_lines(text: str) -> list[str]:
    """join blif '\\' continuations into logical lines."""
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
    return lines


def fix_ram_addr_pads(line: str, ram_subckt_re: re.Pattern[str]) -> tuple[str, int]:
    """rewrite ram addr pads from gnd to unconn on matching .subckt lines."""
    if not ram_subckt_re.match(line):
        return line, 0
    n_hits = len(ADDR_GND_RE.findall(line))
    return ADDR_GND_RE.sub(r"\1unconn", line), n_hits


def fix_hier_dots(line: str) -> tuple[str, int]:
    """replace hierarchical dots inside identifiers with ~."""
    n_hits = len(HIER_DOT_RE.findall(line))
    return HIER_DOT_RE.sub("~", line), n_hits


def collect_latch_qs(lines: list[str]) -> set[str]:
    """collect latch Q net names from .latch and latch_ .subckt lines."""
    latch_qs: set[str] = set()
    for line in lines:
        s = line.strip()
        if s.startswith(".latch"):
            parts = s.split()
            if len(parts) >= 3:
                latch_qs.add(parts[2])
        elif LATCH_SUBCKT_RE.match(s):
            for tok in s.split()[2:]:
                if tok.startswith("O="):
                    latch_qs.add(tok[2:])
    return latch_qs


def collect_blackbox_inputs_and_pos(lines: list[str]) -> set[str]:
    """nets that become abc COs (blackbox input pins plus primary outputs)."""
    nets: set[str] = set()
    for line in lines:
        s = line.strip()
        if s.startswith(".outputs"):
            nets.update(s.split()[1:])
        elif s.startswith(".subckt"):
            # every pin except O= is an input to the blackbox instance.
            for tok in s.split()[2:]:
                if "=" not in tok:
                    continue
                pin, net = tok.split("=", 1)
                if pin != "O":
                    nets.add(net)
    return nets


# pylint: disable=too-many-locals
def uniquify_latch_q_collisions(lines: list[str]) -> tuple[list[str], int]:
    """rename latch Q nets that collide with blackbox inputs or POs.

    inserts buffers so consumers keep the original net name.
    """
    latch_qs = collect_latch_qs(lines)
    collision_nets = latch_qs & collect_blackbox_inputs_and_pos(lines)
    if not collision_nets:
        return lines, 0

    # avoid clobbering an existing net name when choosing the uniquified latch Q.
    all_nets: set[str] = set()
    token_re = re.compile(r"[^\s=]+")
    for line in lines:
        all_nets.update(token_re.findall(line))

    rename_map: dict[str, str] = {}
    for q_name in collision_nets:
        new_name = q_name + LATCH_Q_SUFFIX
        n = 0
        while new_name in all_nets or new_name in rename_map.values():
            n += 1
            new_name = f"{q_name}{LATCH_Q_SUFFIX}{n}"
        rename_map[q_name] = new_name
        all_nets.add(new_name)

    out_lines: list[str] = []
    n_renamed = 0
    for line in lines:
        s = line.strip()
        if s.startswith(".latch"):
            parts = s.split()
            if len(parts) >= 3 and parts[2] in rename_map:
                old_q = parts[2]
                new_q = rename_map[old_q]
                parts[2] = new_q
                out_lines.append(" ".join(parts))
                # buffer the renamed latch Q so consumers keep the original net name.
                out_lines.append(f".names {new_q} {old_q}")
                out_lines.append("1 1")
                n_renamed += 1
                continue
        elif LATCH_SUBCKT_RE.match(s):
            toks = s.split()
            old_for_buf = None
            for i, tok in enumerate(toks):
                if tok.startswith("O="):
                    old_q = tok[2:]
                    if old_q in rename_map:
                        toks[i] = f"O={rename_map[old_q]}"
                        old_for_buf = old_q
            if old_for_buf is not None:
                out_lines.append(" ".join(toks))
                out_lines.append(f".names {rename_map[old_for_buf]} {old_for_buf}")
                out_lines.append("1 1")
                n_renamed += 1
                continue
        out_lines.append(line)
    return out_lines, n_renamed


def fix_blif_text(
    text: str,
    sp_ram_model: str = "single_port_ram",
    dp_ram_model: str = "dual_port_ram",
) -> tuple[str, dict]:
    """apply all blif hygiene fixes and return rewritten text plus stats."""
    stats = {
        "ram_addr_gnd_to_unconn": 0,
        "hier_dots": 0,
        "latch_q_uniquified": 0,
    }
    ram_subckt_re = make_ram_subckt_re(sp_ram_model, dp_ram_model)
    # work on logical lines so latch/subckt rewrites see full statements.
    logical = join_continued_lines(text)
    out_logical: list[str] = []
    for line in logical:
        line, n_addr = fix_ram_addr_pads(line, ram_subckt_re)
        stats["ram_addr_gnd_to_unconn"] += n_addr
        line, n_dots = fix_hier_dots(line)
        stats["hier_dots"] += n_dots
        out_logical.append(line)

    out_logical, n_uniq = uniquify_latch_q_collisions(out_logical)
    stats["latch_q_uniquified"] = n_uniq

    out = "\n".join(out_logical)
    if text.endswith("\n") or out:
        out += "\n"
    return out, stats


def fix_blif_file(
    blif_path: Path,
    sp_ram_model: str = "single_port_ram",
    dp_ram_model: str = "dual_port_ram",
) -> dict:
    """rewrite a blif file in place."""
    text = blif_path.read_text(encoding="utf-8", errors="replace")
    out, stats = fix_blif_text(text, sp_ram_model=sp_ram_model, dp_ram_model=dp_ram_model)
    blif_path.write_text(out, encoding="utf-8")
    return stats


def read_ram_models_from_arch_facts(arch_facts_path: Path) -> tuple[str, str]:
    """parse sp/dp ram model names from arch_facts.tcl with classic defaults."""
    sp_ram_model = "single_port_ram"
    dp_ram_model = "dual_port_ram"
    if not arch_facts_path.is_file():
        return sp_ram_model, dp_ram_model
    for line in arch_facts_path.read_text(encoding="utf-8", errors="replace").splitlines():
        stripped = line.strip()
        # tcl knobs keep camelCase names from the mosaic arch_facts emitter.
        if stripped.startswith("set spRamModel "):
            sp_ram_model = stripped.split(None, 2)[2].strip().strip('"')
        elif stripped.startswith("set dpRamModel "):
            dp_ram_model = stripped.split(None, 2)[2].strip().strip('"')
    return sp_ram_model, dp_ram_model


def main(argv=None) -> int:
    """cli entry: rewrite a blif in place for vpr/abc hygiene."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("blif", type=Path, help="blif file to rewrite in place")
    parser.add_argument(
        "--sp-ram",
        default=None,
        help="single-port ram model name (default: single_port_ram or arch_facts)",
    )
    parser.add_argument(
        "--dp-ram",
        default=None,
        help="dual-port ram model name (default: dual_port_ram or arch_facts)",
    )
    parser.add_argument(
        "--arch-facts",
        type=Path,
        default=None,
        help="optional arch_facts.tcl with spRamModel/dpRamModel",
    )
    args = parser.parse_args(argv)
    if not args.blif.is_file():
        print(f"error: missing blif {args.blif}", file=sys.stderr)
        return 1
    sp_ram_model = args.sp_ram
    dp_ram_model = args.dp_ram
    if sp_ram_model is None or dp_ram_model is None:
        facts_sp, facts_dp = read_ram_models_from_arch_facts(
            args.arch_facts if args.arch_facts is not None else args.blif.parent / "arch_facts.tcl"
        )
        if sp_ram_model is None:
            sp_ram_model = facts_sp
        if dp_ram_model is None:
            dp_ram_model = facts_dp
    stats = fix_blif_file(args.blif, sp_ram_model=sp_ram_model, dp_ram_model=dp_ram_model)
    print(
        f"fix_blif_for_vpr: {args.blif} "
        f"sp={sp_ram_model} dp={dp_ram_model} "
        f"ram_addr_gnd_to_unconn={stats['ram_addr_gnd_to_unconn']} "
        f"hier_dots={stats['hier_dots']} "
        f"latch_q_uniquified={stats['latch_q_uniquified']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
