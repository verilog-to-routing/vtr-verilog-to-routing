#!/bin/bash
#
# Install dependencies used ONLY by VTR's CI test infrastructure — not needed
# to build or run VTR itself, so they are kept out of install_apt_packages.sh.
# Versions are pinned in dev/test_requirements.txt.
#
# Executed by the test workflow (.github/workflows/test.yml) in the jobs that
# run those tests.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

pip install -r "${SCRIPT_DIR}/test_requirements.txt"
