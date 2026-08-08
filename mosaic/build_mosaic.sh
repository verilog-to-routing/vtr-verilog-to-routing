#!/usr/bin/env bash
# build the mosaic plugin against the yosys that ships inside the vtr build.

# prerequisites: vtr is already built so <vtr>/build/bin/yosys-config exists
# because the plugin only needs yosys headers and does not link any vtr lib.

scriptDir="$(cd "$(dirname "$0")" && pwd)"
vtrDir="${VTR_DIR:-$(cd "${scriptDir}/.." && pwd)}"
mosaicSrc="${scriptDir}/src"
wildebeestSrc="${scriptDir}/wildebeest"
buildDir="${scriptDir}/build"
yosysConfig="${vtrDir}/build/bin/yosys-config"

if [[ -n "${BUILD_JOBS:-}" ]]; then
    jobCount="${BUILD_JOBS}"
else
    jobCount="$(nproc 2>/dev/null || echo 4)"
fi

echo "mosaic src:        ${mosaicSrc}"
echo "wildebeest lib:    ${wildebeestSrc}"
echo "build dir:         ${buildDir}"
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

if [[ ! -f "${mosaicSrc}/vtr_arch_rules.cc" ]]; then
    echo "error: mosaic source not found at ${mosaicSrc}"
    exit 1
fi

if [[ ! -f "${wildebeestSrc}/src/clk_domains.cc" ]]; then
    echo "error: wildebeest source not found at ${wildebeestSrc}"
    exit 1
fi

echo ""
echo "step 1: configure and build mosaic (mosaic.so)"
cmake -S "${scriptDir}" -B "${buildDir}" -DYOSYS_CONFIG="${yosysConfig}"
cmake --build "${buildDir}" -j"${jobCount}"

plugin="${buildDir}/mosaic.so"
if [[ ! -f "${plugin}" ]]; then
    echo "error: mosaic.so not produced at ${plugin}"
    exit 1
fi

echo ""
echo "step 2: install mosaic plugin into the vtr yosys tree"
cmake --install "${buildDir}"

echo ""
echo "mosaic build ok."
echo "  plugin:      ${plugin}"
echo "  installed:   ${vtrDir}/build/share/yosys/plugins/mosaic.so"
echo ""
