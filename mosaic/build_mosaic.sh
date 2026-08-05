#!/usr/bin/env bash
# build the mosaic plugin against the yosys that ships inside the vtr build.
# sources are split between mosaic/wildebeest/src (wildebeest-originated
# clk_domains / max_level) and mosaic/src (mosaic-only vtr_arch_* / arch_rule_gen).

# prerequisites: vtr is already built so <vtr>/build/bin/yosys-config exists
# because the plugin only needs yosys headers and does not link any vtr lib.

scriptDir="$(cd "$(dirname "$0")" && pwd)"
vtrDir="${VTR_DIR:-$(cd "${scriptDir}/.." && pwd)}"
wildebeestSrc="${scriptDir}/wildebeest"
mosaicSrc="${scriptDir}/src"
yosysConfig="${vtrDir}/build/bin/yosys-config"

if [[ -n "${BUILD_JOBS:-}" ]]; then
    jobCount="${BUILD_JOBS}"
else
    jobCount="$(nproc 2>/dev/null || echo 4)"
fi

echo "wildebeest src:    ${wildebeestSrc}"
echo "mosaic src:  ${mosaicSrc}"
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

if [[ ! -f "${mosaicSrc}/vtr_arch_rules.cc" ]]; then
    echo "error: mosaic source not found at ${mosaicSrc}"
    exit 1
fi

echo ""
echo "step 1: configure and build mosaic (wildebeest.so)"cmake -S "${wildebeestSrc}" -B "${wildebeestSrc}/build" -DYOSYS_CONFIG="${yosysConfig}"
cmake --build "${wildebeestSrc}/build" -j"${jobCount}"

plugin="${wildebeestSrc}/build/wildebeest.so"
if [[ ! -f "${plugin}" ]]; then
    echo "error: wildebeest.so not produced at ${plugin}"
    exit 1
fi

echo ""
echo "step 2: install mosaic plugin and share files into the vtr yosys tree"cmake --install "${wildebeestSrc}/build"

echo ""
echo "mosaic build ok."
echo "  plugin:      ${plugin}"
echo "  installed:   ${vtrDir}/build/share/yosys/plugins/wildebeest.so"
echo ""
