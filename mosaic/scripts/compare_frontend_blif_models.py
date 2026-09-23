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


def arch_model_names(arch_xml_path):
    """collect hardblock model names declared in an arch xml."""
    text = Path(arch_xml_path).read_text(encoding="utf-8", errors="replace")
    names = set(re.findall(r'<model\s+name="([^"]+)"', text))
    if names:
        return names
    for match in re.finditer(r'blif_model="\.subckt\s+([^"]+)"', text):
        names.add(match.group(1))
    return names


def used_hardblock_models(blif_path, arch_models):
    """intersect .subckt names in a blif with arch-declared hardblock models."""
    text = Path(blif_path).read_text(encoding="utf-8", errors="replace")
    used = set(SUBCKT_RE.findall(text))
    return sorted(used & set(arch_models))


def compare_sets(parmys_models, mosaic_models):
    """return models only on each side of a set comparison."""
    left = set(parmys_models)
    right = set(mosaic_models)
    only_parmys = sorted(left - right)
    only_mosaic = sorted(right - left)
    return only_parmys, only_mosaic


def run_frontend(circuit, arch, start_stage, work_dir):
    """run one frontend stage (-start/-end) into work_dir and return its blif."""
    work_dir = Path(work_dir).expanduser()
    if not work_dir.is_absolute():
        work_dir = (REPO_ROOT / work_dir).resolve()
    else:
        work_dir = work_dir.resolve()
    work_dir.mkdir(parents=True, exist_ok=True)

    circuit_path = Path(circuit)
    if not circuit_path.is_absolute():
        circuit_path = (REPO_ROOT / circuit_path).resolve()
    arch_path = Path(arch)
    if not arch_path.is_absolute():
        arch_path = (REPO_ROOT / arch_path).resolve()

    cmd = [
        sys.executable,
        str(RUN_VTR_FLOW),
        str(circuit_path),
        str(arch_path),
        "-start",
        start_stage,
        "-end",
        start_stage,
        "-temp_dir",
        str(work_dir),
    ]
    print("running:", " ".join(cmd))
    subprocess.check_call(cmd, cwd=str(REPO_ROOT))
    stem = circuit_path.stem
    blif_name = "{}.{}.blif".format(stem, start_stage)
    blif_path = work_dir / blif_name
    if not blif_path.is_file():
        raise SystemExit("expected blif missing: {}".format(blif_path))
    return blif_path


def main(argv=None):  # pylint: disable=too-many-return-statements,too-many-branches,too-many-statements
    """parse arguments, run or load blifs, and compare hardblock model sets."""
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

    arch_path = Path(args.arch)
    if not arch_path.is_file():
        print("error: arch not found: {}".format(arch_path), file=sys.stderr)
        return 1

    if args.run:
        if not RUN_VTR_FLOW.is_file():
            print("error: missing {}".format(RUN_VTR_FLOW), file=sys.stderr)
            return 1
        if args.work_dir:
            parent = Path(args.work_dir).expanduser()
            if not parent.is_absolute():
                parent = (REPO_ROOT / parent).resolve()
            else:
                parent = parent.resolve()
        else:
            parent = Path(tempfile.mkdtemp(prefix="mosaic_blif_cmp_"))
        parent.mkdir(parents=True, exist_ok=True)
        parmys_dir = parent / "parmys"
        mosaic_dir = parent / "mosaic"
        try:
            parmys_blif = run_frontend(args.circuit, arch_path, "parmys", parmys_dir)
            mosaic_blif = run_frontend(args.circuit, arch_path, "mosaic", mosaic_dir)
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
        parmys_blif = Path(args.parmys_blif)
        mosaic_blif = Path(args.mosaic_blif)
        if not parmys_blif.is_file() or not mosaic_blif.is_file():
            print("error: blif missing", file=sys.stderr)
            return 1

    arch_models = arch_model_names(arch_path)
    if not arch_models:
        print("error: no models found in {}".format(arch_path), file=sys.stderr)
        return 1

    parmys_models = used_hardblock_models(parmys_blif, arch_models)
    mosaic_models = used_hardblock_models(mosaic_blif, arch_models)
    only_parmys, only_mosaic = compare_sets(parmys_models, mosaic_models)

    print("arch: {}".format(arch_path))
    print("parmys hardblock .subckt models: {}".format(" ".join(parmys_models) or "-"))
    print("mosaic hardblock .subckt models: {}".format(" ".join(mosaic_models) or "-"))

    if only_parmys or only_mosaic:
        if only_parmys:
            print(
                "only in parmys: {}".format(" ".join(only_parmys)),
                file=sys.stderr,
            )
        if only_mosaic:
            print(
                "only in mosaic: {}".format(" ".join(only_mosaic)),
                file=sys.stderr,
            )
        print("FAIL: hardblock model sets differ", file=sys.stderr)
        return 1

    print("ok: hardblock model sets match")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
