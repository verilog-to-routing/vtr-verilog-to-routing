"""invoke mosaic verilator_check random-vector equivalence from the vtr flow."""

from __future__ import print_function

import subprocess
import sys
from pathlib import Path

from vtr.paths import mosaic_path, root_path, yosys_exe_path


def verilator_check_script_path():
    """return path to mosaic/verilator_check/run_random_check.py."""
    return mosaic_path / "verilator_check" / "run_random_check.py"


def run_verilator_random_check(
    rtl_path,
    post_synth_blif,
    temp_dir,
    include_files=None,
    vectors=50000,
    seed=1,
    verilator_jobs=2,
):
    """run rtl vs post-synth random check. does not abort the cad flow."""
    script = verilator_check_script_path()
    if not script.is_file():
        print("verilator_check script missing {}".format(script), flush=True)
        return 2

    work_dir = Path(temp_dir) / "verilator_check"
    cmd = [
        sys.executable,
        str(script),
        "--rtl",
        str(Path(rtl_path).resolve()),
        "--post-synth-blif",
        str(Path(post_synth_blif).resolve()),
        "--work-dir",
        str(work_dir.resolve()),
        "--vectors",
        str(int(vectors)),
        "--seed",
        str(int(seed)),
        "--verilator-j",
        str(int(verilator_jobs)),
    ]
    if include_files:
        for include in include_files:
            include_path = Path(include)
            if include_path.is_file():
                cmd.extend(["--include", str(include_path.resolve())])
    if Path(yosys_exe_path).is_file():
        cmd.extend(["--yosys", str(Path(yosys_exe_path).resolve())])

    print("mosaic verilator random-check:", " ".join(cmd), flush=True)
    log_path = work_dir.parent / "verilator_random_check.out"
    work_dir.parent.mkdir(parents=True, exist_ok=True)
    with open(str(log_path), "w", encoding="utf-8") as log_file:
        returncode = subprocess.call(
            cmd,
            cwd=str(root_path),
            stdout=log_file,
            stderr=subprocess.STDOUT,
        )
    if returncode != 0:
        print(
            "verilator random-check failed (exit {}) see {}".format(returncode, log_path),
            flush=True,
        )
    return returncode
