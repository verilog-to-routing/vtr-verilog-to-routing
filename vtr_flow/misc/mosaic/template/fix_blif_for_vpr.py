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


latchSubcktRe = re.compile(r"^\.subckt\s+latch_")
addrGndRe = re.compile(r"\b((?:addr|addr1|addr2)\[\d+\]=)gnd\b")
# hierarchical separator inside identifiers (word.word, not directive dots).
hierDotRe = re.compile(r"(?<=[\w\]])(\.)(?=[\w$])")
latchQSuffix = "$lq"


# USE: build a .subckt matcher for the (possibly aliased) ram model names.
def makeRamSubcktRe(spRamModel: str, dpRamModel: str) -> re.Pattern[str]:
    names = sorted({spRamModel, dpRamModel})
    escaped = "|".join(re.escape(name) for name in names)
    return re.compile(r"^\.subckt\s+(?:" + escaped + r")\b")


# USE: join blif '\\' continuations into logical lines.
def joinContinuedLines(text: str) -> list[str]:
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


# USE: rewrite ram addr pads from gnd to unconn on matching .subckt lines.
def fixRamAddrPads(line: str, ramSubcktRe: re.Pattern[str]) -> tuple[str, int]:
    if not ramSubcktRe.match(line):
        return line, 0
    nHits = len(addrGndRe.findall(line))
    return addrGndRe.sub(r"\1unconn", line), nHits


# USE: replace hierarchical dots inside identifiers with ~.
def fixHierDots(line: str) -> tuple[str, int]:
    nHits = len(hierDotRe.findall(line))
    return hierDotRe.sub("~", line), nHits


# HELPER: collect latch Q net names from .latch and latch_ .subckt lines.
def collectLatchQs(lines: list[str]) -> set[str]:
    latchQs: set[str] = set()
    for line in lines:
        s = line.strip()
        if s.startswith(".latch"):
            parts = s.split()
            if len(parts) >= 3:
                latchQs.add(parts[2])
        elif latchSubcktRe.match(s):
            for tok in s.split()[2:]:
                if tok.startswith("O="):
                    latchQs.add(tok[2:])
    return latchQs


# HELPER: nets that become abc COs (blackbox input pins plus primary outputs).
def collectBlackboxInputsAndPos(lines: list[str]) -> set[str]:
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


# USE: rename latch Q nets that collide with blackbox inputs or POs.
# inserts buffers so consumers keep the original net name.
def uniquifyLatchQCollisions(lines: list[str]) -> tuple[list[str], int]:
    latchQs = collectLatchQs(lines)
    collisionNets = latchQs & collectBlackboxInputsAndPos(lines)
    if not collisionNets:
        return lines, 0

    # avoid clobbering an existing net name when choosing the uniquified latch Q.
    allNets: set[str] = set()
    tokenRe = re.compile(r"[^\s=]+")
    for line in lines:
        allNets.update(tokenRe.findall(line))

    renameMap: dict[str, str] = {}
    for qName in collisionNets:
        newName = qName + latchQSuffix
        n = 0
        while newName in allNets or newName in renameMap.values():
            n += 1
            newName = f"{qName}{latchQSuffix}{n}"
        renameMap[qName] = newName
        allNets.add(newName)

    outLines: list[str] = []
    nRenamed = 0
    for line in lines:
        s = line.strip()
        if s.startswith(".latch"):
            parts = s.split()
            if len(parts) >= 3 and parts[2] in renameMap:
                oldQ = parts[2]
                newQ = renameMap[oldQ]
                parts[2] = newQ
                outLines.append(" ".join(parts))
                # buffer the renamed latch Q so consumers keep the original net name.
                outLines.append(f".names {newQ} {oldQ}")
                outLines.append("1 1")
                nRenamed += 1
                continue
        elif latchSubcktRe.match(s):
            toks = s.split()
            changed = False
            for i, tok in enumerate(toks):
                if tok.startswith("O="):
                    oldQ = tok[2:]
                    if oldQ in renameMap:
                        toks[i] = f"O={renameMap[oldQ]}"
                        changed = True
                        oldForBuf = oldQ
            if changed:
                outLines.append(" ".join(toks))
                outLines.append(f".names {renameMap[oldForBuf]} {oldForBuf}")
                outLines.append("1 1")
                nRenamed += 1
                continue
        outLines.append(line)
    return outLines, nRenamed


# USE: apply all blif hygiene fixes and return rewritten text plus stats.
def fixBlifText(
    text: str,
    spRamModel: str = "single_port_ram",
    dpRamModel: str = "dual_port_ram",
) -> tuple[str, dict]:
    stats = {
        "ramAddrGndToUnconn": 0,
        "hierDots": 0,
        "latchQUniquified": 0,
    }
    ramSubcktRe = makeRamSubcktRe(spRamModel, dpRamModel)
    # work on logical lines so latch/subckt rewrites see full statements.
    logical = joinContinuedLines(text)
    outLogical: list[str] = []
    for line in logical:
        line, nAddr = fixRamAddrPads(line, ramSubcktRe)
        stats["ramAddrGndToUnconn"] += nAddr
        line, nDots = fixHierDots(line)
        stats["hierDots"] += nDots
        outLogical.append(line)

    outLogical, nUniq = uniquifyLatchQCollisions(outLogical)
    stats["latchQUniquified"] = nUniq

    out = "\n".join(outLogical)
    if text.endswith("\n") or out:
        out += "\n"
    return out, stats


# USE: rewrite a blif file in place.
def fixBlifFile(
    blifPath: Path,
    spRamModel: str = "single_port_ram",
    dpRamModel: str = "dual_port_ram",
) -> dict:
    text = blifPath.read_text(encoding="utf-8", errors="replace")
    out, stats = fixBlifText(text, spRamModel=spRamModel, dpRamModel=dpRamModel)
    blifPath.write_text(out, encoding="utf-8")
    return stats


# USE: parse spRamModel/dpRamModel from arch_facts.tcl with classic defaults.
def readRamModelsFromArchFacts(archFactsPath: Path) -> tuple[str, str]:
    spRamModel = "single_port_ram"
    dpRamModel = "dual_port_ram"
    if not archFactsPath.is_file():
        return spRamModel, dpRamModel
    for line in archFactsPath.read_text(encoding="utf-8", errors="replace").splitlines():
        stripped = line.strip()
        if stripped.startswith("set spRamModel "):
            spRamModel = stripped.split(None, 2)[2].strip().strip('"')
        elif stripped.startswith("set dpRamModel "):
            dpRamModel = stripped.split(None, 2)[2].strip().strip('"')
    return spRamModel, dpRamModel


def main(argv=None) -> int:
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
    spRamModel = args.sp_ram
    dpRamModel = args.dp_ram
    if spRamModel is None or dpRamModel is None:
        factsSp, factsDp = readRamModelsFromArchFacts(
            args.arch_facts
            if args.arch_facts is not None
            else args.blif.parent / "arch_facts.tcl"
        )
        if spRamModel is None:
            spRamModel = factsSp
        if dpRamModel is None:
            dpRamModel = factsDp
    stats = fixBlifFile(args.blif, spRamModel=spRamModel, dpRamModel=dpRamModel)
    print(
        f"fix_blif_for_vpr: {args.blif} "
        f"sp={spRamModel} dp={dpRamModel} "
        f"ramAddrGndToUnconn={stats['ramAddrGndToUnconn']} "
        f"hierDots={stats['hierDots']} "
        f"latchQUniquified={stats['latchQUniquified']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
