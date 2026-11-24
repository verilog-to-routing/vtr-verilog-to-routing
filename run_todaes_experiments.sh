#!/usr/bin/env bash
# Runs the TODAES experiment tasks (bidir, unidir, and variable-length interposers,
# plus the 3D tasks without and with ESD).
# Usage: ./run_todaes_experiments.sh [num_parallel_jobs]   (default: 4)
# Requires VPR to be built first (run `make` from the repo root).
set -euo pipefail

cd "$(dirname "$0")"
JOBS="${1:-4}"

if [ -f .venv/bin/activate ]; then
    source .venv/bin/activate
fi

# Generate the Hecate 3D arch variants (written to arch/multi_die/hecate/3d/hecate_3D/).
# The generator resolves its CSV/template and output paths relative to its own directory,
# so it must run from there. This must happen before the 3D tasks run.
echo "=== Generating Hecate 3D architectures ==="
(cd vtr_flow/arch/multi_die/hecate/3d && python3 generate_hecate_3d_sb_archs.py)

for task in 10um_bidir_interposer 10um_unidir_interposer 10um_varlen_interposer 3d_no_esd 3d_with_esd; do
    echo "=== Running TODAES_TASKS/${task} ==="
    ./vtr_flow/scripts/run_vtr_task.py "TODAES_TASKS/${task}" -j "${JOBS}"
done
