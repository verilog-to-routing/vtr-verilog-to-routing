#!/usr/bin/env bash
# build the frankenstein wildebeest plugin against the yosys that ships inside
# the vtr build. sources are split:
#   frankenstein/wildebeest/src  wildebeest-originated (clk_domains / max_level)
#   frankenstein/src             frankenstein-only (vtr_arch_* / arch_rule_gen)
#
# prerequisites: vtr already built so that <vtr>/build/bin/yosys-config exists
# (the plugin only needs yosys headers; it does NOT link any vtr lib).
#
# run from anywhere:
#   bash frankenstein/build_frankenstein.sh
set -euo pipefail

scriptDir="$(cd "$(dirname "$0")" && pwd)"
vtrDir="${VTR_DIR:-$(cd "${scriptDir}/.." && pwd)}"
wildebeestSrc="${scriptDir}/wildebeest"
frankensteinSrc="${scriptDir}/src"
yosysConfig="${vtrDir}/build/bin/yosys-config"

if [[ -n "${BUILD_JOBS:-}" ]]; then
    jobCount="${BUILD_JOBS}"
else
    jobCount="$(nproc 2>/dev/null || echo 4)"
fi

echo "wildebeest src:    ${wildebeestSrc}"
echo "frankenstein src:  ${frankensteinSrc}"
echo "vtr dir:           ${vtrDir}"
echo "build jobs:        ${jobCount}"

if [[ ! -f "${yosysConfig}" ]]; then
    echo "error: yosys-config not found at ${yosysConfig}"
    echo ""
    echo "build vtr first:"
    echo "  cd ${vtrDir}"
    echo "  make -j\$(nproc)"
    exit 1
fi

if [[ ! -f "${wildebeestSrc}/src/clk_domains.cc" ]]; then
    echo "error: wildebeest source not found at ${wildebeestSrc}"
    exit 1
fi

if [[ ! -f "${frankensteinSrc}/vtr_arch_rules.cc" ]]; then
    echo "error: frankenstein source not found at ${frankensteinSrc}"
    exit 1
fi

echo ""
echo "=== step 1: configure + build the plugin ==="
cmake -S "${wildebeestSrc}" -B "${wildebeestSrc}/build" -DYOSYS_CONFIG="${yosysConfig}"
cmake --build "${wildebeestSrc}/build" -j"${jobCount}"

plugin="${wildebeestSrc}/build/wildebeest.so"
if [[ ! -f "${plugin}" ]]; then
    echo "error: wildebeest.so not produced at ${plugin}"
    exit 1
fi

echo ""
echo "=== step 2: install plugin + share files into the vtr yosys tree ==="
cmake --install "${wildebeestSrc}/build"

echo ""
echo "frankenstein build ok."
echo "  plugin:      ${plugin}"
echo "  installed:   ${vtrDir}/build/share/yosys/plugins/wildebeest.so"
echo ""
