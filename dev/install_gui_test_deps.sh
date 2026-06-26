#!/bin/bash
#
# Install dependencies used ONLY by VTR's GUI tests — not needed to build or
# run VTR itself, so they are kept out of install_apt_packages.sh. Versions are
# pinned in dev/gui_test_requirements.txt.
#
# Executed by the test workflow (.github/workflows/test.yml) in the GUI test
# jobs.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

pip install -r "${SCRIPT_DIR}/gui_test_requirements.txt"
