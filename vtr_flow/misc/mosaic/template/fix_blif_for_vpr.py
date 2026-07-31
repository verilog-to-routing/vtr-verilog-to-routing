#!/usr/bin/env python3
"""post-process mosaic blif so vpr and abc behave like vanilla.

1. ram unused addr pads gnd to unconn
   vpr treats a pin tied to const0 as connected which forces every 1-bit
   ram slice to claim all addr pins and blocks wide-mode packing. unconn
   pins are open so sibling slices share the live addr nets. only ram
   addr addr1 addr2 pins are rewritten. never touch .names inputs or the
   sparse-pin crash comes back.

2. hierarchical net dots to ~
   matches the parmys naming style. cosmetic but keeps abc happier.

3. latch-Q and blackbox-input CI/CO uniquify (--vtr-abc)
   a latch Q that also feeds a blackbox input or PO makes abc create a
   CI/CO pair sharing one name and Abc_NtkCheck then fails. rename the
   latch Q to <q>$lq and buffer <q>$lq to <q> so consumers keep the old
   name while the latch CI is unique.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


ramSubcktRe = re.compile(r"^\.subckt\s+(?:single_port_ram|dual_port_ram)\b")
latchSubcktRe = re.compile(r"^\.subckt\s+latch_")
addrGndRe = re.compile(r"\b((?:addr|addr1|addr2)\[\d+\]=)gnd\b")
# hierarchical separator inside identifiers  word.word not directive dots
hierDotRe = re.compile(r"(?<=[\w\]])(\.)(?=[\w$])")
latchQSuffix = "$lq"


def joinContinuedLines(text: str) -> list[str]:
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


def fixRamAddrPads(line: str) -> tuple[str, int]:
    if not ramSubcktRe.match(line):
        return line, 0
    nHits = len(addrGndRe.findall(line))
    return addrGndRe.sub(r"\1unconn", line), nHits


def fixHierDots(line: str) -> tuple[str, int]:
    nHits = len(hierDotRe.findall(line))
    return hierDotRe.sub("~", line), nHits


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


def collectBlackboxInputsAndPos(lines: list[str]) -> set[str]:
    """nets that become abc COs: blackbox input pins + primary outputs."""
    nets: set[str] = set()
    for line in lines:
        s = line.strip()
        if s.startswith(".outputs"):
            nets.update(s.split()[1:])
        elif s.startswith(".subckt"):
            # every pin except O= is an input to the blackbox instance
            for tok in s.split()[2:]:
                if "=" not in tok:
                    continue
                pin, net = tok.split("=", 1)
                if pin != "O":
                    nets.add(net)
    return nets


def uniquifyLatchQCollisions(lines: list[str]) -> tuple[list[str], int]:
    """rename latch Q that also feed blackbox inputs / POs; insert buffers."""
    latchQs = collectLatchQs(lines)
    collisionNets = latchQs & collectBlackboxInputsAndPos(lines)
    if not collisionNets:
        return lines, 0

    # avoid clobbering an existing net name
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
                # buffer so consumers keep the original net name
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


def fixBlifText(text: str) -> tuple[str, dict]:
    stats = {
        "ramAddrGndToUnconn": 0,
        "hierDots": 0,
        "latchQUniquified": 0,
    }
    # work on logical lines so latch/subckt rewrites see full statements
    logical = joinContinuedLines(text)
    outLogical: list[str] = []
    for line in logical:
        line, nAddr = fixRamAddrPads(line)
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


def fixBlifFile(blifPath: Path) -> dict:
    text = blifPath.read_text(encoding="utf-8", errors="replace")
    out, stats = fixBlifText(text)
    blifPath.write_text(out, encoding="utf-8")
    return stats


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("blif", type=Path, help="blif file to rewrite in place")
    args = parser.parse_args(argv)
    if not args.blif.is_file():
        print(f"error: missing blif {args.blif}", file=sys.stderr)
        return 1
    stats = fixBlifFile(args.blif)
    print(
        f"fix_blif_for_vpr: {args.blif} "
        f"ramAddrGndToUnconn={stats['ramAddrGndToUnconn']} "
        f"hierDots={stats['hierDots']} "
        f"latchQUniquified={stats['latchQUniquified']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
