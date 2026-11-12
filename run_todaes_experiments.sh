#!/usr/bin/env bash
# Runs the three TODAES experiment tasks (bidir, unidir, and variable-length interposers).
# Usage: ./run_todaes_experiments.sh [num_parallel_jobs]   (default: 4)
# Requires VPR to be built first (run `make` from the repo root).
set -euo pipefail

cd "$(dirname "$0")"
JOBS="${1:-4}"

if [ -f .venv/bin/activate ]; then
    source .venv/bin/activate
fi

for task in 10um_bidir_interposer 10um_unidir_interposer 10um_varlen_interposer; do
    echo "=== Running TODAES_TASKS/${task} ==="
    ./vtr_flow/scripts/run_vtr_task.py "TODAES_TASKS/${task}" -j "${JOBS}"
done
