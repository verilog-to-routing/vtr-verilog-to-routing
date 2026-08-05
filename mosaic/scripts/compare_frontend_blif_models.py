#!/usr/bin/env python3
"""diff hardblock .subckt model sets between parmys and mosaic blifs.

compares the set of arch-declared hardblock models that appear as `.subckt`
instances. soft-gate structure is ignored.

modes:
  1) compare existing blifs:
       python mosaic/scripts/compare_frontend_blif_models.py \\
         --arch <arch.xml> --parmys-blif a.blif --mosaic-blif b.blif
  2) run both synths then compare (needs built vtr plus mosaic plugin):
       python mosaic/scripts/compare_frontend_blif_models.py --run \\
         --circuit <c.v> --arch <arch.xml>

exit 0 when used hardblock model sets match. 1 on mismatch or tool failure.
"""

from __future__ import print_function

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
MOSAIC_ROOT = SCRIPT_DIR.parent
REPO_ROOT = MOSAIC_ROOT.parent
DEFAULT_CIRCUIT = (
    REPO_ROOT / "vtr_flow/benchmarks/verilog/diffeq1.v"
)
DEFAULT_ARCH = (
    REPO_ROOT / "vtr_flow/arch/timing/k6_frac_N10_frac_chain_mem32K_40nm.xml"
)
RUN_VTR_FLOW = REPO_ROOT / "vtr_flow/scripts/run_vtr_flow.py"

SUBCKT_RE = re.compile(r"^\.subckt\s+(\S+)", re.MULTILINE)


# USE: collect hardblock model names declared in an arch xml.
def archModelNames(archXmlPath):
    text = Path(archXmlPath).read_text(encoding="utf-8", errors="replace")
    # prefer <models> declarations and fall back to pb_type blif_model scan.
    names = set(re.findall(r'<model\s+name="([^"]+)"', text))
    if names:
        return names
    for match in re.finditer(r'blif_model="\.subckt\s+([^"]+)"', text):
        names.add(match.group(1))
    return names


# USE: intersect .subckt names in a blif with arch-declared hardblock models.
def usedHardblockModels(blifPath, archModels):
    text = Path(blifPath).read_text(encoding="utf-8", errors="replace")
    used = set(SUBCKT_RE.findall(text))
    return sorted(used & set(archModels))


# HELPER: return models only on each side of a set comparison.
def compareSets(parmysModels, mosaicModels):
    left = set(parmysModels)
    right = set(mosaicModels)
    onlyParmys = sorted(left - right)
    onlyMosaic = sorted(right - left)
    return onlyParmys, onlyMosaic


# USE: run one frontend stage (-start/-end) into workDir and return its blif.
def runFrontend(circuit, arch, startStage, workDir):
    workDir = Path(workDir)
    workDir.mkdir(parents=True, exist_ok=True)
    cmd = [
        sys.executable,
        str(RUN_VTR_FLOW),
        str(circuit),
        str(arch),
        "-start",
        startStage,
        "-end",
        startStage,
        "-temp_dir",
        str(workDir),
    ]
    print("running:", " ".join(cmd))
    subprocess.check_call(cmd, cwd=str(REPO_ROOT))
    stem = Path(circuit).stem
    blifName = "{}.{}.blif".format(stem, startStage)
    blifPath = workDir / blifName
    if not blifPath.is_file():
        raise SystemExit("expected blif missing: {}".format(blifPath))
    return blifPath


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--arch",
        type=Path,
        default=DEFAULT_ARCH,
        help="architecture xml (filters hardblock model names)",
    )
    parser.add_argument("--parmys-blif", type=Path, help="existing parmys blif")
    parser.add_argument("--mosaic-blif", type=Path, help="existing mosaic blif")
    parser.add_argument(
        "--run",
        action="store_true",
        help="run parmys and mosaic synth (-end same stage) then compare",
    )
    parser.add_argument(
        "--circuit",
        type=Path,
        default=DEFAULT_CIRCUIT,
        help="circuit for --run (default: diffeq1.v)",
    )
    parser.add_argument(
        "--work-dir",
        type=Path,
        default=None,
        help="parent dir for --run temps (default: mkdtemp under cwd)",
    )
    args = parser.parse_args(argv)

    archPath = Path(args.arch)
    if not archPath.is_file():
        print("error: arch not found: {}".format(archPath), file=sys.stderr)
        return 1

    if args.run:
        if not RUN_VTR_FLOW.is_file():
            print("error: missing {}".format(RUN_VTR_FLOW), file=sys.stderr)
            return 1
        parent = Path(args.work_dir) if args.work_dir else Path(tempfile.mkdtemp(prefix="mosaic_blif_cmp_"))
        parent.mkdir(parents=True, exist_ok=True)
        parmysDir = parent / "parmys"
        mosaicDir = parent / "mosaic"
        try:
            parmysBlif = runFrontend(args.circuit, archPath, "parmys", parmysDir)
            mosaicBlif = runFrontend(args.circuit, archPath, "mosaic", mosaicDir)
        except subprocess.CalledProcessError as exc:
            print("error: flow failed: {}".format(exc), file=sys.stderr)
            return 1
    else:
        if not args.parmys_blif or not args.mosaic_blif:
            print(
                "error: provide --parmys-blif and --mosaic-blif, or --run",
                file=sys.stderr,
            )
            return 1
        parmysBlif = Path(args.parmys_blif)
        mosaicBlif = Path(args.mosaic_blif)
        if not parmysBlif.is_file() or not mosaicBlif.is_file():
            print("error: blif missing", file=sys.stderr)
            return 1

    archModels = archModelNames(archPath)
    if not archModels:
        print("error: no models found in {}".format(archPath), file=sys.stderr)
        return 1

    parmysModels = usedHardblockModels(parmysBlif, archModels)
    mosaicModels = usedHardblockModels(mosaicBlif, archModels)
    onlyParmys, onlyMosaic = compareSets(parmysModels, mosaicModels)

    print("arch: {}".format(archPath))
    print("parmys hardblock .subckt models: {}".format(" ".join(parmysModels) or "-"))
    print("mosaic hardblock .subckt models: {}".format(" ".join(mosaicModels) or "-"))

    if onlyParmys or onlyMosaic:
        if onlyParmys:
            print(
                "only in parmys: {}".format(" ".join(onlyParmys)),
                file=sys.stderr,
            )
        if onlyMosaic:
            print(
                "only in mosaic: {}".format(" ".join(onlyMosaic)),
                file=sys.stderr,
            )
        print("FAIL: hardblock model sets differ", file=sys.stderr)
        return 1

    print("ok: hardblock model sets match")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
