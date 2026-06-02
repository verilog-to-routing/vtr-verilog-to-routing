#!/bin/bash
#
# Install dependencies used ONLY by VTR's CI test infrastructure — not needed
# to build or run VTR itself, so they are kept out of install_apt_packages.sh.
#
#   - numpy, scikit-image, Pillow : GUI Layer-5 visual regression (SSIM compare)
#   - gcovr                        : GUI coverage report / regression gate
#
# Executed by the test workflow (.github/workflows/test.yml) in the jobs that
# run those tests.

set -e

pip install scikit-image Pillow numpy gcovr
